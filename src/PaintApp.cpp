#include "PaintApp.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace {
constexpr ImU32 kTransparent = IM_COL32(0, 0, 0, 0);

int toInt(float value) { return static_cast<int>(std::round(value)); }

int colorDistance(ImU32 a, ImU32 b) {
    const int ar = static_cast<int>(a & 0xFF);
    const int ag = static_cast<int>((a >> 8) & 0xFF);
    const int ab = static_cast<int>((a >> 16) & 0xFF);
    const int br = static_cast<int>(b & 0xFF);
    const int bg = static_cast<int>((b >> 8) & 0xFF);
    const int bb = static_cast<int>((b >> 16) & 0xFF);
    return std::abs(ar - br) + std::abs(ag - bg) + std::abs(ab - bb);
}

ImU32 blendOver(ImU32 dst, ImU32 src, float alphaFactor = 1.0f) {
    float sa = ((src >> 24) & 0xFF) / 255.0f;
    sa *= std::clamp(alphaFactor, 0.0f, 1.0f);
    float da = ((dst >> 24) & 0xFF) / 255.0f;

    auto chan = [](ImU32 c, int shift) { return static_cast<float>((c >> shift) & 0xFF); };
    float sr = chan(src, 0), sg = chan(src, 8), sb = chan(src, 16);
    float dr = chan(dst, 0), dg = chan(dst, 8), db = chan(dst, 16);

    const float outA = sa + da * (1.0f - sa);
    if (outA <= 0.0f) {
        return 0;
    }

    const float outR = (sr * sa + dr * da * (1.0f - sa)) / outA;
    const float outG = (sg * sa + dg * da * (1.0f - sa)) / outA;
    const float outB = (sb * sa + db * da * (1.0f - sa)) / outA;

    return IM_COL32(static_cast<int>(outR), static_cast<int>(outG), static_cast<int>(outB), static_cast<int>(outA * 255.0f));
}
} // namespace

PaintApp::PaintApp(int canvasW, int canvasH) : width_(canvasW), height_(canvasH) {
    Layer base;
    base.name = "Layer 1";
    base.pixels.assign(static_cast<size_t>(width_ * height_), IM_COL32(255, 255, 255, 255));
    layers_.push_back(base);

    composite_ = base.pixels;

    palette_ = {
        IM_COL32(0, 0, 0, 255),        IM_COL32(255, 255, 255, 255), IM_COL32(255, 0, 0, 255),
        IM_COL32(0, 255, 0, 255),      IM_COL32(0, 0, 255, 255),     IM_COL32(255, 255, 0, 255),
        IM_COL32(0, 255, 255, 255),    IM_COL32(255, 0, 255, 255),   IM_COL32(255, 128, 0, 255),
        IM_COL32(128, 64, 0, 255),     IM_COL32(128, 128, 128, 255), IM_COL32(64, 64, 255, 255),
    };
}

void PaintApp::render() {
    drawToolbar();
    drawPalette();
    drawLayers();
    drawCanvas();
    drawStatus();
}

