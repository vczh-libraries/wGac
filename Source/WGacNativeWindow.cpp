#include "WGacNativeWindow.h"
#include "WGacGacView.h"
#include <cstring>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    const xdg_surface_listener xdg_surface_listener_ = {
        .configure = WGacNativeWindow::xdg_surface_configure,
    };

    const xdg_toplevel_listener xdg_toplevel_listener_ = {
        .configure = WGacNativeWindow::xdg_toplevel_configure,
        .close = WGacNativeWindow::xdg_toplevel_close,
        .configure_bounds = WGacNativeWindow::xdg_toplevel_configure_bounds,
        .wm_capabilities = WGacNativeWindow::xdg_toplevel_wm_capabilities,
    };

    const xdg_popup_listener xdg_popup_listener_ = {
        .configure = WGacNativeWindow::xdg_popup_configure,
        .popup_done = WGacNativeWindow::xdg_popup_done,
    };

    const wl_callback_listener frame_listener = {
        .done = WGacNativeWindow::frame_done,
    };

    const wl_callback_listener popup_sync_listener = {
        .done = WGacNativeWindow::popup_sync_done,
    };
}

WGacNativeWindow::WGacNativeWindow(WaylandDisplay* _display, INativeWindow::WindowMode _mode)
    : display(_display)
    , surface(nullptr)
    , xdgSurface(nullptr)
    , toplevel(nullptr)
    , popup(nullptr)
    , decoration(nullptr)
    , frameCallback(nullptr)
    , popupSyncCallback(nullptr)
    , bufferPool(nullptr)
    , view(nullptr)
    , parentWindow(nullptr)
    , cursor(nullptr)
    , graphicsHandler(nullptr)
    , mode(_mode)
    , currentWidth(800)
    , currentHeight(600)
    , posX(0)
    , posY(0)
    , currentBufferScale(1)
    , configured(false)
    , visible(false)
    , closed(false)
    , pendingFrame(false)
    , hasFirstFrame(false)
    , customFrameMode(true)
    , enabled(true)
    , capturing(false)
    , border(true)
    , sizeBox(true)
    , topMost(false)
    , titleBar(true)
    , iconVisible(true)
    , maximizedBox(true)
    , minimizedBox(true)
    , sizeState(WindowSizeState::Restored)
    , caretPoint(0, 0)
    , textInputEnabled(false)
    , hasKeyboardFocus(false)
{
}

WGacNativeWindow::~WGacNativeWindow()
{
    Destroy();
}

bool WGacNativeWindow::Create()
{
    if (surface) return false;

    surface = wl_compositor_create_surface(display->GetCompositor());
    if (!surface) return false;

    // For popup/tooltip/menu windows, delay xdg_surface creation until Show()
    // because we need parent and position information first
    bool isPopup = (mode == WindowMode::Popup || mode == WindowMode::Tooltip || mode == WindowMode::Menu);

    if (!isPopup) {
        // Create xdg_surface and toplevel immediately for normal windows
        if (!CreateXdgSurface()) {
            wl_surface_destroy(surface);
            surface = nullptr;
            return false;
        }
    }

    // Get scale factor from display
    int32_t scale = display->GetOutputScale();
    if (scale < 1) scale = 1;
    currentBufferScale = scale;

    bufferPool = new WaylandBufferPool(display->GetShm());
    // Create buffer at scaled size
    if (!bufferPool->Resize(currentWidth * scale, currentHeight * scale)) {
        Destroy();
        return false;
    }

    // Set buffer scale on surface
    wl_surface_set_buffer_scale(surface, scale);

    view = new WGacView(this, bufferPool);
    display->RegisterWindow(this);

    // Only commit for normal windows; popups commit in Show()
    if (!isPopup) {
        wl_surface_commit(surface);
    }

    return true;
}

