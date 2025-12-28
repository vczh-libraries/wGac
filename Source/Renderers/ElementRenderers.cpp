#include "ElementRenderers.h"
#include <cmath>
#include <algorithm>

namespace vl {
namespace presentation {
namespace elements {
namespace wgac {

// Helper functions

void DrawRoundedRectPath(cairo_t* cr, const Rect& bounds, const ElementShape& shape) {
    double degrees = M_PI / 180.0;
    double rx = shape.radiusX;
    double ry = shape.radiusY;

    if (rx <= 0 || ry <= 0) {
        cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
        return;
    }

    // Handle elliptical corners by scaling
    double larger = std::max(rx, ry);
    double smaller = std::min(rx, ry);
    double scale = smaller / larger;

    cairo_save(cr);

    if (rx > ry) {
        cairo_scale(cr, 1.0, scale);
        double y1_scaled = bounds.y1 / scale;
        double y2_scaled = bounds.y2 / scale;

        cairo_new_sub_path(cr);
        cairo_arc(cr, bounds.x1 + rx, y1_scaled + larger, larger, 180 * degrees, 270 * degrees);
        cairo_arc(cr, bounds.x2 - rx, y1_scaled + larger, larger, 270 * degrees, 0);
        cairo_arc(cr, bounds.x2 - rx, y2_scaled - larger, larger, 0, 90 * degrees);
        cairo_arc(cr, bounds.x1 + rx, y2_scaled - larger, larger, 90 * degrees, 180 * degrees);
    } else {
        cairo_scale(cr, scale, 1.0);
        double x1_scaled = bounds.x1 / scale;
        double x2_scaled = bounds.x2 / scale;

        cairo_new_sub_path(cr);
        cairo_arc(cr, x1_scaled + larger, bounds.y1 + ry, larger, 180 * degrees, 270 * degrees);
        cairo_arc(cr, x2_scaled - larger, bounds.y1 + ry, larger, 270 * degrees, 0);
        cairo_arc(cr, x2_scaled - larger, bounds.y2 - ry, larger, 0, 90 * degrees);
        cairo_arc(cr, x1_scaled + larger, bounds.y2 - ry, larger, 90 * degrees, 180 * degrees);
    }

    cairo_close_path(cr);
    cairo_restore(cr);

    // Re-create the path without scaling for proper stroke/fill
    cairo_new_sub_path(cr);
    cairo_arc(cr, bounds.x1 + rx, bounds.y1 + ry, rx, 180 * degrees, 270 * degrees);
    cairo_arc(cr, bounds.x2 - rx, bounds.y1 + ry, rx, 270 * degrees, 0);
    cairo_arc(cr, bounds.x2 - rx, bounds.y2 - ry, rx, 0, 90 * degrees);
    cairo_arc(cr, bounds.x1 + rx, bounds.y2 - ry, rx, 90 * degrees, 180 * degrees);
    cairo_close_path(cr);
}

void DrawEllipsePath(cairo_t* cr, const Rect& bounds) {
    double cx = bounds.x1 + bounds.Width() / 2.0;
    double cy = bounds.y1 + bounds.Height() / 2.0;
    double rx = bounds.Width() / 2.0;
    double ry = bounds.Height() / 2.0;

    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(cr);
}

// SolidBorderRenderer

void SolidBorderRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c = element->GetColor();
    cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
    cairo_set_line_width(cr, 1);

    const ElementShape& shape = element->GetShape();
    Rect strokeBounds(bounds.x1 + 0.5, bounds.y1 + 0.5,
                      bounds.x2 - 0.5, bounds.y2 - 0.5);

    switch (shape.shapeType) {
        case ElementShapeType::RoundRect:
            DrawRoundedRectPath(cr, strokeBounds, shape);
            cairo_stroke(cr);
            break;

        case ElementShapeType::Ellipse:
            DrawEllipsePath(cr, strokeBounds);
            cairo_stroke(cr);
            break;

        case ElementShapeType::Rectangle:
        default:
            cairo_rectangle(cr, strokeBounds.x1, strokeBounds.y1,
                           strokeBounds.Width(), strokeBounds.Height());
            cairo_stroke(cr);
            break;
    }
}

// SolidBackgroundRenderer

void SolidBackgroundRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c = element->GetColor();
    cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);

    const ElementShape& shape = element->GetShape();

    switch (shape.shapeType) {
        case ElementShapeType::RoundRect:
            DrawRoundedRectPath(cr, bounds, shape);
            cairo_fill(cr);
            break;

        case ElementShapeType::Ellipse:
            DrawEllipsePath(cr, bounds);
            cairo_fill(cr);
            break;

        case ElementShapeType::Rectangle:
        default:
            cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
            cairo_fill(cr);
            break;
    }
}

// GradientBackgroundRenderer

void GradientBackgroundRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c1 = element->GetColor1();
    Color c2 = element->GetColor2();

