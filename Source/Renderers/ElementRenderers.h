#ifndef WGAC_ELEMENT_RENDERERS_H
#define WGAC_ELEMENT_RENDERERS_H

#include "WGacRenderer.h"
#include <string>
#include <vector>

namespace vl {
namespace presentation {
namespace elements {
namespace wgac {

// Direction for gradients
enum class Direction {
    Horizontal,
    Vertical,
    Slash,     // top-left to bottom-right
    Backslash  // top-right to bottom-left
};

// Text alignment
enum class TextAlignment {
    Left,
    Center,
    Right
};

enum class VerticalAlignment {
    Top,
    Center,
    Bottom
};

// Base element class
class Element {
protected:
    ElementShape shape;
    bool visible = true;

public:
    virtual ~Element() = default;

    void SetShape(ElementShapeType type, int radiusX = 0, int radiusY = 0) {
        shape.shapeType = type;
        shape.radiusX = radiusX;
        shape.radiusY = radiusY;
    }

    const ElementShape& GetShape() const { return shape; }
    void SetVisible(bool v) { visible = v; }
    bool IsVisible() const { return visible; }
};

// Element renderer base
class IElementRenderer {
public:
    virtual ~IElementRenderer() = default;
    virtual void Render(const Rect& bounds) = 0;
};

// Solid border element
class SolidBorderElement : public Element {
private:
    Color color{0, 0, 0, 255};

public:
    void SetColor(const Color& c) { color = c; }
    const Color& GetColor() const { return color; }
};

// Solid border renderer
class SolidBorderRenderer : public IElementRenderer {
private:
    SolidBorderElement* element;

public:
    explicit SolidBorderRenderer(SolidBorderElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Solid background element
class SolidBackgroundElement : public Element {
private:
    Color color{255, 255, 255, 255};

public:
    void SetColor(const Color& c) { color = c; }
    const Color& GetColor() const { return color; }
};

// Solid background renderer
class SolidBackgroundRenderer : public IElementRenderer {
private:
    SolidBackgroundElement* element;

public:
    explicit SolidBackgroundRenderer(SolidBackgroundElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Gradient background element
class GradientBackgroundElement : public Element {
private:
    Color color1{255, 255, 255, 255};
    Color color2{200, 200, 200, 255};
    Direction direction = Direction::Horizontal;

public:
    void SetColors(const Color& c1, const Color& c2) { color1 = c1; color2 = c2; }
    const Color& GetColor1() const { return color1; }
    const Color& GetColor2() const { return color2; }

    void SetDirection(Direction dir) { direction = dir; }
    Direction GetDirection() const { return direction; }
};

// Gradient background renderer
class GradientBackgroundRenderer : public IElementRenderer {
private:
    GradientBackgroundElement* element;

public:
    explicit GradientBackgroundRenderer(GradientBackgroundElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Solid label (text) element
class SolidLabelElement : public Element {
private:
    std::string text;
    Color color{0, 0, 0, 255};
    FontProperties font;
    TextAlignment hAlignment = TextAlignment::Left;
    VerticalAlignment vAlignment = VerticalAlignment::Top;
    bool wrapLine = false;
    bool ellipse = false;
    bool multiline = false;

public:
    void SetText(const std::string& t) { text = t; }
    const std::string& GetText() const { return text; }

    void SetColor(const Color& c) { color = c; }
    const Color& GetColor() const { return color; }

    void SetFont(const FontProperties& f) { font = f; }
    const FontProperties& GetFont() const { return font; }

    void SetHorizontalAlignment(TextAlignment align) { hAlignment = align; }
    TextAlignment GetHorizontalAlignment() const { return hAlignment; }

    void SetVerticalAlignment(VerticalAlignment align) { vAlignment = align; }
    VerticalAlignment GetVerticalAlignment() const { return vAlignment; }

    void SetWrapLine(bool wrap) { wrapLine = wrap; }
    bool GetWrapLine() const { return wrapLine; }

    void SetEllipse(bool e) { ellipse = e; }
    bool GetEllipse() const { return ellipse; }

    void SetMultiline(bool m) { multiline = m; }
    bool GetMultiline() const { return multiline; }
};

// Solid label renderer
class SolidLabelRenderer : public IElementRenderer {
private:
    SolidLabelElement* element;
    PangoLayout* layout = nullptr;

public:
    explicit SolidLabelRenderer(SolidLabelElement* elem);
    ~SolidLabelRenderer();

    void Render(const Rect& bounds) override;

private:
    void UpdateLayout(cairo_t* cr, int width);
};

// 3D border element (for button-like effects)
class Border3DElement : public Element {
private:
    Color color1{255, 255, 255, 255};  // light color
    Color color2{128, 128, 128, 255};  // dark color

public:
    void SetColors(const Color& c1, const Color& c2) { color1 = c1; color2 = c2; }
    const Color& GetColor1() const { return color1; }
    const Color& GetColor2() const { return color2; }
};

// 3D border renderer
class Border3DRenderer : public IElementRenderer {
private:
    Border3DElement* element;

public:
    explicit Border3DRenderer(Border3DElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Polygon element
class PolygonElement : public Element {
private:
    std::vector<Point> points;
    Color borderColor{0, 0, 0, 255};
    Color backgroundColor{255, 255, 255, 255};
    int borderWidth = 1;

public:
    void SetPoints(const std::vector<Point>& pts) { points = pts; }
    const std::vector<Point>& GetPoints() const { return points; }

    void SetBorderColor(const Color& c) { borderColor = c; }
    const Color& GetBorderColor() const { return borderColor; }

    void SetBackgroundColor(const Color& c) { backgroundColor = c; }
    const Color& GetBackgroundColor() const { return backgroundColor; }

    void SetBorderWidth(int w) { borderWidth = w; }
    int GetBorderWidth() const { return borderWidth; }
};

// Polygon renderer
class PolygonRenderer : public IElementRenderer {
private:
    PolygonElement* element;

public:
    explicit PolygonRenderer(PolygonElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Focus rectangle element
class FocusRectangleElement : public Element {
private:
    Color color{0, 0, 0, 255};

public:
    void SetColor(const Color& c) { color = c; }
    const Color& GetColor() const { return color; }
};

// Focus rectangle renderer (dashed border)
class FocusRectangleRenderer : public IElementRenderer {
private:
    FocusRectangleElement* element;

public:
    explicit FocusRectangleRenderer(FocusRectangleElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Inner shadow element (for inset effects)
class InnerShadowElement : public Element {
private:
    Color color{0, 0, 0, 64};
    int thickness = 5;

public:
    void SetColor(const Color& c) { color = c; }
    const Color& GetColor() const { return color; }

    void SetThickness(int t) { thickness = t; }
    int GetThickness() const { return thickness; }
};

// Inner shadow renderer
class InnerShadowRenderer : public IElementRenderer {
private:
    InnerShadowElement* element;

public:
    explicit InnerShadowRenderer(InnerShadowElement* elem) : element(elem) {}

    void Render(const Rect& bounds) override;
};

// Helper function to draw rounded rectangle path
void DrawRoundedRectPath(cairo_t* cr, const Rect& bounds, const ElementShape& shape);

// Helper function to draw ellipse path
void DrawEllipsePath(cairo_t* cr, const Rect& bounds);

} // namespace wgac
} // namespace elements
} // namespace presentation
} // namespace vl

#endif // WGAC_ELEMENT_RENDERERS_H
