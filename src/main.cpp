#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <SDL2/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#if defined(DAW_HAS_LAME)
#include <lame/lame.h>
#endif

namespace {

constexpr int kSampleRate = 44100;
constexpr int kChannels = 2;

struct AudioClip {
    std::string name;
    std::string source_path;
    std::vector<float> samples; // interleaved stereo
    int start_sample = 0;
    float speed = 1.0f;
    bool selected = false;
};

struct Track {
    std::string name;
    float gain = 1.0f;
    bool mute = false;
    std::vector<AudioClip> clips;
};

struct Project {
    std::string name = "Untitled";
    std::vector<Track> tracks;
};

struct AudioEngine {
    SDL_AudioDeviceID device = 0;
    std::vector<float> mix_buffer;
    bool is_playing = false;
    int playhead_sample = 0;
};

bool load_wav_file(const std::string& path, std::vector<float>& out_samples, std::string& err) {
    SDL_AudioSpec spec{};
    Uint8* data = nullptr;
    Uint32 len = 0;

    if (!SDL_LoadWAV(path.c_str(), &spec, &data, &len)) {
        err = std::string("Failed to load WAV: ") + SDL_GetError();
        return false;
    }

    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, AUDIO_F32, kChannels, kSampleRate) < 0) {
        SDL_FreeWAV(data);
        err = std::string("Audio conversion failed: ") + SDL_GetError();
        return false;
    }

    cvt.len = static_cast<int>(len);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(static_cast<size_t>(cvt.len) * cvt.len_mult));
    if (!cvt.buf) {
        SDL_FreeWAV(data);
        err = "Out of memory while converting audio.";
        return false;
    }

    std::memcpy(cvt.buf, data, len);
    SDL_FreeWAV(data);

    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_free(cvt.buf);
        err = std::string("Audio conversion failed: ") + SDL_GetError();
        return false;
    }

    const auto float_count = cvt.len_cvt / static_cast<int>(sizeof(float));
    out_samples.assign(reinterpret_cast<float*>(cvt.buf), reinterpret_cast<float*>(cvt.buf) + float_count);
    SDL_free(cvt.buf);
    return true;
}

void reverse_clip(AudioClip& clip) {
    const int frames = static_cast<int>(clip.samples.size()) / kChannels;
    for (int i = 0; i < frames / 2; ++i) {
        for (int ch = 0; ch < kChannels; ++ch) {
            std::swap(clip.samples[i * kChannels + ch], clip.samples[(frames - 1 - i) * kChannels + ch]);
        }
    }
}

void change_speed(AudioClip& clip, float new_speed) {
    new_speed = std::clamp(new_speed, 0.25f, 4.0f);
    if (clip.samples.empty()) {
        clip.speed = new_speed;
        return;
    }

    const int old_frames = static_cast<int>(clip.samples.size()) / kChannels;
    const int new_frames = std::max(1, static_cast<int>(old_frames / new_speed));
    std::vector<float> resized(static_cast<size_t>(new_frames * kChannels));

    for (int i = 0; i < new_frames; ++i) {
        const float src = i * new_speed;
        const int a = std::clamp(static_cast<int>(src), 0, old_frames - 1);
        const int b = std::clamp(a + 1, 0, old_frames - 1);
        const float t = src - static_cast<float>(a);
        for (int ch = 0; ch < kChannels; ++ch) {
            const float va = clip.samples[a * kChannels + ch];
            const float vb = clip.samples[b * kChannels + ch];
            resized[i * kChannels + ch] = va + (vb - va) * t;
        }
    }

    clip.samples = std::move(resized);
    clip.speed = new_speed;
}

void chop_clip(Track& track, std::size_t clip_index, int chop_sample) {
    if (clip_index >= track.clips.size()) {
        return;
    }

    AudioClip& clip = track.clips[clip_index];
    const int clip_frames = static_cast<int>(clip.samples.size()) / kChannels;
    const int local_chop = chop_sample - clip.start_sample;
    if (local_chop <= 0 || local_chop >= clip_frames) {
        return;
    }

    AudioClip right = clip;
    right.name += "_part2";
    right.start_sample = chop_sample;

    const int left_size = local_chop * kChannels;
    const int right_size = (clip_frames - local_chop) * kChannels;

    std::vector<float> left_samples(clip.samples.begin(), clip.samples.begin() + left_size);
    std::vector<float> right_samples(clip.samples.begin() + left_size, clip.samples.begin() + left_size + right_size);

    clip.samples = std::move(left_samples);
    right.samples = std::move(right_samples);

    track.clips.insert(track.clips.begin() + static_cast<std::ptrdiff_t>(clip_index + 1), std::move(right));
}