bool WGacNativeWindow::CreateXdgSurface()
{
    if (xdgSurface) return true;  // Already created

    xdgSurface = xdg_wm_base_get_xdg_surface(display->GetXdgWmBase(), surface);
    if (!xdgSurface) {
        return false;
    }
    xdg_surface_add_listener(xdgSurface, &xdg_surface_listener_, this);

    bool isPopup = (mode == WindowMode::Popup || mode == WindowMode::Tooltip || mode == WindowMode::Menu);

    if (isPopup && parentWindow && parentWindow->xdgSurface) {
        // Create popup window
        xdg_positioner* positioner = xdg_wm_base_create_positioner(display->GetXdgWmBase());
        if (!positioner) {
            xdg_surface_destroy(xdgSurface);
            xdgSurface = nullptr;
            return false;
        }

        // Calculate position relative to parent
        int32_t relativeX, relativeY;
        bool parentIsPopup = (parentWindow->mode == WindowMode::Popup ||
                              parentWindow->mode == WindowMode::Tooltip ||
                              parentWindow->mode == WindowMode::Menu);
        if (parentIsPopup) {
            // Parent is a popup - GacUI gives coordinates relative to the popup
            relativeX = posX;
            relativeY = posY;
        } else {
            // Parent is main window - GacUI gives screen coordinates
            relativeX = posX - parentWindow->posX;
            relativeY = posY - parentWindow->posY;
        }

        // Set popup size
        xdg_positioner_set_size(positioner, currentWidth > 0 ? currentWidth : 100, currentHeight > 0 ? currentHeight : 100);
        // Set anchor rectangle (position relative to parent)
        xdg_positioner_set_anchor_rect(positioner, relativeX, relativeY, 1, 1);
        // Anchor to top-left of the anchor rect
        xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
        // Gravity: popup expands to bottom-right
        xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
        // Allow compositor to adjust position if popup would be outside screen
        xdg_positioner_set_constraint_adjustment(positioner,
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y |
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X |
            XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y);

        popup = xdg_surface_get_popup(xdgSurface, parentWindow->xdgSurface, positioner);
        xdg_positioner_destroy(positioner);

        if (!popup) {
            xdg_surface_destroy(xdgSurface);
            xdgSurface = nullptr;
            return false;
        }
        xdg_popup_add_listener(popup, &xdg_popup_listener_, this);

        // Grab the popup so it receives input events and doesn't dismiss immediately
        WaylandSeat* seat = display->GetWaylandSeat();
        if (seat && seat->GetSeat()) {
            xdg_popup_grab(popup, seat->GetSeat(), seat->GetLastPointerSerial());
        }
    } else {
        // Create toplevel window
        toplevel = xdg_surface_get_toplevel(xdgSurface);
        if (!toplevel) {
            xdg_surface_destroy(xdgSurface);
            xdgSurface = nullptr;
            return false;
        }
        xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener_, this);

        if (parentWindow && parentWindow->toplevel) {
            xdg_toplevel_set_parent(toplevel, parentWindow->toplevel);
        }

        xdg_toplevel_set_title(toplevel, "GacUI Window");
        xdg_toplevel_set_app_id(toplevel, "gacui");

        if (display->GetDecorationManager()) {
            decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
                display->GetDecorationManager(), toplevel);
            if (decoration) {
                zxdg_toplevel_decoration_v1_set_mode(
                    decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
            }
        }
    }

    return true;
}

void WGacNativeWindow::Destroy()
{
    // Clear any seat focus references to this window before destroying
    // This prevents dangling pointer issues when compositor sends events
    if (display && display->GetWaylandSeat()) {
        display->GetWaylandSeat()->ClearFocusFor(this);
    }

    if (display && surface) {
        display->UnregisterWindow(this);
    }

    if (frameCallback) {
        wl_callback_destroy(frameCallback);
        frameCallback = nullptr;
    }

    delete view;
    view = nullptr;

    delete bufferPool;
    bufferPool = nullptr;

    if (decoration) {
        zxdg_toplevel_decoration_v1_destroy(decoration);
        decoration = nullptr;
    }

    if (popup) {
        xdg_popup_destroy(popup);
        popup = nullptr;
    }

    if (toplevel) {
        xdg_toplevel_destroy(toplevel);
        toplevel = nullptr;
    }

    if (xdgSurface) {
        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
    }

    if (surface) {
        wl_surface_destroy(surface);
        surface = nullptr;
    }

    configured = false;
    visible = false;
    closed = false;
    hasFirstFrame = false;
}

void WGacNativeWindow::SetGraphicsHandler(Interface* handler)
{
    graphicsHandler = handler;
}

Interface* WGacNativeWindow::GetGraphicsHandler() const
{
    return graphicsHandler;
}

void WGacNativeWindow::CommitBuffer()
{
    // Don't commit buffer before configure event (Wayland protocol requirement)
    if (!configured) {
        return;
    }
    if (view && view->GetCurrentBuffer() && surface) {
        auto* buffer = view->GetCurrentBuffer();
        buffer->Attach(surface, 0, 0);
        buffer->DamageAll(surface);
        wl_surface_commit(surface);
    }
}

// Static Wayland callbacks
void WGacNativeWindow::xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    bool firstConfigure = !self->configured;
    self->configured = true;

    if (self->currentWidth > 0 && self->currentHeight > 0) {
        // Get scale factor for buffer sizing
        int32_t scale = self->display->GetOutputScale();
        if (scale < 1) scale = 1;
        int32_t scaledWidth = self->currentWidth * scale;
        int32_t scaledHeight = self->currentHeight * scale;

        if (self->bufferPool->GetWidth() != static_cast<uint32_t>(scaledWidth) ||
            self->bufferPool->GetHeight() != static_cast<uint32_t>(scaledHeight)) {
            self->bufferPool->Resize(scaledWidth, scaledHeight);
            // Only update buffer scale if it actually changed to avoid configure loops
            if (self->currentBufferScale != scale) {
                self->currentBufferScale = scale;
                wl_surface_set_buffer_scale(self->surface, scale);
            }
            for (vint i = 0; i < self->listeners.Count(); i++) {
                self->listeners[i]->Moved();
            }
        }
    }

    if (firstConfigure && self->visible) {
        // If a frame was already requested before configure (e.g., from Opened callback),
        // the Paint() would have been skipped because configured was false.
        // Reset the state and request a new frame now that we're configured.
        if (self->pendingFrame) {
            self->pendingFrame = false;
            self->hasFirstFrame = false;
        }
        self->RequestFrame();
    }
}

