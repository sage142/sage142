#pragma once

#include <imgui.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class Tool {
    Pencil,
    BrushRound,
    BrushBlend,
    Eraser,
    Fill,
    ColorPicker,
    Line,
    Rectangle,
    Ellipse,
    Polygon,
    Text,
    SelectRect,
    SelectFree
};

struct Layer {
    std::string name;
    bool visible{true};
    std::vector<uint32_t> pixels;
};

struct StrokePoint {
    ImVec2 pos;
    float timeSec;
};

struct ReplayStroke {
    Tool tool;
    ImU32 color;
    float size;
    std::vector<StrokePoint> points;
};

class PaintApp {
public:
    PaintApp(int canvasW, int canvasH);
    void render();

private:
    void drawToolbar();
    void drawCanvas();
    void drawPalette();
    void drawLayers();
    void drawStatus();

    void beginStroke(const ImVec2& canvasPos);
    void updateStroke(const ImVec2& canvasPos, float timeSec);
    void endStroke();

    void stampCircle(int x, int y, int radius, ImU32 color, bool blend = false);
    void drawLinePixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color, bool blend = false);
    void drawRectPixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color);
    void drawEllipsePixels(const ImVec2& a, const ImVec2& b, float thickness, ImU32 color);
    void bucketFill(int x, int y, ImU32 color, int tolerance);
    ImU32 sampleColor(int x, int y) const;

    void pushUndo();
    void undo();
    void redo();

    void rebuildComposite();
    void applySymmetry(const ImVec2& p, const ImVec2& prev, float size, ImU32 color, bool blend);

    ImVec2 screenToCanvas(const ImVec2& p) const;
    bool inBounds(int x, int y) const;

private:
    int width_;
    int height_;
    float zoom_{1.0f};
    bool showGrid_{false};
    bool snapToGrid_{false};
    bool symmetry_{false};
    bool smoothing_{true};
    bool shapeCorrection_{true};
    bool pressureSimulation_{true};
    bool replayMode_{false};

    int gridSize_{16};
    int fillTolerance_{20};

    Tool tool_{Tool::Pencil};
    float brushSize_{4.0f};

    ImU32 primaryColor_{IM_COL32(0, 0, 0, 255)};
    ImU32 secondaryColor_{IM_COL32(255, 255, 255, 255)};

    std::vector<Layer> layers_;
    int activeLayer_{0};
    std::vector<uint32_t> composite_;

    std::vector<std::vector<Layer>> undoStack_;
    std::vector<std::vector<Layer>> redoStack_;

    std::optional<ImVec2> strokeStart_;
    std::optional<ImVec2> previousPoint_;
    std::vector<StrokePoint> activeStroke_;
    std::vector<ReplayStroke> replay_;
    float replayCursor_{0.0f};

    ImVec2 canvasTopLeft_{0.0f, 0.0f};

    std::vector<ImU32> palette_;
    char textBuffer_[128] = "Text";
    int textSize_{18};
};