void PaintApp::drawToolbar() {
    ImGui::Begin("Toolbar");
    const char* toolLabels[] = {"Pencil",  "Round Brush", "Blend Brush", "Eraser", "Fill",     "Picker", "Line",
                                "Rect",    "Ellipse",     "Polygon",     "Text",   "Sel Rect", "Sel Free"};

    for (int i = 0; i < IM_ARRAYSIZE(toolLabels); ++i) {
        if (ImGui::Selectable(toolLabels[i], static_cast<int>(tool_) == i)) {
            tool_ = static_cast<Tool>(i);
        }
        if ((i + 1) % 3 != 0) {
            ImGui::SameLine();
        }
    }

    ImGui::SliderFloat("Brush Size", &brushSize_, 1.0f, 64.0f);
    ImGui::SliderInt("Fill tolerance", &fillTolerance_, 0, 255);

    ImGui::Checkbox("Grid", &showGrid_);
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &snapToGrid_);
    ImGui::SameLine();
    ImGui::Checkbox("Symmetry", &symmetry_);

    ImGui::Checkbox("Smoothing", &smoothing_);
    ImGui::SameLine();
    ImGui::Checkbox("Shape Correction", &shapeCorrection_);
    ImGui::SameLine();
    ImGui::Checkbox("Pressure Sim", &pressureSimulation_);

    ImGui::Checkbox("Replay Mode", &replayMode_);

    if (ImGui::Button("Undo")) undo();
    ImGui::SameLine();
    if (ImGui::Button("Redo")) redo();
    ImGui::SameLine();
    if (ImGui::Button("Swap Colors")) std::swap(primaryColor_, secondaryColor_);

    ImGui::SliderFloat("Zoom", &zoom_, 0.25f, 8.0f, "%.2fx");
    ImGui::SliderInt("Grid Size", &gridSize_, 4, 64);

    ImGui::InputText("Text", textBuffer_, sizeof(textBuffer_));
    ImGui::SliderInt("Text Size", &textSize_, 8, 72);

    ImGui::End();
}

void PaintApp::drawPalette() {
    ImGui::Begin("Colors");
    ImGui::Text("Primary");
    ImGui::ColorButton("##primary", ImGui::ColorConvertU32ToFloat4(primaryColor_), ImGuiColorEditFlags_NoTooltip,
                       ImVec2(40, 40));
    ImGui::SameLine();
    ImGui::Text("Secondary");
    ImGui::SameLine();
    ImGui::ColorButton("##secondary", ImGui::ColorConvertU32ToFloat4(secondaryColor_), ImGuiColorEditFlags_NoTooltip,
                       ImVec2(40, 40));

    ImVec4 color = ImGui::ColorConvertU32ToFloat4(primaryColor_);
    if (ImGui::ColorEdit4("Primary Editor", &color.x)) {
        primaryColor_ = ImGui::ColorConvertFloat4ToU32(color);
    }

    for (size_t i = 0; i < palette_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::ColorButton("##p", ImGui::ColorConvertU32ToFloat4(palette_[i]), ImGuiColorEditFlags_NoTooltip,
                               ImVec2(20, 20))) {
            if (ImGui::GetIO().KeyShift) {
                secondaryColor_ = palette_[i];
            } else {
                primaryColor_ = palette_[i];
            }
        }
        if ((i + 1) % 6 != 0) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::Text("Click to set primary, Shift+Click for secondary");
    ImGui::End();
}

void PaintApp::drawLayers() {
    ImGui::Begin("Layers");
    for (int i = static_cast<int>(layers_.size()) - 1; i >= 0; --i) {
        ImGui::PushID(i);
        ImGui::Checkbox("##vis", &layers_[i].visible);
        ImGui::SameLine();
        if (ImGui::Selectable(layers_[i].name.c_str(), activeLayer_ == i)) {
            activeLayer_ = i;
        }
        ImGui::PopID();
    }

    if (ImGui::Button("Add Layer")) {
        Layer l;
        l.name = "Layer " + std::to_string(layers_.size() + 1);
        l.pixels.assign(static_cast<size_t>(width_ * height_), kTransparent);
        layers_.push_back(std::move(l));
        activeLayer_ = static_cast<int>(layers_.size()) - 1;
    }

    if (layers_.size() > 1 && ImGui::Button("Delete Layer")) {
        pushUndo();
        layers_.erase(layers_.begin() + activeLayer_);
        activeLayer_ = std::clamp(activeLayer_, 0, static_cast<int>(layers_.size()) - 1);
        rebuildComposite();
    }

    ImGui::End();
}