void WGacNativeWindow::xdg_toplevel_configure(void* data, xdg_toplevel* /*toplevel*/,
                                               int32_t width, int32_t height, wl_array* states)
{
    auto* self = static_cast<WGacNativeWindow*>(data);

    self->sizeState = WindowSizeState::Restored;
    auto* statePtr = static_cast<uint32_t*>(states->data);
    size_t numStates = states->size / sizeof(uint32_t);
    for (size_t i = 0; i < numStates; ++i) {
        switch (statePtr[i]) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:
                self->sizeState = WindowSizeState::Maximized;
                break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:
                self->sizeState = WindowSizeState::Maximized;
                break;
            default:
                break;
        }
    }

    if (width > 0 && height > 0) {
        self->currentWidth = width;
        self->currentHeight = height;
    }
}

void WGacNativeWindow::xdg_toplevel_close(void* data, xdg_toplevel* /*toplevel*/)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    self->closed = true;
    for (vint i = 0; i < self->listeners.Count(); i++) {
        self->listeners[i]->Closed();
    }
}

void WGacNativeWindow::xdg_toplevel_configure_bounds(void* /*data*/, xdg_toplevel* /*toplevel*/,
                                                      int32_t /*width*/, int32_t /*height*/)
{
}

void WGacNativeWindow::xdg_toplevel_wm_capabilities(void* /*data*/, xdg_toplevel* /*toplevel*/,
                                                     wl_array* /*capabilities*/)
{
}

void WGacNativeWindow::xdg_popup_configure(void* data, xdg_popup* /*popup*/,
                                            int32_t x, int32_t y, int32_t width, int32_t height)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    // Note: x/y are relative to parent, but we keep posX/posY in screen coordinates
    // for GacUI compatibility. Only update size if compositor adjusted it.
    (void)x;
    (void)y;
    if (width > 0 && height > 0) {
        self->currentWidth = width;
        self->currentHeight = height;
    }
}

void WGacNativeWindow::xdg_popup_done(void* data, xdg_popup* /*popup*/)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    // Popup was dismissed (e.g., user clicked outside)

    // Save parent reference before clearing focus
    auto* parent = self->parentWindow;

    // Clear seat focus references before destroying surfaces
    // Pass parent to restore pointer focus (Wayland doesn't send pointer_enter
    // when popup is dismissed and pointer is already over parent)
    if (self->display && self->display->GetWaylandSeat()) {
        self->display->GetWaylandSeat()->ClearFocusFor(self, parent);
    }

    // Reset state so next Show() works correctly
    self->hasKeyboardFocus = false;
    self->capturing = false;

    // Clean up xdg resources so popup can be shown again
    if (self->frameCallback) {
        wl_callback_destroy(self->frameCallback);
        self->frameCallback = nullptr;
    }
    self->pendingFrame = false;
    if (self->popup) {
        xdg_popup_destroy(self->popup);
        self->popup = nullptr;
    }
    if (self->xdgSurface) {
        xdg_surface_destroy(self->xdgSurface);
        self->xdgSurface = nullptr;
    }
    // Unmap the surface by attaching null buffer so it can be remapped later
    if (self->surface) {
        wl_surface_attach(self->surface, nullptr, 0, 0);
        wl_surface_commit(self->surface);
    }
    self->configured = false;
    self->hasFirstFrame = false;
    self->visible = false;

    // Use parent reference saved earlier
    bool parentVisible = parent ? parent->visible : false;

    // Follow Windows event sequence: BeforeClosing -> AfterClosing -> Closed
    bool cancel = false;
    for (vint i = 0; i < self->listeners.Count(); i++) {
        self->listeners[i]->BeforeClosing(cancel);
    }

    // AfterClosing triggers WindowReadyToClose which re-enables owner window in modal dialogs
    if (!cancel) {
        for (vint i = 0; i < self->listeners.Count(); i++) {
            self->listeners[i]->AfterClosing();
        }
    }

    // Explicitly restore focus to parent window BEFORE Closed()
    if (parent && parentVisible) {
        parent->OnFocusChanged(true);
    }

    // Notify listeners that window is closed
    for (vint i = 0; i < self->listeners.Count(); i++) {
        self->listeners[i]->Closed();
    }
}

void WGacNativeWindow::frame_done(void* data, wl_callback* callback, uint32_t /*time*/)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    wl_callback_destroy(callback);
    self->OnFrame();
}

