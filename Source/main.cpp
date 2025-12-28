#include "Wayland/WaylandDisplay.h"
#include "Wayland/WaylandSeat.h"
#include "WGacWindow.h"
#include "WGacView.h"
#include "WGacCursor.h"
#include "Renderers/WGacRenderer.h"
#include "Renderers/ElementRenderers.h"
#include <cstdio>
#include <cmath>
#include <memory>

using namespace vl::presentation::wayland;
using namespace vl::presentation::elements::wgac;

// Global resources
std::unique_ptr<WGacResourceManager> g_resourceManager;
std::unique_ptr<WGacObjectProvider> g_objectProvider;
std::unique_ptr<WGacCursor> g_cursor;

// Input state for display
int g_mouseX = 0;
int g_mouseY = 0;
bool g_mouseLeft = false;
bool g_mouseRight = false;
bool g_mouseMiddle = false;
bool g_focused = false;
std::string g_lastKey;
std::string g_modifiers;

// Demo elements
std::unique_ptr<GradientBackgroundElement> bgElement;
std::unique_ptr<GradientBackgroundRenderer> bgRenderer;

std::unique_ptr<SolidBackgroundElement> panelElement;
std::unique_ptr<SolidBackgroundRenderer> panelRenderer;

std::unique_ptr<SolidBorderElement> borderElement;
std::unique_ptr<SolidBorderRenderer> borderRenderer;

std::unique_ptr<SolidLabelElement> titleElement;
std::unique_ptr<SolidLabelRenderer> titleRenderer;

std::unique_ptr<SolidLabelElement> subtitleElement;
std::unique_ptr<SolidLabelRenderer> subtitleRenderer;

std::unique_ptr<SolidLabelElement> mouseLabel;
std::unique_ptr<SolidLabelRenderer> mouseLabelRenderer;

std::unique_ptr<SolidLabelElement> keyLabel;
std::unique_ptr<SolidLabelRenderer> keyLabelRenderer;

std::unique_ptr<SolidLabelElement> modLabel;
std::unique_ptr<SolidLabelRenderer> modLabelRenderer;

std::unique_ptr<SolidBackgroundElement> clickIndicator;
std::unique_ptr<SolidBackgroundRenderer> clickIndicatorRenderer;

std::unique_ptr<SolidBackgroundElement> button1;
std::unique_ptr<SolidBackgroundRenderer> button1Renderer;
std::unique_ptr<SolidBorderElement> button1Border;
std::unique_ptr<SolidBorderRenderer> button1BorderRenderer;
std::unique_ptr<SolidLabelElement> button1Label;
std::unique_ptr<SolidLabelRenderer> button1LabelRenderer;

std::unique_ptr<SolidBackgroundElement> button2;
std::unique_ptr<SolidBackgroundRenderer> button2Renderer;
std::unique_ptr<SolidBorderElement> button2Border;
std::unique_ptr<SolidBorderRenderer> button2BorderRenderer;
std::unique_ptr<SolidLabelElement> button2Label;
std::unique_ptr<SolidLabelRenderer> button2LabelRenderer;

std::unique_ptr<SolidLabelElement> statusLabel;
std::unique_ptr<SolidLabelRenderer> statusLabelRenderer;

// Button hover states
bool g_button1Hover = false;
bool g_button2Hover = false;
Rect g_button1Rect, g_button2Rect;