int project_length_samples(const Project& project) {
    int max_end = 0;
    for (const auto& track : project.tracks) {
        for (const auto& clip : track.clips) {
            const int frames = static_cast<int>(clip.samples.size()) / kChannels;
            max_end = std::max(max_end, clip.start_sample + frames);
        }
    }
    return std::max(max_end, kSampleRate * 4);
}

std::vector<float> render_mix(const Project& project) {
    const int total_frames = project_length_samples(project);
    std::vector<float> mix(static_cast<size_t>(total_frames * kChannels), 0.0f);

    for (const auto& track : project.tracks) {
        if (track.mute) {
            continue;
        }
        for (const auto& clip : track.clips) {
            const int clip_frames = static_cast<int>(clip.samples.size()) / kChannels;
            for (int f = 0; f < clip_frames; ++f) {
                const int out_f = clip.start_sample + f;
                if (out_f < 0 || out_f >= total_frames) {
                    continue;
                }
                for (int ch = 0; ch < kChannels; ++ch) {
                    mix[out_f * kChannels + ch] += clip.samples[f * kChannels + ch] * track.gain;
                }
            }
        }
    }

    for (auto& s : mix) {
        s = std::clamp(s, -1.0f, 1.0f);
    }
    return mix;
}

void stop_audio(AudioEngine& engine) {
    if (engine.device != 0) {
        SDL_ClearQueuedAudio(engine.device);
    }
    engine.is_playing = false;
    engine.playhead_sample = 0;
}

void play_audio(AudioEngine& engine, const Project& project) {
    engine.mix_buffer = render_mix(project);
    if (engine.device == 0) {
        return;
    }

    SDL_ClearQueuedAudio(engine.device);
    SDL_QueueAudio(engine.device, engine.mix_buffer.data(), static_cast<Uint32>(engine.mix_buffer.size() * sizeof(float)));
    SDL_PauseAudioDevice(engine.device, 0);
    engine.is_playing = true;
    engine.playhead_sample = 0;
}

void pause_audio(AudioEngine& engine) {
    if (engine.device != 0) {
        SDL_PauseAudioDevice(engine.device, 1);
    }
    engine.is_playing = false;
}

bool save_project(const Project& p, const std::string& file, std::string& err) {
    std::ofstream out(file);
    if (!out) {
        err = "Could not open file for writing.";
        return false;
    }

    out << "PROJECT " << p.name << "\n";
    out << "TRACKS " << p.tracks.size() << "\n";
    for (const auto& track : p.tracks) {
        out << "TRACK " << track.name << " " << track.gain << " " << (track.mute ? 1 : 0) << " " << track.clips.size() << "\n";
        for (const auto& clip : track.clips) {
            out << "CLIP " << clip.name << "|" << clip.source_path << "|" << clip.start_sample << "|" << clip.speed << "\n";
        }
    }
    return true;
}

bool load_project(Project& p, const std::string& file, std::string& err) {
    std::ifstream in(file);
    if (!in) {
        err = "Could not open project file.";
        return false;
    }

    Project loaded;
    std::string tag;
    in >> tag;
    if (tag != "PROJECT") {
        err = "Invalid project file.";
        return false;
    }
    in >> loaded.name;

    std::size_t track_count = 0;
    in >> tag >> track_count;
    if (tag != "TRACKS") {
        err = "Invalid TRACKS section.";
        return false;
    }

    std::string line;
    std::getline(in, line);

    for (std::size_t t = 0; t < track_count; ++t) {
        Track track;
        std::size_t clip_count = 0;
        in >> tag >> track.name >> track.gain >> track.mute >> clip_count;
        std::getline(in, line);
        if (tag != "TRACK") {
            err = "Invalid TRACK line.";
            return false;
        }

        for (std::size_t c = 0; c < clip_count; ++c) {
            std::getline(in, line);
            if (line.rfind("CLIP ", 0) != 0) {
                err = "Invalid CLIP line.";
                return false;
            }
            line = line.substr(5);
            std::array<std::string, 4> parts{};
            std::stringstream ss(line);
            for (int i = 0; i < 4; ++i) {
                if (!std::getline(ss, parts[i], '|')) {
                    err = "Malformed CLIP line.";
                    return false;
                }
            }

            AudioClip clip;
            clip.name = parts[0];
            clip.source_path = parts[1];
            clip.start_sample = std::stoi(parts[2]);
            clip.speed = std::stof(parts[3]);

            if (!load_wav_file(clip.source_path, clip.samples, err)) {
                err = "Failed to load sample '" + clip.source_path + "' while loading project.";
                return false;
            }
            if (std::abs(clip.speed - 1.0f) > 0.001f) {
                change_speed(clip, clip.speed);
            }

            track.clips.push_back(std::move(clip));
        }

        loaded.tracks.push_back(std::move(track));
    }

    p = std::move(loaded);
    return true;
}