void WGacNativeWindow::RequestFrame()
{
    if (pendingFrame || !surface) return;

    pendingFrame = true;

    if (!hasFirstFrame) {
        hasFirstFrame = true;
        // For first frame, trigger Paint on listeners to initialize GacUI rendering
        for (vint i = 0; i < listeners.Count(); i++) {
            listeners[i]->Paint();
        }
        // Buffer is committed in StopRendering()
    }

    frameCallback = wl_surface_frame(surface);
    wl_callback_add_listener(frameCallback, &frame_listener, this);
    wl_surface_commit(surface);
}

void WGacNativeWindow::OnFrame()
{
    pendingFrame = false;
    frameCallback = nullptr;

    if (!visible || !configured) return;

    // Trigger GacUI's paint pipeline through listeners
    // Buffer is committed in StopRendering()
    for (vint i = 0; i < listeners.Count(); i++) {
        listeners[i]->Paint();
    }

    // Request next frame for continuous rendering
    RequestFrame();
}

// INativeWindow implementation
bool WGacNativeWindow::IsActivelyRefreshing() { return true; }
NativeSize WGacNativeWindow::GetRenderingOffset() { return NativeSize(0, 0); }
bool WGacNativeWindow::IsRenderingAsActivated() {
    return IsActivated();
}

Point WGacNativeWindow::Convert(NativePoint value) { return Point(value.x.value, value.y.value); }
NativePoint WGacNativeWindow::Convert(Point value) { return NativePoint(value.x, value.y); }
Size WGacNativeWindow::Convert(NativeSize value) { return Size(value.x.value, value.y.value); }
NativeSize WGacNativeWindow::Convert(Size value) { return NativeSize(value.x, value.y); }
Margin WGacNativeWindow::Convert(NativeMargin value) { return Margin(value.left.value, value.top.value, value.right.value, value.bottom.value); }
NativeMargin WGacNativeWindow::Convert(Margin value) { return NativeMargin(value.left, value.top, value.right, value.bottom); }

NativeRect WGacNativeWindow::GetBounds() { return NativeRect(posX, posY, posX + currentWidth, posY + currentHeight); }
void WGacNativeWindow::SetBounds(const NativeRect& bounds) {
    posX = bounds.x1.value;
    posY = bounds.y1.value;
    int32_t newWidth = bounds.Width().value;
    int32_t newHeight = bounds.Height().value;

    if (newWidth != currentWidth || newHeight != currentHeight) {
        currentWidth = newWidth;
        currentHeight = newHeight;
        if (bufferPool) {
            int32_t scale = display->GetOutputScale();
            if (scale < 1) scale = 1;
            bufferPool->Resize(currentWidth * scale, currentHeight * scale);
        }
    }
    for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Moved(); }
}
NativeSize WGacNativeWindow::GetClientSize() {
    // Ensure we never return zero size - this causes clipping issues
    int32_t w = currentWidth > 0 ? currentWidth : 1;
    int32_t h = currentHeight > 0 ? currentHeight : 1;
    return NativeSize(w, h);
}
void WGacNativeWindow::SetClientSize(NativeSize size) {
    currentWidth = size.x.value;
    currentHeight = size.y.value;
    if (bufferPool) bufferPool->Resize(currentWidth, currentHeight);
    for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Moved(); }
}
NativeRect WGacNativeWindow::GetClientBoundsInScreen() {
    // For popup windows, return bounds relative to self (0,0) since rendering is in window-local coords
    bool isPopup = (mode == WindowMode::Popup || mode == WindowMode::Tooltip || mode == WindowMode::Menu);
    if (isPopup) {
        return NativeRect(0, 0, currentWidth, currentHeight);
    }
    return NativeRect(posX, posY, posX + currentWidth, posY + currentHeight);
}

void WGacNativeWindow::SuggestMinClientSize(NativeSize size) {
    // Wayland doesn't have direct min size API, but we can store it for reference
    // The actual enforcement would need to be done in resize handling
}

WString WGacNativeWindow::GetTitle() { return title; }
void WGacNativeWindow::SetTitle(const WString& _title) {
    title = _title;
    if (toplevel) {
        AString aTitle = wtoa(title);
        xdg_toplevel_set_title(toplevel, aTitle.Buffer());
    }
}

INativeCursor* WGacNativeWindow::GetWindowCursor() { return cursor; }
void WGacNativeWindow::SetWindowCursor(INativeCursor* _cursor) { cursor = _cursor; }
NativePoint WGacNativeWindow::GetCaretPoint() { return caretPoint; }
void WGacNativeWindow::SetCaretPoint(NativePoint point) {
    caretPoint = point;
    // TODO: Text input is temporarily disabled to debug focus issues
    // Will re-enable once button click handling is fixed
}