    double x0, y0, x1, y1;
    switch (element->GetDirection()) {
        case Direction::Horizontal:
            x0 = bounds.x1; y0 = bounds.y1;
            x1 = bounds.x2; y1 = bounds.y1;
            break;
        case Direction::Vertical:
            x0 = bounds.x1; y0 = bounds.y1;
            x1 = bounds.x1; y1 = bounds.y2;
            break;
        case Direction::Slash:
            x0 = bounds.x1; y0 = bounds.y1;
            x1 = bounds.x2; y1 = bounds.y2;
            break;
        case Direction::Backslash:
            x0 = bounds.x2; y0 = bounds.y1;
            x1 = bounds.x1; y1 = bounds.y2;
            break;
    }

    cairo_pattern_t* gradient = cairo_pattern_create_linear(x0, y0, x1, y1);
    cairo_pattern_add_color_stop_rgba(gradient, 0, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
    cairo_pattern_add_color_stop_rgba(gradient, 1, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);

    cairo_set_source(cr, gradient);

    const ElementShape& shape = element->GetShape();
    switch (shape.shapeType) {
        case ElementShapeType::RoundRect:
            DrawRoundedRectPath(cr, bounds, shape);
            cairo_fill(cr);
            break;

        case ElementShapeType::Ellipse:
            DrawEllipsePath(cr, bounds);
            cairo_fill(cr);
            break;

        case ElementShapeType::Rectangle:
        default:
            cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
            cairo_fill(cr);
            break;
    }

    cairo_pattern_destroy(gradient);
}

// SolidLabelRenderer

SolidLabelRenderer::SolidLabelRenderer(SolidLabelElement* elem) : element(elem) {
}

SolidLabelRenderer::~SolidLabelRenderer() {
    if (layout) {
        g_object_unref(layout);
    }
}

void SolidLabelRenderer::UpdateLayout(cairo_t* cr, int width) {
    if (!layout) {
        layout = pango_cairo_create_layout(cr);
    } else {
        pango_cairo_update_layout(cr, layout);
    }

    // Set font
    const FontProperties& font = element->GetFont();
    IWGacResourceManager* rm = GetWGacResourceManager();
    if (rm) {
        PangoFontDescription* desc = rm->CreateFont(font);
        pango_layout_set_font_description(layout, desc);
    }

    // Set text
    pango_layout_set_text(layout, element->GetText().c_str(), -1);

    // Set width for wrapping
    if (element->GetWrapLine() && width > 0) {
        pango_layout_set_width(layout, width * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    } else {
        pango_layout_set_width(layout, -1);
    }

    // Set ellipsize
    if (element->GetEllipse()) {
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    } else {
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    }

    // Set alignment
    PangoAlignment align = PANGO_ALIGN_LEFT;
    switch (element->GetHorizontalAlignment()) {
        case TextAlignment::Center: align = PANGO_ALIGN_CENTER; break;
        case TextAlignment::Right: align = PANGO_ALIGN_RIGHT; break;
        default: break;
    }
    pango_layout_set_alignment(layout, align);
}

void SolidLabelRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;
    if (element->GetText().empty()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    UpdateLayout(cr, bounds.Width());

    // Get layout size
    int textWidth, textHeight;
    pango_layout_get_pixel_size(layout, &textWidth, &textHeight);

    // Calculate position based on alignment
    double x = bounds.x1;
    double y = bounds.y1;

    switch (element->GetHorizontalAlignment()) {
        case TextAlignment::Center:
            x = bounds.x1 + (bounds.Width() - textWidth) / 2.0;
            break;
        case TextAlignment::Right:
            x = bounds.x2 - textWidth;
            break;
        default:
            break;
    }

    switch (element->GetVerticalAlignment()) {
        case VerticalAlignment::Center:
            y = bounds.y1 + (bounds.Height() - textHeight) / 2.0;
            break;
        case VerticalAlignment::Bottom:
            y = bounds.y2 - textHeight;
            break;
        default:
            break;
    }

    // Set color and draw
    Color c = element->GetColor();
    cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);

    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);
}

// Border3DRenderer

void Border3DRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c1 = element->GetColor1();
    Color c2 = element->GetColor2();

    cairo_set_line_width(cr, 1);

    // Light edges (top and left)
    cairo_set_source_rgba(cr, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
    cairo_move_to(cr, bounds.x1 + 0.5, bounds.y2 - 0.5);
    cairo_line_to(cr, bounds.x1 + 0.5, bounds.y1 + 0.5);
    cairo_line_to(cr, bounds.x2 - 0.5, bounds.y1 + 0.5);
    cairo_stroke(cr);

    // Dark edges (bottom and right)
    cairo_set_source_rgba(cr, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);
    cairo_move_to(cr, bounds.x2 - 0.5, bounds.y1 + 0.5);
    cairo_line_to(cr, bounds.x2 - 0.5, bounds.y2 - 0.5);
    cairo_line_to(cr, bounds.x1 + 0.5, bounds.y2 - 0.5);
    cairo_stroke(cr);
}