void InitElements() {
    // Background gradient
    bgElement = std::make_unique<GradientBackgroundElement>();
    bgElement->SetColors(Color(40, 60, 80), Color(20, 30, 40));
    bgElement->SetDirection(Direction::Vertical);
    bgRenderer = std::make_unique<GradientBackgroundRenderer>(bgElement.get());

    // White panel
    panelElement = std::make_unique<SolidBackgroundElement>();
    panelElement->SetColor(Color(250, 250, 250, 245));
    panelElement->SetShape(ElementShapeType::RoundRect, 15, 15);
    panelRenderer = std::make_unique<SolidBackgroundRenderer>(panelElement.get());

    // Panel border
    borderElement = std::make_unique<SolidBorderElement>();
    borderElement->SetColor(Color(80, 80, 80));
    borderElement->SetShape(ElementShapeType::RoundRect, 15, 15);
    borderRenderer = std::make_unique<SolidBorderRenderer>(borderElement.get());

    // Title
    titleElement = std::make_unique<SolidLabelElement>();
    titleElement->SetText("wGac - Phase 3: Input System");
    titleElement->SetColor(Color(30, 30, 30));
    FontProperties titleFont;
    titleFont.fontFamily = "Sans";
    titleFont.size = 22;
    titleFont.bold = true;
    titleElement->SetFont(titleFont);
    titleElement->SetHorizontalAlignment(TextAlignment::Center);
    titleRenderer = std::make_unique<SolidLabelRenderer>(titleElement.get());

    // Subtitle
    subtitleElement = std::make_unique<SolidLabelElement>();
    subtitleElement->SetText("Mouse and Keyboard Input Demo");
    subtitleElement->SetColor(Color(100, 100, 100));
    FontProperties subFont;
    subFont.fontFamily = "Sans";
    subFont.size = 13;
    subtitleElement->SetFont(subFont);
    subtitleElement->SetHorizontalAlignment(TextAlignment::Center);
    subtitleRenderer = std::make_unique<SolidLabelRenderer>(subtitleElement.get());

    // Mouse info label
    mouseLabel = std::make_unique<SolidLabelElement>();
    mouseLabel->SetColor(Color(50, 50, 50));
    FontProperties infoFont;
    infoFont.fontFamily = "Monospace";
    infoFont.size = 12;
    mouseLabel->SetFont(infoFont);
    mouseLabelRenderer = std::make_unique<SolidLabelRenderer>(mouseLabel.get());

    // Key info label
    keyLabel = std::make_unique<SolidLabelElement>();
    keyLabel->SetColor(Color(50, 50, 50));
    keyLabel->SetFont(infoFont);
    keyLabelRenderer = std::make_unique<SolidLabelRenderer>(keyLabel.get());

    // Modifier label
    modLabel = std::make_unique<SolidLabelElement>();
    modLabel->SetColor(Color(100, 100, 100));
    modLabel->SetFont(infoFont);
    modLabelRenderer = std::make_unique<SolidLabelRenderer>(modLabel.get());

    // Click indicator
    clickIndicator = std::make_unique<SolidBackgroundElement>();
    clickIndicator->SetColor(Color(100, 100, 100));
    clickIndicator->SetShape(ElementShapeType::Ellipse);
    clickIndicatorRenderer = std::make_unique<SolidBackgroundRenderer>(clickIndicator.get());

    // Button 1
    button1 = std::make_unique<SolidBackgroundElement>();
    button1->SetColor(Color(70, 130, 180));
    button1->SetShape(ElementShapeType::RoundRect, 8, 8);
    button1Renderer = std::make_unique<SolidBackgroundRenderer>(button1.get());

    button1Border = std::make_unique<SolidBorderElement>();
    button1Border->SetColor(Color(50, 100, 150));
    button1Border->SetShape(ElementShapeType::RoundRect, 8, 8);
    button1BorderRenderer = std::make_unique<SolidBorderRenderer>(button1Border.get());

    button1Label = std::make_unique<SolidLabelElement>();
    button1Label->SetText("Button 1");
    button1Label->SetColor(Color(255, 255, 255));
    FontProperties btnFont;
    btnFont.fontFamily = "Sans";
    btnFont.size = 13;
    btnFont.bold = true;
    button1Label->SetFont(btnFont);
    button1Label->SetHorizontalAlignment(TextAlignment::Center);
    button1LabelRenderer = std::make_unique<SolidLabelRenderer>(button1Label.get());

    // Button 2
    button2 = std::make_unique<SolidBackgroundElement>();
    button2->SetColor(Color(60, 160, 60));
    button2->SetShape(ElementShapeType::RoundRect, 8, 8);
    button2Renderer = std::make_unique<SolidBackgroundRenderer>(button2.get());

    button2Border = std::make_unique<SolidBorderElement>();
    button2Border->SetColor(Color(40, 120, 40));
    button2Border->SetShape(ElementShapeType::RoundRect, 8, 8);
    button2BorderRenderer = std::make_unique<SolidBorderRenderer>(button2Border.get());

    button2Label = std::make_unique<SolidLabelElement>();
    button2Label->SetText("Button 2");
    button2Label->SetColor(Color(255, 255, 255));
    button2Label->SetFont(btnFont);
    button2Label->SetHorizontalAlignment(TextAlignment::Center);
    button2LabelRenderer = std::make_unique<SolidLabelRenderer>(button2Label.get());

    // Status label
    statusLabel = std::make_unique<SolidLabelElement>();
    statusLabel->SetColor(Color(80, 80, 80));
    FontProperties statusFont;
    statusFont.fontFamily = "Sans";
    statusFont.size = 11;
    statusLabel->SetFont(statusFont);
    statusLabel->SetHorizontalAlignment(TextAlignment::Center);
    statusLabelRenderer = std::make_unique<SolidLabelRenderer>(statusLabel.get());
}