INativeWindow* WGacNativeWindow::GetParent() { return parentWindow; }
void WGacNativeWindow::SetParent(INativeWindow* parent) {
    parentWindow = dynamic_cast<WGacNativeWindow*>(parent);
    if (toplevel) {
        xdg_toplevel_set_parent(toplevel, parentWindow ? parentWindow->toplevel : nullptr);
    }
}
INativeWindow::WindowMode WGacNativeWindow::GetWindowMode() { return mode; }
void WGacNativeWindow::EnableCustomFrameMode() { customFrameMode = true; }
void WGacNativeWindow::DisableCustomFrameMode() { customFrameMode = false; }
bool WGacNativeWindow::IsCustomFrameModeEnabled() { return customFrameMode; }
NativeMargin WGacNativeWindow::GetCustomFramePadding() { return sizeBox || titleBar ? NativeMargin(5, 5, 5, 5) : NativeMargin(0, 0, 0, 0); }

Ptr<GuiImageData> WGacNativeWindow::GetIcon() { return nullptr; }
void WGacNativeWindow::SetIcon(Ptr<GuiImageData> icon) {}

INativeWindow::WindowSizeState WGacNativeWindow::GetSizeState() { return sizeState; }

void WGacNativeWindow::Show() {
    visible = true;

    bool isPopup = (mode == WindowMode::Popup || mode == WindowMode::Tooltip || mode == WindowMode::Menu);

    // Recreate xdg_surface if it was destroyed by Hide()
    if (!xdgSurface) {
        if (isPopup && !popupSyncCallback) {
            // For popup windows, delay creation using sync callback to let SetBounds complete
            popupSyncCallback = wl_display_sync(display->GetDisplay());
            wl_callback_add_listener(popupSyncCallback, &popup_sync_listener, this);
            // Don't call Opened() yet - wait for popup to be created
            return;
        } else if (!isPopup) {
            // For normal windows, recreate the xdg_surface and toplevel
            if (!CreateXdgSurface()) {
                return;
            }
            wl_surface_commit(surface);
        }
    }

    if (configured) RequestFrame();
    for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Opened(); }
}

void WGacNativeWindow::popup_sync_done(void* data, wl_callback* callback, uint32_t /*time*/)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    wl_callback_destroy(callback);
    self->popupSyncCallback = nullptr;

    if (!self->visible || self->xdgSurface) return;

    if (!self->CreateXdgSurface()) {
        return;
    }
    // Detach any existing buffer before first commit (Wayland protocol requirement)
    wl_surface_attach(self->surface, nullptr, 0, 0);
    wl_surface_commit(self->surface);

    if (self->configured) self->RequestFrame();
    for (vint i = 0; i < self->listeners.Count(); i++) { self->listeners[i]->Opened(); }
}
void WGacNativeWindow::ShowDeactivated() { Show(); }
void WGacNativeWindow::ShowRestored() { if (toplevel) xdg_toplevel_unset_maximized(toplevel); }
void WGacNativeWindow::ShowMaximized() { if (toplevel) xdg_toplevel_set_maximized(toplevel); }
void WGacNativeWindow::ShowMinimized() { if (toplevel) xdg_toplevel_set_minimized(toplevel); }
void WGacNativeWindow::Hide(bool closeWindow) {
    // If already hidden or never shown, just update state
    if (!visible && !xdgSurface) {
        if (closeWindow) {
            for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Closed(); }
        }
        return;
    }

    visible = false;

    // Clear seat focus references before destroying surfaces
    // This prevents dangling pointer issues when compositor sends events
    // Pass parent to restore pointer focus (Wayland doesn't send pointer_enter
    // when popup is dismissed and pointer is already over parent)
    if (display && display->GetWaylandSeat()) {
        display->GetWaylandSeat()->ClearFocusFor(this, parentWindow);
    }

    // Cancel any pending frame callback
    if (frameCallback) {
        wl_callback_destroy(frameCallback);
        frameCallback = nullptr;
    }
    pendingFrame = false;

    bool isPopup = (mode == WindowMode::Popup || mode == WindowMode::Tooltip || mode == WindowMode::Menu);
    if (isPopup) {
        // Reset state so next Show() works correctly
        hasKeyboardFocus = false;
        capturing = false;

        // For popup windows, destroy the xdg_popup and xdg_surface to properly close
        if (popupSyncCallback) {
            wl_callback_destroy(popupSyncCallback);
            popupSyncCallback = nullptr;
        }
        if (popup) {
            xdg_popup_destroy(popup);
            popup = nullptr;
        }
        if (xdgSurface) {
            xdg_surface_destroy(xdgSurface);
            xdgSurface = nullptr;
        }
        configured = false;
        hasFirstFrame = false;
        // Unmap the surface by attaching null buffer
        if (surface) {
            wl_surface_attach(surface, nullptr, 0, 0);
            wl_surface_commit(surface);
        }
        // Flush to ensure compositor processes the surface destruction
        display->Flush();
    } else if (xdgSurface) {
        // For normal windows, destroy xdg_toplevel to hide
        // This is necessary because Wayland doesn't have a "hide" concept for toplevels
        if (decoration) {
            zxdg_toplevel_decoration_v1_destroy(decoration);
            decoration = nullptr;
        }
        if (toplevel) {
            xdg_toplevel_destroy(toplevel);
            toplevel = nullptr;
        }
        if (xdgSurface) {
            xdg_surface_destroy(xdgSurface);
            xdgSurface = nullptr;
        }
        configured = false;
        hasFirstFrame = false;
        // Unmap the surface by attaching null buffer
        if (surface) {
            wl_surface_attach(surface, nullptr, 0, 0);
            wl_surface_commit(surface);
        }
        // Flush to ensure compositor processes the surface destruction
        display->Flush();
    }

    if (closeWindow) {
        // Save parent reference before closing events which may affect window state
        auto* parent = parentWindow;
        bool parentVisible = parent ? parent->visible : false;

        // Follow Windows event sequence: BeforeClosing -> AfterClosing -> Closed
        // BeforeClosing allows cancellation (not handled here, but required for modal dialogs)
        bool cancel = false;
        for (vint i = 0; i < listeners.Count(); i++) {
            listeners[i]->BeforeClosing(cancel);
        }

        // AfterClosing triggers WindowReadyToClose which re-enables owner window in modal dialogs
        if (!cancel) {
            for (vint i = 0; i < listeners.Count(); i++) {
                listeners[i]->AfterClosing();
            }
        }

        // Explicitly restore focus to parent window BEFORE Closed()
        // This ensures GacUI framework sees correct focus state during cleanup
        if (parent && parentVisible) {
            parent->OnFocusChanged(true);
        }

        // Notify listeners that window is closed
        for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Closed(); }
    }
}
bool WGacNativeWindow::IsVisible() { return visible; }