// PolygonRenderer

void PolygonRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    const auto& points = element->GetPoints();
    if (points.size() < 3) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    // Start path
    cairo_move_to(cr, bounds.x1 + points[0].x, bounds.y1 + points[0].y);
    for (size_t i = 1; i < points.size(); ++i) {
        cairo_line_to(cr, bounds.x1 + points[i].x, bounds.y1 + points[i].y);
    }
    cairo_close_path(cr);

    // Fill
    Color bg = element->GetBackgroundColor();
    cairo_set_source_rgba(cr, bg.r / 255.0, bg.g / 255.0, bg.b / 255.0, bg.a / 255.0);
    cairo_fill_preserve(cr);

    // Stroke
    Color border = element->GetBorderColor();
    cairo_set_source_rgba(cr, border.r / 255.0, border.g / 255.0, border.b / 255.0, border.a / 255.0);
    cairo_set_line_width(cr, element->GetBorderWidth());
    cairo_stroke(cr);
}

// FocusRectangleRenderer

void FocusRectangleRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c = element->GetColor();
    cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);

    // Set dashed line
    double dashes[] = {1.0, 1.0};
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_set_line_width(cr, 1);

    cairo_rectangle(cr, bounds.x1 + 0.5, bounds.y1 + 0.5,
                   bounds.Width() - 1, bounds.Height() - 1);
    cairo_stroke(cr);

    // Reset dash
    cairo_set_dash(cr, nullptr, 0, 0);
}

// InnerShadowRenderer

void InnerShadowRenderer::Render(const Rect& bounds) {
    if (!element->IsVisible()) return;

    cairo_t* cr = GetCurrentCairoContext();
    if (!cr) return;

    Color c = element->GetColor();
    int thickness = element->GetThickness();

    if (thickness <= 0) return;

    // Create gradients for each edge
    // Top shadow
    cairo_pattern_t* topGradient = cairo_pattern_create_linear(0, bounds.y1, 0, bounds.y1 + thickness);
    cairo_pattern_add_color_stop_rgba(topGradient, 0, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
    cairo_pattern_add_color_stop_rgba(topGradient, 1, c.r / 255.0, c.g / 255.0, c.b / 255.0, 0);

    cairo_set_source(cr, topGradient);
    cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), thickness);
    cairo_fill(cr);
    cairo_pattern_destroy(topGradient);

    // Left shadow
    cairo_pattern_t* leftGradient = cairo_pattern_create_linear(bounds.x1, 0, bounds.x1 + thickness, 0);
    cairo_pattern_add_color_stop_rgba(leftGradient, 0, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
    cairo_pattern_add_color_stop_rgba(leftGradient, 1, c.r / 255.0, c.g / 255.0, c.b / 255.0, 0);

    cairo_set_source(cr, leftGradient);
    cairo_rectangle(cr, bounds.x1, bounds.y1, thickness, bounds.Height());
    cairo_fill(cr);
    cairo_pattern_destroy(leftGradient);

    // Bottom shadow (lighter)
    cairo_pattern_t* bottomGradient = cairo_pattern_create_linear(0, bounds.y2 - thickness, 0, bounds.y2);
    cairo_pattern_add_color_stop_rgba(bottomGradient, 0, c.r / 255.0, c.g / 255.0, c.b / 255.0, 0);
    cairo_pattern_add_color_stop_rgba(bottomGradient, 1, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 510.0);

    cairo_set_source(cr, bottomGradient);
    cairo_rectangle(cr, bounds.x1, bounds.y2 - thickness, bounds.Width(), thickness);
    cairo_fill(cr);
    cairo_pattern_destroy(bottomGradient);

    // Right shadow (lighter)
    cairo_pattern_t* rightGradient = cairo_pattern_create_linear(bounds.x2 - thickness, 0, bounds.x2, 0);
    cairo_pattern_add_color_stop_rgba(rightGradient, 0, c.r / 255.0, c.g / 255.0, c.b / 255.0, 0);
    cairo_pattern_add_color_stop_rgba(rightGradient, 1, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 510.0);

    cairo_set_source(cr, rightGradient);
    cairo_rectangle(cr, bounds.x2 - thickness, bounds.y1, thickness, bounds.Height());
    cairo_fill(cr);
    cairo_pattern_destroy(rightGradient);
}

} // namespace wgac
} // namespace elements
} // namespace presentation
} // namespace vl