#if defined(DAW_HAS_LAME)
bool export_mp3(const Project& project, const std::string& file, std::string& err) {
    const auto mix = render_mix(project);
    const int frames = static_cast<int>(mix.size()) / kChannels;

    lame_t lame = lame_init();
    if (!lame) {
        err = "Failed to initialize LAME.";
        return false;
    }

    lame_set_in_samplerate(lame, kSampleRate);
    lame_set_num_channels(lame, kChannels);
    lame_set_VBR(lame, vbr_default);
    if (lame_init_params(lame) < 0) {
        lame_close(lame);
        err = "Failed to configure LAME.";
        return false;
    }

    std::ofstream out(file, std::ios::binary);
    if (!out) {
        lame_close(lame);
        err = "Could not open MP3 file for writing.";
        return false;
    }

    std::vector<short> pcm(static_cast<size_t>(frames * kChannels));
    for (std::size_t i = 0; i < mix.size(); ++i) {
        pcm[i] = static_cast<short>(std::clamp(mix[i], -1.0f, 1.0f) * 32767.0f);
    }

    std::vector<unsigned char> mp3_buffer(static_cast<size_t>(1.25 * frames + 7200));
    int written = lame_encode_buffer_interleaved(lame, pcm.data(), frames, mp3_buffer.data(), static_cast<int>(mp3_buffer.size()));
    if (written < 0) {
        lame_close(lame);
        err = "LAME encoding failed.";
        return false;
    }
    out.write(reinterpret_cast<const char*>(mp3_buffer.data()), written);

    written = lame_encode_flush(lame, mp3_buffer.data(), static_cast<int>(mp3_buffer.size()));
    if (written > 0) {
        out.write(reinterpret_cast<const char*>(mp3_buffer.data()), written);
    }

    lame_close(lame);
    return true;
}
#else
bool export_mp3(const Project&, const std::string&, std::string& err) {
    err = "MP3 export requires libmp3lame. Rebuild with LAME installed.";
    return false;
}
#endif

} // namespace