void WGacNativeWindow::Enable() { enabled = true; for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Enabled(); } }
void WGacNativeWindow::Disable() { enabled = false; for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Disabled(); } }
bool WGacNativeWindow::IsEnabled() { return enabled; }
void WGacNativeWindow::SetActivate() { Show(); }
bool WGacNativeWindow::IsActivated() {
    return hasKeyboardFocus;
}

void WGacNativeWindow::ShowInTaskBar() {}
void WGacNativeWindow::HideInTaskBar() {}
bool WGacNativeWindow::IsAppearedInTaskBar() { return true; }
void WGacNativeWindow::EnableActivate() {}
void WGacNativeWindow::DisableActivate() {}
bool WGacNativeWindow::IsEnabledActivate() { return true; }

bool WGacNativeWindow::RequireCapture() {
    capturing = true;
    return true;
}
bool WGacNativeWindow::ReleaseCapture() {
    capturing = false;
    return true;
}
bool WGacNativeWindow::IsCapturing() { return capturing; }

bool WGacNativeWindow::GetMaximizedBox() { return maximizedBox; }
void WGacNativeWindow::SetMaximizedBox(bool visible) { maximizedBox = visible; }
bool WGacNativeWindow::GetMinimizedBox() { return minimizedBox; }
void WGacNativeWindow::SetMinimizedBox(bool visible) { minimizedBox = visible; }
bool WGacNativeWindow::GetBorder() { return border; }
void WGacNativeWindow::SetBorder(bool visible) { border = visible; }
bool WGacNativeWindow::GetSizeBox() { return sizeBox; }
void WGacNativeWindow::SetSizeBox(bool visible) { sizeBox = visible; }
bool WGacNativeWindow::GetIconVisible() { return iconVisible; }
void WGacNativeWindow::SetIconVisible(bool visible) { iconVisible = visible; }
bool WGacNativeWindow::GetTitleBar() { return titleBar; }
void WGacNativeWindow::SetTitleBar(bool visible) { titleBar = visible; }
bool WGacNativeWindow::GetTopMost() { return topMost; }
void WGacNativeWindow::SetTopMost(bool top) { topMost = top; }

void WGacNativeWindow::SupressAlt() {}
bool WGacNativeWindow::InstallListener(INativeWindowListener* listener) {
    if (listeners.Contains(listener)) return false;
    listeners.Add(listener);
    return true;
}
bool WGacNativeWindow::UninstallListener(INativeWindowListener* listener) {
    if (listeners.Contains(listener)) {
        listeners.Remove(listener);
        return true;
    }
    return false;
}
void WGacNativeWindow::RedrawContent() { RequestFrame(); }

// IWaylandWindow input event handlers
void WGacNativeWindow::OnMouseEnter(int32_t x, int32_t y) {
    for (auto listener : listeners) {
        listener->MouseEntered();
    }
    // Also send MouseMoving to update control hover states
    // GacUI's MouseEntered() is empty, it relies on MouseMoving to set mouseEnterCompositions
    NativeWindowMouseInfo nativeInfo = {};
    nativeInfo.x = x;
    nativeInfo.y = y;
    for (auto listener : listeners) {
        listener->MouseMoving(nativeInfo);
    }
}