void PaintApp::drawCanvas() {
    ImGui::Begin("Canvas");

    const ImVec2 canvasSize(width_ * zoom_, height_ * zoom_);
    canvasTopLeft_ = ImGui::GetCursorScreenPos();
    auto* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(canvasTopLeft_, ImVec2(canvasTopLeft_.x + canvasSize.x, canvasTopLeft_.y + canvasSize.y),
                            IM_COL32(235, 235, 235, 255));

    rebuildComposite();
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const ImU32 c = composite_[static_cast<size_t>(y * width_ + x)];
            if (((c >> 24) & 0xFF) == 0) continue;
            ImVec2 a(canvasTopLeft_.x + x * zoom_, canvasTopLeft_.y + y * zoom_);
            ImVec2 b(a.x + zoom_ + 0.5f, a.y + zoom_ + 0.5f);
            drawList->AddRectFilled(a, b, c);
        }
    }

    if (showGrid_) {
        for (int x = 0; x <= width_; x += gridSize_) {
            drawList->AddLine(ImVec2(canvasTopLeft_.x + x * zoom_, canvasTopLeft_.y),
                              ImVec2(canvasTopLeft_.x + x * zoom_, canvasTopLeft_.y + canvasSize.y),
                              IM_COL32(0, 0, 0, 30));
        }
        for (int y = 0; y <= height_; y += gridSize_) {
            drawList->AddLine(ImVec2(canvasTopLeft_.x, canvasTopLeft_.y + y * zoom_),
                              ImVec2(canvasTopLeft_.x + canvasSize.x, canvasTopLeft_.y + y * zoom_),
                              IM_COL32(0, 0, 0, 30));
        }
    }

    ImGui::InvisibleButton("canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    const ImVec2 mouseCanvas = screenToCanvas(ImGui::GetIO().MousePos);

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        beginStroke(mouseCanvas);
    }
    if (held && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        updateStroke(mouseCanvas, static_cast<float>(ImGui::GetTime()));
    }
    if (strokeStart_.has_value() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        endStroke();
    }

    ImGui::End();
}

void PaintApp::drawStatus() {
    ImGui::Begin("Status");
    ImVec2 mouseCanvas = screenToCanvas(ImGui::GetIO().MousePos);
    ImGui::Text("Cursor: %.1f, %.1f", mouseCanvas.x, mouseCanvas.y);
    ImGui::Text("Undo: %zu Redo: %zu", undoStack_.size(), redoStack_.size());
    ImGui::Text("Features: Perfect line (Shift), quick-fill assist, replay log (%zu strokes)", replay_.size());
    ImGui::End();
}

void PaintApp::beginStroke(const ImVec2& canvasPos) {
    pushUndo();
    strokeStart_ = canvasPos;
    previousPoint_ = canvasPos;
    activeStroke_.clear();
    activeStroke_.push_back({canvasPos, static_cast<float>(ImGui::GetTime())});

    if (tool_ == Tool::Fill) {
        bucketFill(toInt(canvasPos.x), toInt(canvasPos.y), primaryColor_, fillTolerance_);
        strokeStart_.reset();
    } else if (tool_ == Tool::ColorPicker) {
        primaryColor_ = sampleColor(toInt(canvasPos.x), toInt(canvasPos.y));
        strokeStart_.reset();
    }
}