int main() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW init failed\n");
        SDL_Quit();
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "BeatCanvas DAW", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        SDL_Quit();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    SDL_AudioSpec want{};
    want.freq = kSampleRate;
    want.format = AUDIO_F32;
    want.channels = kChannels;
    want.samples = 1024;
    want.callback = nullptr;

    AudioEngine engine;
    engine.device = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);

    Project project;
    project.tracks.push_back({"Drums", 1.0f, false, {}});

    int selected_track = 0;
    int selected_clip = -1;
    int chop_at_sample = kSampleRate;
    char sample_path[1024] = "./samples/kick.wav";
    char project_file[1024] = "./project.beat";
    char export_file[1024] = "./mixdown.mp3";
    std::string status = "Ready.";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (engine.device != 0 && engine.is_playing) {
            const Uint32 queued = SDL_GetQueuedAudioSize(engine.device);
            if (queued == 0) {
                engine.is_playing = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("BeatCanvas - Producer View");

        ImGui::Columns(2, nullptr, true);

        ImGui::Text("Tracks");
        if (ImGui::Button("+ Add Track")) {
            project.tracks.push_back({"Track" + std::to_string(project.tracks.size() + 1), 1.0f, false, {}});
        }
        ImGui::Separator();

        for (int t = 0; t < static_cast<int>(project.tracks.size()); ++t) {
            Track& track = project.tracks[t];
            ImGuiTreeNodeFlags flags = (selected_track == t ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_DefaultOpen;
            const bool opened = ImGui::TreeNodeEx((track.name + "##" + std::to_string(t)).c_str(), flags);
            if (ImGui::IsItemClicked()) {
                selected_track = t;
            }
            if (opened) {
                ImGui::Checkbox(("Mute##" + std::to_string(t)).c_str(), &track.mute);
                ImGui::SliderFloat(("Gain##" + std::to_string(t)).c_str(), &track.gain, 0.0f, 2.0f);

                for (int c = 0; c < static_cast<int>(track.clips.size()); ++c) {
                    AudioClip& clip = track.clips[c];
                    if (ImGui::Selectable((clip.name + "##clip" + std::to_string(c)).c_str(), selected_clip == c && selected_track == t)) {
                        selected_track = t;
                        selected_clip = c;
                    }
                }
                ImGui::TreePop();
            }
        }

        ImGui::NextColumn();

        ImGui::Text("Transport");
        if (ImGui::Button("Play")) {
            play_audio(engine, project);
            status = "Playing...";
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            pause_audio(engine);
            status = "Paused.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            stop_audio(engine);
            status = "Stopped.";
        }

        ImGui::Separator();
        ImGui::Text("Sample Loader (WAV)");
        ImGui::InputText("Sample Path", sample_path, IM_ARRAYSIZE(sample_path));
        if (ImGui::Button("Load sample to selected track")) {
            if (selected_track < 0 || selected_track >= static_cast<int>(project.tracks.size())) {
                status = "Select a track first.";
            } else {
                AudioClip clip;
                clip.name = "Clip" + std::to_string(project.tracks[selected_track].clips.size() + 1);
                clip.source_path = sample_path;
                clip.start_sample = 0;
                if (load_wav_file(sample_path, clip.samples, status)) {
                    project.tracks[selected_track].clips.push_back(std::move(clip));
                    status = "Sample loaded.";
                }
            }
        }

        if (selected_track >= 0 && selected_track < static_cast<int>(project.tracks.size()) &&
            selected_clip >= 0 && selected_clip < static_cast<int>(project.tracks[selected_track].clips.size())) {
            auto& clip = project.tracks[selected_track].clips[selected_clip];
            ImGui::Separator();
            ImGui::Text("Clip Tools: %s", clip.name.c_str());

            if (ImGui::Button("Reverse")) {
                reverse_clip(clip);
                status = "Clip reversed.";
            }
            ImGui::SameLine();
            if (ImGui::Button("x0.5")) {
                change_speed(clip, 0.5f);
                status = "Clip speed set to 0.5x.";
            }
            ImGui::SameLine();
            if (ImGui::Button("x1.0")) {
                change_speed(clip, 1.0f);
                status = "Clip speed set to 1.0x.";
            }
            ImGui::SameLine();
            if (ImGui::Button("x2.0")) {
                change_speed(clip, 2.0f);
                status = "Clip speed set to 2.0x.";
            }

            ImGui::InputInt("Chop at sample", &chop_at_sample);
            if (ImGui::Button("Chop Clip")) {
                chop_clip(project.tracks[selected_track], static_cast<std::size_t>(selected_clip), chop_at_sample);
                status = "Clip chopped.";
            }

            ImGui::InputInt("Start sample", &clip.start_sample);
        }

        ImGui::Separator();
        ImGui::Text("Project");
        ImGui::InputText("Project File", project_file, IM_ARRAYSIZE(project_file));
        if (ImGui::Button("Save Project")) {
            if (save_project(project, project_file, status)) {
                status = "Project saved.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Project")) {
            if (load_project(project, project_file, status)) {
                selected_track = project.tracks.empty() ? -1 : 0;
                selected_clip = -1;
                status = "Project loaded.";
            }
        }

        ImGui::InputText("Export MP3", export_file, IM_ARRAYSIZE(export_file));
        if (ImGui::Button("Export")) {
            if (export_mp3(project, export_file, status)) {
                status = "MP3 export complete.";
            }
        }

        ImGui::Separator();
        ImGui::TextWrapped("Timeline / Sequencer");
        ImVec2 timeline_size(ImGui::GetContentRegionAvail().x, 220);
        ImGui::BeginChild("timeline", timeline_size, true);

        const float px_per_sec = 90.0f;
        for (int t = 0; t < static_cast<int>(project.tracks.size()); ++t) {
            const float y = 30.0f + t * 48.0f;
            ImGui::Text("%s", project.tracks[t].name.c_str());
            ImGui::SameLine();

            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddLine(ImVec2(origin.x, y + 16), ImVec2(origin.x + 2000, y + 16), IM_COL32(60, 60, 70, 255));

            for (const auto& clip : project.tracks[t].clips) {
                const float start_sec = static_cast<float>(clip.start_sample) / kSampleRate;
                const float dur_sec = static_cast<float>(clip.samples.size() / kChannels) / kSampleRate;
                const float x0 = origin.x + start_sec * px_per_sec;
                const float x1 = x0 + dur_sec * px_per_sec;
                draw->AddRectFilled(ImVec2(x0, y), ImVec2(x1, y + 28), IM_COL32(90, 170, 255, 210), 4.0f);
                draw->AddText(ImVec2(x0 + 5, y + 6), IM_COL32(20, 20, 20, 255), clip.name.c_str());
            }
            ImGui::Dummy(ImVec2(0, 38));
        }

        ImGui::EndChild();

        ImGui::TextWrapped("Status: %s", status.c_str());

        ImGui::Columns(1);
        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    stop_audio(engine);
    if (engine.device != 0) {
        SDL_CloseAudioDevice(engine.device);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    SDL_Quit();
    return 0;
}