void WGacNativeWindow::OnMouseLeave() {
    for (auto listener : listeners) {
        listener->MouseLeaved();
    }
}

void WGacNativeWindow::OnMouseMove(const MouseEventInfo& info) {
    NativeWindowMouseInfo nativeInfo = {};  // Zero-initialize all fields
    nativeInfo.x = info.x;
    nativeInfo.y = info.y;
    nativeInfo.left = info.left;
    nativeInfo.middle = info.middle;
    nativeInfo.right = info.right;
    nativeInfo.ctrl = info.ctrl;
    nativeInfo.shift = info.shift;
    nativeInfo.wheel = 0;
    nativeInfo.nonClient = false;
    for (auto listener : listeners) {
        listener->MouseMoving(nativeInfo);
    }
}

void WGacNativeWindow::OnMouseButton(const MouseEventInfo& info, bool pressed) {
    NativeWindowMouseInfo nativeInfo = {};  // Zero-initialize all fields
    nativeInfo.x = info.x;
    nativeInfo.y = info.y;
    nativeInfo.ctrl = info.ctrl;
    nativeInfo.shift = info.shift;
    nativeInfo.wheel = 0;
    nativeInfo.nonClient = false;

    // Set button states
    // For press: include the button being pressed (state updated before callback)
    // For release: use current state (button already released, so flag is false)
    // This matches Windows behavior where WM_LBUTTONUP has MK_LBUTTON=0
    if (pressed) {
        // Button state was already updated to include this button
        nativeInfo.left = info.left;
        nativeInfo.middle = info.middle;
        nativeInfo.right = info.right;
    } else {
        // Button state was already updated to exclude this button
        // This allows MouseUncapture to detect when all buttons are released
        nativeInfo.left = info.left;
        nativeInfo.middle = info.middle;
        nativeInfo.right = info.right;
    }

    for (auto listener : listeners) {
        if (pressed) {
            if (info.button == static_cast<uint32_t>(MouseButton::Left)) {
                listener->LeftButtonDown(nativeInfo);
            } else if (info.button == static_cast<uint32_t>(MouseButton::Right)) {
                listener->RightButtonDown(nativeInfo);
            } else if (info.button == static_cast<uint32_t>(MouseButton::Middle)) {
                listener->MiddleButtonDown(nativeInfo);
            }
        } else {
            if (info.button == static_cast<uint32_t>(MouseButton::Left)) {
                listener->LeftButtonUp(nativeInfo);
            } else if (info.button == static_cast<uint32_t>(MouseButton::Right)) {
                listener->RightButtonUp(nativeInfo);
            } else if (info.button == static_cast<uint32_t>(MouseButton::Middle)) {
                listener->MiddleButtonUp(nativeInfo);
            }
        }
    }
}

void WGacNativeWindow::OnMouseScroll(const ScrollEventInfo& info) {
    NativeWindowMouseInfo nativeInfo = {};  // Zero-initialize all fields
    nativeInfo.x = info.x;
    nativeInfo.y = info.y;
    nativeInfo.ctrl = info.ctrl;
    nativeInfo.shift = info.shift;
    nativeInfo.left = false;
    nativeInfo.middle = false;
    nativeInfo.right = false;
    nativeInfo.nonClient = false;

    for (auto listener : listeners) {
        if (info.deltaY != 0) {
            // Invert deltaY: Wayland positive = scroll down, GacUI positive = scroll up
            nativeInfo.wheel = static_cast<vint>(-info.deltaY * 120 / 15);
            listener->VerticalWheel(nativeInfo);
        }
        if (info.deltaX != 0) {
            nativeInfo.wheel = static_cast<vint>(info.deltaX * 120 / 15);
            listener->HorizontalWheel(nativeInfo);
        }
    }
}