void PaintApp::updateStroke(const ImVec2& rawCanvasPos, float timeSec) {
    if (!strokeStart_.has_value()) return;

    ImVec2 canvasPos = rawCanvasPos;
    if (snapToGrid_) {
        canvasPos.x = static_cast<float>((toInt(canvasPos.x) / gridSize_) * gridSize_);
        canvasPos.y = static_cast<float>((toInt(canvasPos.y) / gridSize_) * gridSize_);
    }

    if (smoothing_ && previousPoint_.has_value()) {
        canvasPos.x = previousPoint_->x * 0.7f + canvasPos.x * 0.3f;
        canvasPos.y = previousPoint_->y * 0.7f + canvasPos.y * 0.3f;
    }

    const ImU32 drawColor = (ImGui::GetIO().MouseDown[ImGuiMouseButton_Right]) ? secondaryColor_ : primaryColor_;

    if (tool_ == Tool::Pencil || tool_ == Tool::BrushRound || tool_ == Tool::BrushBlend || tool_ == Tool::Eraser) {
        float size = brushSize_;
        if (pressureSimulation_ && previousPoint_.has_value() && !activeStroke_.empty()) {
            float dt = std::max(0.001f, timeSec - activeStroke_.back().timeSec);
            float dx = canvasPos.x - previousPoint_->x;
            float dy = canvasPos.y - previousPoint_->y;
            float speed = std::sqrt(dx * dx + dy * dy) / dt;
            size = std::clamp(brushSize_ + (20.0f - speed * 0.02f), 1.0f, brushSize_ * 2.2f);
        }

        ImVec2 start = *previousPoint_;
        ImVec2 end = canvasPos;
        if (ImGui::GetIO().KeyShift) {
            end.y = start.y;
        }

        if (tool_ == Tool::Eraser) {
            drawLinePixels(start, end, size, kTransparent);
        } else {
            const bool blend = tool_ == Tool::BrushBlend;
            drawLinePixels(start, end, size, drawColor, blend);
            if (symmetry_) {
                applySymmetry(end, start, size, drawColor, blend);
            }
        }
    }

    previousPoint_ = canvasPos;
    activeStroke_.push_back({canvasPos, timeSec});
}

void PaintApp::endStroke() {
    if (!strokeStart_.has_value()) return;

    const ImVec2 start = *strokeStart_;
    const ImVec2 end = previousPoint_.value_or(start);

    if (tool_ == Tool::Line) {
        drawLinePixels(start, end, brushSize_, primaryColor_);
    } else if (tool_ == Tool::Rectangle) {
        drawRectPixels(start, end, brushSize_, primaryColor_);
    } else if (tool_ == Tool::Ellipse) {
        drawEllipsePixels(start, end, brushSize_, primaryColor_);
    } else if (tool_ == Tool::Text) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddText(nullptr, static_cast<float>(textSize_),
                    ImVec2(canvasTopLeft_.x + start.x * zoom_, canvasTopLeft_.y + start.y * zoom_), primaryColor_,
                    textBuffer_);
        stampCircle(toInt(start.x), toInt(start.y), 1, primaryColor_);
    } else if (tool_ == Tool::Polygon) {
        if (activeStroke_.size() > 2) {
            for (size_t i = 1; i < activeStroke_.size(); ++i) {
                drawLinePixels(activeStroke_[i - 1].pos, activeStroke_[i].pos, brushSize_, primaryColor_);
            }
            drawLinePixels(activeStroke_.front().pos, activeStroke_.back().pos, brushSize_, primaryColor_);
        }
    }

    if (shapeCorrection_ && (tool_ == Tool::Pencil || tool_ == Tool::BrushRound) && activeStroke_.size() > 8) {
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 30.0f) {
            drawLinePixels(start, end, brushSize_, primaryColor_);
        }
    }

    if (!activeStroke_.empty()) {
        replay_.push_back({tool_, primaryColor_, brushSize_, activeStroke_});
    }

    strokeStart_.reset();
    previousPoint_.reset();
    activeStroke_.clear();
}

void PaintApp::stampCircle(int x, int y, int radius, ImU32 color, bool blend) {
    if (activeLayer_ < 0 || activeLayer_ >= static_cast<int>(layers_.size())) return;
    auto& pix = layers_[activeLayer_].pixels;
    for (int yy = y - radius; yy <= y + radius; ++yy) {
        for (int xx = x - radius; xx <= x + radius; ++xx) {
            if (!inBounds(xx, yy)) continue;
            const int dx = xx - x;
            const int dy = yy - y;
            if (dx * dx + dy * dy > radius * radius) continue;
            auto& p = pix[static_cast<size_t>(yy * width_ + xx)];
            if (blend) {
                p = blendOver(p, color, 0.25f);
            } else {
                p = color;
            }
        }
    }
}

void PaintApp::drawLinePixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color, bool blend) {
    const int steps = std::max(1, static_cast<int>(std::hypot(b.x - a.x, b.y - a.y) * 2.0f));
    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const float x = a.x + (b.x - a.x) * t;
        const float y = a.y + (b.y - a.y) * t;
        stampCircle(toInt(x), toInt(y), std::max(1, toInt(thickness * 0.5f)), color, blend);
    }
}

