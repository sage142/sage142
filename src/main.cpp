#include <cstdio>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "llama.h"

namespace {

struct LlamaState {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    llama_sampler* sampler = nullptr;
    int n_ctx = 2048;
};

void destroy_llama(LlamaState& state) {
    if (state.sampler) {
        llama_sampler_free(state.sampler);
        state.sampler = nullptr;
    }
    if (state.ctx) {
        llama_free(state.ctx);
        state.ctx = nullptr;
    }
    if (state.model) {
        llama_model_free(state.model);
        state.model = nullptr;
    }
}

bool init_llama(LlamaState& state, const char* model_path, std::string& status) {
    destroy_llama(state);

    llama_model_params model_params = llama_model_default_params();
    state.model = llama_model_load_from_file(model_path, model_params);
    if (!state.model) {
        status = "Failed to load model.";
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = state.n_ctx;
    state.ctx = llama_init_from_model(state.model, ctx_params);
    if (!state.ctx) {
        status = "Failed to initialize llama context.";
        destroy_llama(state);
        return false;
    }

    auto* chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(chain, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(chain, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(chain, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(chain, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    state.sampler = chain;

    status = "Model loaded and ready.";
    return true;
}

std::string generate_text(LlamaState& state, const std::string& prompt, int max_tokens) {
    if (!state.model || !state.ctx || !state.sampler) {
        return "[error] Load a model first.";
    }

    const llama_vocab* vocab = llama_model_get_vocab(state.model);
    const int n_prompt = -llama_tokenize(vocab, prompt.c_str(), static_cast<int>(prompt.size()), nullptr, 0, true, true);
    if (n_prompt <= 0) {
        return "[error] Prompt tokenization failed.";
    }

    std::vector<llama_token> prompt_tokens(static_cast<size_t>(n_prompt));
    if (llama_tokenize(vocab, prompt.c_str(), static_cast<int>(prompt.size()), prompt_tokens.data(), n_prompt, true, true) < 0) {
        return "[error] Prompt tokenization failed.";
    }

    llama_kv_cache_clear(state.ctx);
    llama_batch batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int>(prompt_tokens.size()));
    if (llama_decode(state.ctx, batch) != 0) {
        return "[error] Failed to decode prompt.";
    }

    std::string out;
    for (int i = 0; i < max_tokens; ++i) {
        const llama_token token = llama_sampler_sample(state.sampler, state.ctx, -1);
        if (llama_vocab_is_eog(vocab, token)) {
            break;
        }

        char piece[256];
        const int n = llama_token_to_piece(vocab, token, piece, sizeof(piece), 0, true);
        if (n > 0) {
            out.append(piece, static_cast<size_t>(n));
        }

        llama_batch next_batch = llama_batch_get_one(&token, 1);
        if (llama_decode(state.ctx, next_batch) != 0) {
            out += "\n[error] Decoding interrupted.";
            break;
        }
    }

    return out;
}

} // namespace

int main() {
    llama_backend_init();

    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 800, "Dear ImGui + llama.cpp demo", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
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

    char model_path[1024] = "./models/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    char prompt[4096] = "Write a haiku about interactive UIs and machine learning.";
    int max_tokens = 120;
    std::string status = "Load a .gguf model to start.";
    std::string output;
    LlamaState llama_state;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("AI Playground");
        ImGui::TextWrapped("This desktop app combines Dear ImGui with llama.cpp for local text generation.");

        ImGui::InputText("Model path (.gguf)", model_path, IM_ARRAYSIZE(model_path));
        ImGui::InputInt("Max new tokens", &max_tokens);
        if (max_tokens < 1) max_tokens = 1;
        if (max_tokens > 1024) max_tokens = 1024;

        if (ImGui::Button("Load model")) {
            init_llama(llama_state, model_path, status);
        }
        ImGui::SameLine();
        if (ImGui::Button("Unload model")) {
            destroy_llama(llama_state);
            status = "Model unloaded.";
        }

        ImGui::Separator();
        ImGui::TextWrapped("Status: %s", status.c_str());
        ImGui::InputTextMultiline("Prompt", prompt, IM_ARRAYSIZE(prompt), ImVec2(-FLT_MIN, 180));

        if (ImGui::Button("Generate")) {
            output = generate_text(llama_state, prompt, max_tokens);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Output:");
        ImGui::BeginChild("output_box", ImVec2(0, 250), true);
        ImGui::TextWrapped("%s", output.empty() ? "(no output yet)" : output.c_str());
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    destroy_llama(llama_state);
    llama_backend_free();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