bool PointInRect(int x, int y, const Rect& r) {
    return x >= r.x1 && x < r.x2 && y >= r.y1 && y < r.y2;
}

void RenderDemo(WGacWindow* window) {
    int width = window->GetWidth();
    int height = window->GetHeight();

    // Get render target
    IWGacRenderTarget* rt = g_objectProvider->GetRenderTarget(window);
    if (!rt) return;

    rt->StartRendering();

    // Draw background
    bgRenderer->Render(Rect(0, 0, width, height));

    // Draw panel
    int panelX = 30, panelY = 30;
    int panelW = width - 60, panelH = height - 60;
    if (panelW > 0 && panelH > 0) {
        Rect panelRect(panelX, panelY, panelX + panelW, panelY + panelH);
        panelRenderer->Render(panelRect);
        borderRenderer->Render(panelRect);
    }

    // Draw title
    titleRenderer->Render(Rect(panelX, panelY + 15, panelX + panelW, panelY + 50));

    // Draw subtitle
    subtitleRenderer->Render(Rect(panelX, panelY + 45, panelX + panelW, panelY + 70));

    // Draw mouse info
    char mouseText[128];
    snprintf(mouseText, sizeof(mouseText), "Mouse: (%d, %d)  Buttons: [%s%s%s]",
             g_mouseX, g_mouseY,
             g_mouseLeft ? "L" : "-",
             g_mouseMiddle ? "M" : "-",
             g_mouseRight ? "R" : "-");
    mouseLabel->SetText(mouseText);
    mouseLabelRenderer->Render(Rect(panelX + 20, panelY + 90, panelX + panelW - 20, panelY + 110));

    // Draw key info
    char keyText[128];
    snprintf(keyText, sizeof(keyText), "Last Key: %s", g_lastKey.empty() ? "(none)" : g_lastKey.c_str());
    keyLabel->SetText(keyText);
    keyLabelRenderer->Render(Rect(panelX + 20, panelY + 115, panelX + panelW - 20, panelY + 135));

    // Draw modifier info
    modLabel->SetText(g_modifiers.empty() ? "Modifiers: (none)" : ("Modifiers: " + g_modifiers).c_str());
    modLabelRenderer->Render(Rect(panelX + 20, panelY + 140, panelX + panelW - 20, panelY + 160));

    // Draw click indicator
    Color indicatorColor(100, 100, 100);
    if (g_mouseLeft) indicatorColor = Color(220, 50, 50);
    else if (g_mouseRight) indicatorColor = Color(50, 50, 220);
    else if (g_mouseMiddle) indicatorColor = Color(50, 180, 50);
    clickIndicator->SetColor(indicatorColor);
    clickIndicatorRenderer->Render(Rect(panelX + panelW - 80, panelY + 90, panelX + panelW - 30, panelY + 140));

    // Draw buttons
    int btnY = panelY + 180;
    int btnW = 120, btnH = 40;
    int btnSpacing = 30;

    // Button 1
    g_button1Rect = Rect(panelX + 50, btnY, panelX + 50 + btnW, btnY + btnH);
    Color btn1Color = g_button1Hover ? Color(100, 150, 200) : Color(70, 130, 180);
    button1->SetColor(btn1Color);
    button1Renderer->Render(g_button1Rect);
    button1BorderRenderer->Render(g_button1Rect);
    button1LabelRenderer->Render(Rect(g_button1Rect.x1, g_button1Rect.y1 + 10, g_button1Rect.x2, g_button1Rect.y2));

    // Button 2
    g_button2Rect = Rect(panelX + 50 + btnW + btnSpacing, btnY, panelX + 50 + 2*btnW + btnSpacing, btnY + btnH);
    Color btn2Color = g_button2Hover ? Color(80, 200, 80) : Color(60, 160, 60);
    button2->SetColor(btn2Color);
    button2Renderer->Render(g_button2Rect);
    button2BorderRenderer->Render(g_button2Rect);
    button2LabelRenderer->Render(Rect(g_button2Rect.x1, g_button2Rect.y1 + 10, g_button2Rect.x2, g_button2Rect.y2));

    // Status
    char statusText[128];
    snprintf(statusText, sizeof(statusText), "Focus: %s | Press ESC to exit | Size: %dx%d",
             g_focused ? "Yes" : "No", width, height);
    statusLabel->SetText(statusText);
    statusLabelRenderer->Render(Rect(panelX, panelY + panelH - 35, panelX + panelW, panelY + panelH - 10));

    rt->StopRendering();
}