void PaintApp::drawRectPixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color) {
    ImVec2 p1(std::min(a.x, b.x), std::min(a.y, b.y));
    ImVec2 p2(std::max(a.x, b.x), std::max(a.y, b.y));
    drawLinePixels({p1.x, p1.y}, {p2.x, p1.y}, thickness, color);
    drawLinePixels({p2.x, p1.y}, {p2.x, p2.y}, thickness, color);
    drawLinePixels({p2.x, p2.y}, {p1.x, p2.y}, thickness, color);
    drawLinePixels({p1.x, p2.y}, {p1.x, p1.y}, thickness, color);
}

void PaintApp::drawEllipsePixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color) {
    const float cx = (a.x + b.x) * 0.5f;
    const float cy = (a.y + b.y) * 0.5f;
    const float rx = std::max(1.0f, std::abs(b.x - a.x) * 0.5f);
    const float ry = std::max(1.0f, std::abs(b.y - a.y) * 0.5f);

    ImVec2 prev(cx + rx, cy);
    for (int i = 1; i <= 360; ++i) {
        float ang = i * 3.1415926f / 180.0f;
        ImVec2 cur(cx + std::cos(ang) * rx, cy + std::sin(ang) * ry);
        drawLinePixels(prev, cur, thickness, color);
        prev = cur;
    }
}

void PaintApp::bucketFill(int x, int y, ImU32 color, int tolerance) {
    if (!inBounds(x, y)) return;
    auto& pix = layers_[activeLayer_].pixels;
    const ImU32 target = pix[static_cast<size_t>(y * width_ + x)];
    if (target == color) return;

    std::queue<std::pair<int, int>> q;
    q.push({x, y});

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        if (!inBounds(cx, cy)) continue;
        auto& p = pix[static_cast<size_t>(cy * width_ + cx)];
        if (colorDistance(p, target) > tolerance) continue;
        p = color;

        q.push({cx + 1, cy});
        q.push({cx - 1, cy});
        q.push({cx, cy + 1});
        q.push({cx, cy - 1});
    }
}

ImU32 PaintApp::sampleColor(int x, int y) const {
    if (!inBounds(x, y)) return primaryColor_;
    return composite_[static_cast<size_t>(y * width_ + x)];
}

void PaintApp::pushUndo() {
    undoStack_.push_back(layers_);
    if (undoStack_.size() > 100) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void PaintApp::undo() {
    if (undoStack_.empty()) return;
    redoStack_.push_back(layers_);
    layers_ = undoStack_.back();
    undoStack_.pop_back();
    rebuildComposite();
}

void PaintApp::redo() {
    if (redoStack_.empty()) return;
    undoStack_.push_back(layers_);
    layers_ = redoStack_.back();
    redoStack_.pop_back();
    rebuildComposite();
}

void PaintApp::rebuildComposite() {
    composite_.assign(static_cast<size_t>(width_ * height_), IM_COL32(255, 255, 255, 255));
    for (const auto& layer : layers_) {
        if (!layer.visible) continue;
        for (size_t i = 0; i < composite_.size(); ++i) {
            composite_[i] = blendOver(composite_[i], layer.pixels[i]);
        }
    }
}

void PaintApp::applySymmetry(const ImVec2& p, const ImVec2& prev, float size, ImU32 color, bool blend) {
    const float centerX = width_ / 2.0f;
    ImVec2 sp(2.0f * centerX - p.x, p.y);
    ImVec2 sPrev(2.0f * centerX - prev.x, prev.y);
    drawLinePixels(sPrev, sp, size, color, blend);
}

ImVec2 PaintApp::screenToCanvas(const ImVec2& p) const {
    return {(p.x - canvasTopLeft_.x) / zoom_, (p.y - canvasTopLeft_.y) / zoom_};
}

bool PaintApp::inBounds(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_; }