// Convert XKB keysym to Windows virtual key code
static VKEY KeysymToVKey(uint32_t keysym) {
    // Letters A-Z
    if (keysym >= 'a' && keysym <= 'z') {
        return static_cast<VKEY>('A' + (keysym - 'a'));
    }
    if (keysym >= 'A' && keysym <= 'Z') {
        return static_cast<VKEY>(keysym);
    }
    // Numbers 0-9
    if (keysym >= '0' && keysym <= '9') {
        return static_cast<VKEY>(keysym);
    }
    // Function keys
    if (keysym >= 0xffbe && keysym <= 0xffc9) { // XKB_KEY_F1 to XKB_KEY_F12
        return static_cast<VKEY>(0x70 + (keysym - 0xffbe)); // VK_F1 = 0x70
    }
    // Special keys
    switch (keysym) {
        case 0xff08: return VKEY::KEY_BACK;      // XKB_KEY_BackSpace
        case 0xff09: return VKEY::KEY_TAB;       // XKB_KEY_Tab
        case 0xff0d: return VKEY::KEY_RETURN;    // XKB_KEY_Return
        case 0xff1b: return VKEY::KEY_ESCAPE;    // XKB_KEY_Escape
        case 0xff50: return VKEY::KEY_HOME;      // XKB_KEY_Home
        case 0xff51: return VKEY::KEY_LEFT;      // XKB_KEY_Left
        case 0xff52: return VKEY::KEY_UP;        // XKB_KEY_Up
        case 0xff53: return VKEY::KEY_RIGHT;     // XKB_KEY_Right
        case 0xff54: return VKEY::KEY_DOWN;      // XKB_KEY_Down
        case 0xff55: return VKEY::KEY_PRIOR;     // XKB_KEY_Page_Up
        case 0xff56: return VKEY::KEY_NEXT;      // XKB_KEY_Page_Down
        case 0xff57: return VKEY::KEY_END;       // XKB_KEY_End
        case 0xff63: return VKEY::KEY_INSERT;    // XKB_KEY_Insert
        case 0xffff: return VKEY::KEY_DELETE;    // XKB_KEY_Delete
        case 0x0020: return VKEY::KEY_SPACE;     // Space
        case 0xffe1:                              // XKB_KEY_Shift_L
        case 0xffe2: return VKEY::KEY_SHIFT;     // XKB_KEY_Shift_R
        case 0xffe3:                              // XKB_KEY_Control_L
        case 0xffe4: return VKEY::KEY_CONTROL;   // XKB_KEY_Control_R
        case 0xffe9:                              // XKB_KEY_Alt_L
        case 0xffea: return VKEY::KEY_MENU;      // XKB_KEY_Alt_R
        default: return VKEY::KEY_UNKNOWN;
    }
}

// Convert UTF-8 string to wchar_t characters and call Char() for each
static void SendUtf8AsChars(INativeWindowListener* listener, const std::string& utf8,
                            bool ctrl, bool shift, bool alt, bool capslock) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8.c_str());
    const unsigned char* end = p + utf8.size();

    while (p < end) {
        wchar_t codepoint = 0;
        unsigned char c = *p;

        if ((c & 0x80) == 0) {
            // ASCII (1 byte)
            codepoint = c;
            p += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte sequence
            if (p + 1 < end) {
                codepoint = ((c & 0x1F) << 6) | (p[1] & 0x3F);
                p += 2;
            } else break;
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte sequence
            if (p + 2 < end) {
                codepoint = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
                p += 3;
            } else break;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte sequence (surrogate pairs needed for wchar_t if sizeof(wchar_t)==2)
            if (p + 3 < end) {
                uint32_t cp = ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) |
                              ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
                p += 4;
                // On Linux wchar_t is typically 32-bit, so we can store directly
                codepoint = static_cast<wchar_t>(cp);
            } else break;
        } else {
            // Invalid UTF-8, skip byte
            p += 1;
            continue;
        }

        NativeWindowCharInfo charInfo;
        charInfo.code = codepoint;
        charInfo.ctrl = ctrl;
        charInfo.shift = shift;
        charInfo.alt = alt;
        charInfo.capslock = capslock;
        listener->Char(charInfo);
    }
}

void WGacNativeWindow::OnKeyEvent(const KeyEventInfo& info) {
    NativeWindowKeyInfo nativeInfo;
    nativeInfo.code = KeysymToVKey(info.keysym);
    nativeInfo.ctrl = info.ctrl;
    nativeInfo.shift = info.shift;
    nativeInfo.alt = info.alt;
    nativeInfo.capslock = info.capsLock;

    for (auto listener : listeners) {
        if (info.state == KeyState::Pressed) {
            listener->KeyDown(nativeInfo);
            // Send character events for printable text
            if (!info.text.empty() && !info.ctrl && !info.alt) {
                SendUtf8AsChars(listener, info.text, info.ctrl, info.shift, info.alt, info.capsLock);
            }
        } else if (info.state == KeyState::Released) {
            listener->KeyUp(nativeInfo);
        }
    }
}

void WGacNativeWindow::OnFocusChanged(bool focused) {
    // Avoid duplicate notifications
    if (hasKeyboardFocus == focused) {
        return;
    }
    hasKeyboardFocus = focused;

    for (auto listener : listeners) {
        if (focused) {
            listener->GotFocus();
            listener->RenderingAsActivated();
        } else {
            listener->LostFocus();
            listener->RenderingAsDeactivated();
        }
    }

    // Request redraw to update visual focus state (e.g., border color)
    if (visible && configured) {
        RequestFrame();
    }
}

void WGacNativeWindow::OnTextInputPreedit(const PreeditInfo& info) {
    // For now, we don't display preedit text inline.
    // A full implementation would need to render the preedit string
    // with underline at the current cursor position.
    // TODO: Implement preedit string display in text controls
    (void)info;
}

void WGacNativeWindow::OnTextInputCommit(const std::string& text) {
    // Convert UTF-8 to wchar_t and send as character events
    for (auto listener : listeners) {
        SendUtf8AsChars(listener, text, false, false, false, false);
    }
}

}
}
}