int main() {
    printf("wGac Test - Phase 3: Input System\n");
    printf("==================================\n\n");

    // Create display
    WaylandDisplay display;

    printf("Connecting to Wayland display...\n");
    if (!display.Connect()) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
    printf("Connected successfully!\n");

    // Initialize resource manager
    g_resourceManager = std::make_unique<WGacResourceManager>();
    SetWGacResourceManager(g_resourceManager.get());

    // Initialize object provider
    g_objectProvider = std::make_unique<WGacObjectProvider>();
    SetWGacObjectProvider(g_objectProvider.get());

    // Initialize cursor
    g_cursor = std::make_unique<WGacCursor>(&display);
    if (!g_cursor->Initialize()) {
        fprintf(stderr, "Warning: Failed to initialize cursor\n");
    }

    // Create window
    printf("\nCreating window...\n");
    WGacWindow window(&display);

    WindowConfig config;
    config.title = "wGac Input Test";
    config.width = 600;
    config.height = 400;
    config.min_width = 400;
    config.min_height = 300;

    if (!window.Create(config)) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }
    printf("Window created successfully!\n");

    // Create render target for this window
    g_objectProvider->CreateRenderTarget(&window);

    // Initialize demo elements
    InitElements();

    // Set up custom drawing
    window.GetView()->SetDrawCallback([&window](cairo_t* cr, int width, int height) {
        (void)cr; (void)width; (void)height;
        RenderDemo(&window);
    });

    // Set up close callback
    bool should_close = false;
    window.SetCloseCallback([&should_close]() {
        printf("Close requested\n");
        should_close = true;
    });

    // Set up resize callback
    window.SetResizeCallback([&window](int32_t w, int32_t h) {
        printf("Window resized to %dx%d\n", w, h);
        window.Invalidate();
    });

    // Set up input callbacks
    window.SetMouseMoveCallback([&window, &display](const MouseEventInfo& info) {
        g_mouseX = info.x;
        g_mouseY = info.y;
        g_mouseLeft = info.left;
        g_mouseRight = info.right;
        g_mouseMiddle = info.middle;

        // Update button hover states
        bool wasBtn1Hover = g_button1Hover;
        bool wasBtn2Hover = g_button2Hover;
        g_button1Hover = PointInRect(info.x, info.y, g_button1Rect);
        g_button2Hover = PointInRect(info.x, info.y, g_button2Rect);

        // Change cursor based on hover
        WaylandSeat* seat = display.GetWaylandSeat();
        if (seat && g_cursor) {
            if (g_button1Hover || g_button2Hover) {
                g_cursor->SetCursor(seat, CursorType::Hand);
            } else {
                g_cursor->SetCursor(seat, CursorType::Arrow);
            }
        }

        if (wasBtn1Hover != g_button1Hover || wasBtn2Hover != g_button2Hover) {
            window.Invalidate();
        }
    });

    window.SetMouseButtonCallback([&window](const MouseEventInfo& info, bool pressed) {
        g_mouseX = info.x;
        g_mouseY = info.y;
        g_mouseLeft = info.left;
        g_mouseRight = info.right;
        g_mouseMiddle = info.middle;

        if (pressed) {
            if (PointInRect(info.x, info.y, g_button1Rect)) {
                printf("Button 1 clicked!\n");
            } else if (PointInRect(info.x, info.y, g_button2Rect)) {
                printf("Button 2 clicked!\n");
            }
        }

        window.Invalidate();
    });

    window.SetMouseScrollCallback([&window](const ScrollEventInfo& info) {
        printf("Scroll: deltaX=%.2f deltaY=%.2f\n", info.deltaX, info.deltaY);
        (void)info;
    });

    window.SetMouseEnterCallback([&display, &window](int32_t x, int32_t y) {
        printf("Mouse entered at (%d, %d)\n", x, y);
        g_mouseX = x;
        g_mouseY = y;

        // Set default cursor
        WaylandSeat* seat = display.GetWaylandSeat();
        if (seat && g_cursor) {
            g_cursor->SetCursor(seat, CursorType::Arrow);
        }

        window.Invalidate();
    });

    window.SetMouseLeaveCallback([&window]() {
        printf("Mouse left\n");
        g_button1Hover = false;
        g_button2Hover = false;
        window.Invalidate();
    });

    window.SetKeyboardCallback([&window, &should_close](const KeyEventInfo& info) {
        if (info.state == KeyState::Pressed) {
            // Build key name
            if (!info.text.empty()) {
                g_lastKey = info.text + " (keycode: " + std::to_string(info.keycode) + ")";
            } else {
                g_lastKey = "keycode: " + std::to_string(info.keycode);
            }

            // Build modifiers string
            g_modifiers = "";
            if (info.ctrl) g_modifiers += "Ctrl ";
            if (info.shift) g_modifiers += "Shift ";
            if (info.alt) g_modifiers += "Alt ";
            if (info.capsLock) g_modifiers += "CapsLock ";

            printf("Key pressed: %s (keysym: 0x%x)\n", g_lastKey.c_str(), info.keysym);

            // ESC to exit
            if (info.keysym == 0xff1b) { // XKB_KEY_Escape
                should_close = true;
            }

            window.Invalidate();
        }
    });

    window.SetFocusCallback([&window](bool focused) {
        g_focused = focused;
        printf("Focus: %s\n", focused ? "gained" : "lost");
        window.Invalidate();
    });

    // Show window
    window.Show();
    printf("Window shown. Running event loop...\n");
    printf("Move mouse, click buttons, press keys. Press ESC to exit.\n\n");

    // Main event loop
    while (!should_close && !window.IsClosed()) {
        if (display.Dispatch() < 0) {
            fprintf(stderr, "Wayland dispatch failed\n");
            break;
        }
    }

    printf("\nCleaning up...\n");

    // Cleanup
    g_cursor.reset();
    g_objectProvider->DestroyRenderTarget(&window);
    window.Destroy();
    display.Disconnect();

    printf("Done!\n");
    return 0;
}
