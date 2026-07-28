#include "WGacNativeWindow.h"
#include "WGacController.h"
#include "WGacGacView.h"
#include "Services/WGacResourceService.h"
#include <cstring>
#include <stdexcept>

namespace vl {
namespace presentation {
namespace wayland {

namespace {
    const xdg_surface_listener xdg_surface_listener_ = {
        .configure = WGacNativeWindow::xdg_surface_configure,
    };

    libdecor_frame_interface libdecor_frame_interface_ = {
        .configure = WGacNativeWindow::libdecor_frame_configure,
        .close = WGacNativeWindow::libdecor_frame_close,
        .commit = WGacNativeWindow::libdecor_frame_commit,
        .dismiss_popup = WGacNativeWindow::libdecor_frame_dismiss_popup,
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
    , popup(nullptr)
    , libdecorFrame(nullptr)
    , frameCallback(nullptr)
    , popupSyncCallback(nullptr)
    , bufferPool(nullptr)
    , view(nullptr)
    , parentWindow(nullptr)
    , popupGrabParent(nullptr)
    , compositionCursor(nullptr)
    , borderOverrideCursor(nullptr)
    , graphicsHandler(nullptr)
    , mode(_mode)
    , currentWidth(800)
    , currentHeight(600)
    , lastFloatingWidth(800)
    , lastFloatingHeight(600)
    , minWidth(0)
    , minHeight(0)
    , posX(0)
    , posY(0)
    , currentBufferScale(1)
    , configured(false)
    , visible(false)
    , closed(false)
    , pendingFrame(false)
    , hasFirstFrame(false)
    , libdecorStateInitialized(false)
    , customFrameMode(false)
    , pressedCaptionButton(INativeWindowListener::NoDecision)
    , enabled(true)
    , capturing(false)
    , border(true)
    , sizeBox(true)
    , topMost(false)
    , titleBar(true)
    , iconVisible(false)
    , maximizedBox(true)
    , minimizedBox(true)
    , sizeState(WindowSizeState::Restored)
    , caretPoint(0, 0)
    , textInputEnabled(false)
    , hasKeyboardFocus(false)
    , popupActivationSerial(0)
{
}

WGacNativeWindow::~WGacNativeWindow()
{
    Destroy();
}

bool WGacNativeWindow::Create()
{
    if (surface || !display || !display->GetLibdecorContext()) return false;

    surface = wl_compositor_create_surface(display->GetCompositor());
    if (!surface) return false;

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
    if (auto resourceService = GetCursorResourceService())
    {
        compositionCursor = resourceService->GetDefaultSystemCursor();
    }

    // Popup/tooltip/menu roles are delayed until Show(), because they require
    // final parent and position information. Libdecor owns every normal role.
    if (!IsPopupMode() && !CreateLibdecorFrame()) {
        Destroy();
        return false;
    }

    return true;
}

bool WGacNativeWindow::IsPopupMode() const
{
    return mode == WindowMode::Popup ||
        mode == WindowMode::Tooltip ||
        mode == WindowMode::Menu;
}

xdg_surface* WGacNativeWindow::GetXdgSurface() const
{
    return libdecorFrame
        ? libdecor_frame_get_xdg_surface(libdecorFrame)
        : xdgSurface;
}

xdg_toplevel* WGacNativeWindow::GetXdgToplevel() const
{
    return libdecorFrame
        ? libdecor_frame_get_xdg_toplevel(libdecorFrame)
        : nullptr;
}

INativeWindowListener::HitTestResult WGacNativeWindow::PerformCustomFrameHitTest(
    int32_t x,
    int32_t y)
{
    if (mode != WindowMode::Normal || !customFrameMode)
    {
        return INativeWindowListener::NoDecision;
    }
    return PerformHitTest(From(listeners), NativePoint(x, y));
}

void WGacNativeWindow::ClearPressedCaptionButton()
{
    pressedCaptionButton = INativeWindowListener::NoDecision;
}

WGacResourceService* WGacNativeWindow::GetCursorResourceService()
{
    auto controller = GetWGacController();
    return controller
        ? dynamic_cast<WGacResourceService*>(controller->ResourceService())
        : nullptr;
}

void WGacNativeWindow::ApplyEffectiveCursor()
{
    auto seat = display ? display->GetWaylandSeat() : nullptr;
    auto resourceService = GetCursorResourceService();
    if (!seat || !resourceService)
    {
        return;
    }

    auto effectiveCursor = borderOverrideCursor
        ? borderOverrideCursor
        : compositionCursor;
    effectiveCursor = resourceService->ResolveSystemCursor(effectiveCursor);
    const auto type = effectiveCursor->GetSystemCursorType();
    seat->ApplyCursor(this, type);
}

void WGacNativeWindow::UpdateBorderOverrideCursor(int32_t x, int32_t y)
{
    INativeCursor* newOverride = nullptr;
    auto resourceService = GetCursorResourceService();
    if (resourceService &&
        mode == WindowMode::Normal &&
        customFrameMode &&
        sizeBox &&
        sizeState == WindowSizeState::Restored &&
        visible &&
        configured &&
        libdecorFrame &&
        libdecor_frame_is_floating(libdecorFrame))
    {
        newOverride = GetCursorFromHitTest(
            PerformCustomFrameHitTest(x, y),
            resourceService);
    }

    if (borderOverrideCursor != newOverride)
    {
        borderOverrideCursor = newOverride;
        ApplyEffectiveCursor();
    }
}

void WGacNativeWindow::RefreshBorderOverrideCursor()
{
    auto seat = display ? display->GetWaylandSeat() : nullptr;
    if (seat && seat->GetPointerFocus() == this)
    {
        UpdateBorderOverrideCursor(
            seat->GetPointerX(),
            seat->GetPointerY());
        ApplyEffectiveCursor();
    }
    else
    {
        borderOverrideCursor = nullptr;
    }
}

void WGacNativeWindow::ClearBorderOverrideCursor(bool applyEffectiveCursor)
{
    borderOverrideCursor = nullptr;
    if (applyEffectiveCursor)
    {
        ApplyEffectiveCursor();
    }
}

bool WGacNativeWindow::CreateLibdecorFrame()
{
    if (IsPopupMode() || libdecorFrame)
    {
        return !IsPopupMode();
    }

    auto context = display ? display->GetLibdecorContext() : nullptr;
    if (!context)
    {
        if (display)
        {
            display->ReportError("A normal window requires a libdecor context.");
        }
        return false;
    }

    libdecorFrame = libdecor_decorate(
        context,
        surface,
        &libdecor_frame_interface_,
        this);
    if (!libdecorFrame)
    {
        display->ReportError("libdecor failed to create a platform frame.");
        return false;
    }

    AString aTitle = title.Length() > 0
        ? wtoa(title)
        : AString::Unmanaged("GacUI Window");
    libdecor_frame_set_title(libdecorFrame, aTitle.Buffer());
    auto applicationId = GetWGacApplicationId();
    libdecor_frame_set_app_id(libdecorFrame, applicationId.Buffer());
    UpdateNativeParent();
    UpdatePlatformFrame(false);

    // libdecor 0.2.2 commits visibility changes with its current content
    // dimensions, even before the first configure. Seed a positive state so a
    // custom/borderless window cannot emit invalid 0x0 window geometry.
    auto initialState = libdecor_state_new(
        lastFloatingWidth > 0 ? lastFloatingWidth : 1,
        lastFloatingHeight > 0 ? lastFloatingHeight : 1);
    if (!initialState)
    {
        display->ReportError(
            "libdecor failed to allocate the initial platform-frame state.");
        DestroyLibdecorFrame();
        return false;
    }
    ::libdecor_frame_commit(libdecorFrame, initialState, nullptr);
    libdecor_state_free(initialState);
    libdecorStateInitialized = true;

    // libdecor silently falls back to an undecorated implementation when no
    // usable runtime plugin is selected. With forced CSD, a real platform
    // plugin must translate content origin below a non-zero title bar.
    int frameX = 0;
    int frameY = 0;
    libdecor_frame_translate_coordinate(
        libdecorFrame,
        0,
        0,
        &frameX,
        &frameY);
    if (frameY <= 0)
    {
        display->ReportError(
            "The selected libdecor runtime plugin does not provide a "
            "platform frame. Install libdecor-0-plugin-1-gtk or another "
            "functional libdecor plugin.");
        DestroyLibdecorFrame();
        return false;
    }

    UpdatePlatformFrame(false);
    return true;
}

void WGacNativeWindow::DestroyLibdecorFrame()
{
    ClearPressedCaptionButton();
    ClearBorderOverrideCursor(false);
    if (!libdecorFrame)
    {
        return;
    }

    libdecor_frame_unref(libdecorFrame);
    libdecorFrame = nullptr;
    configured = false;
    libdecorStateInitialized = false;
}

void WGacNativeWindow::UpdatePlatformFrame(bool commitState)
{
    if (!libdecorFrame)
    {
        return;
    }

    libdecor_frame_set_capabilities(
        libdecorFrame,
        static_cast<libdecor_capabilities>(
            LIBDECOR_ACTION_MOVE |
            LIBDECOR_ACTION_CLOSE |
            LIBDECOR_ACTION_FULLSCREEN));

    if (sizeBox)
    {
        if (!libdecor_frame_has_capability(
            libdecorFrame,
            LIBDECOR_ACTION_RESIZE))
        {
            libdecor_frame_set_capabilities(
                libdecorFrame,
                LIBDECOR_ACTION_RESIZE);
        }
        libdecor_frame_set_min_content_size(
            libdecorFrame,
            minWidth,
            minHeight);
        libdecor_frame_set_max_content_size(
            libdecorFrame,
            0,
            0);
    }
    else
    {
        if (libdecor_frame_has_capability(
                libdecorFrame,
                LIBDECOR_ACTION_RESIZE))
        {
            // Preserve the user constraints that libdecor restores when
            // resize capability is enabled again.
            libdecor_frame_set_min_content_size(
                libdecorFrame,
                minWidth,
                minHeight);
            libdecor_frame_set_max_content_size(
                libdecorFrame,
                0,
                0);
            libdecor_frame_unset_capabilities(
                libdecorFrame,
                LIBDECOR_ACTION_RESIZE);
        }
        if (!libdecor_frame_has_capability(
            libdecorFrame,
            LIBDECOR_ACTION_RESIZE))
        {
            // Never replace the floating fixed size with maximized,
            // fullscreen, or tiled configure dimensions.
            libdecor_frame_set_min_content_size(
                libdecorFrame,
                lastFloatingWidth,
                lastFloatingHeight);
            libdecor_frame_set_max_content_size(
                libdecorFrame,
                lastFloatingWidth,
                lastFloatingHeight);
        }
    }

    if (minimizedBox)
    {
        libdecor_frame_set_capabilities(
            libdecorFrame,
            LIBDECOR_ACTION_MINIMIZE);
    }
    else
    {
        libdecor_frame_unset_capabilities(
            libdecorFrame,
            LIBDECOR_ACTION_MINIMIZE);
    }

    const bool hasContentState = libdecorStateInitialized;
    if (commitState && hasContentState)
    {
        CommitRequestedSize();
    }

    if (hasContentState)
    {
        const bool showPlatformFrame =
            !customFrameMode &&
            border &&
            titleBar;
        if (libdecor_frame_is_visible(libdecorFrame) != showPlatformFrame)
        {
            libdecor_frame_set_visibility(
                libdecorFrame,
                showPlatformFrame);
        }
    }
}

void WGacNativeWindow::UpdateNativeParent()
{
    auto childToplevel = GetXdgToplevel();
    if (!libdecorFrame || !childToplevel)
    {
        return;
    }

    if (parentWindow &&
        parentWindow->libdecorFrame &&
        parentWindow->GetXdgToplevel())
    {
        libdecor_frame_set_parent(
            libdecorFrame,
            parentWindow->libdecorFrame);
    }
    else
    {
        // libdecor 0.2.2 dereferences a null parent. Use its xdg getter for
        // the supported protocol operation when clearing the relationship.
        xdg_toplevel_set_parent(childToplevel, nullptr);
    }
}

void WGacNativeWindow::CommitRequestedSize()
{
    if (!libdecorFrame || !configured ||
        !libdecor_frame_is_floating(libdecorFrame) ||
        currentWidth <= 0 || currentHeight <= 0)
    {
        return;
    }

    auto state = libdecor_state_new(currentWidth, currentHeight);
    if (!state)
    {
        display->ReportError("libdecor failed to allocate a window state.");
        return;
    }

    ::libdecor_frame_commit(libdecorFrame, state, nullptr);
    libdecor_state_free(state);
    wl_surface_commit(surface);
}

void WGacNativeWindow::ResizeBufferForCurrentSize()
{
    if (!bufferPool || currentWidth <= 0 || currentHeight <= 0)
    {
        return;
    }

    int32_t scale = display->GetOutputScale();
    if (scale < 1)
    {
        scale = 1;
    }

    const uint32_t scaledWidth =
        static_cast<uint32_t>(currentWidth * scale);
    const uint32_t scaledHeight =
        static_cast<uint32_t>(currentHeight * scale);
    if (bufferPool->GetWidth() != scaledWidth ||
        bufferPool->GetHeight() != scaledHeight)
    {
        bufferPool->Resize(scaledWidth, scaledHeight);
    }
    if (surface && currentBufferScale != scale)
    {
        currentBufferScale = scale;
        wl_surface_set_buffer_scale(surface, scale);
    }
}

void WGacNativeWindow::ReleasePopupGrab()
{
    if (!popupGrabParent)
    {
        return;
    }

    auto seat = display ? display->GetWaylandSeat() : nullptr;
    if (seat && !seat->GetName().empty() && popupGrabParent->libdecorFrame)
    {
        libdecor_frame_popup_ungrab(
            popupGrabParent->libdecorFrame,
            seat->GetName().c_str());
    }
    popupGrabParent = nullptr;
}

void WGacNativeWindow::DismissPopupChildren()
{
    while (true)
    {
        WGacNativeWindow* child = nullptr;
        for (vint i = childWindows.Count() - 1; i >= 0; i--)
        {
            auto candidate = childWindows[i];
            if (candidate &&
                candidate->IsPopupMode() &&
                (candidate->visible ||
                 candidate->popup ||
                 candidate->xdgSurface ||
                 candidate->popupSyncCallback))
            {
                child = candidate;
                break;
            }
        }
        if (!child)
        {
            break;
        }

        if (child->visible)
        {
            child->Hide(true);
        }
        else
        {
            child->Hide(false);
        }

        // An ancestor popup role cannot legally disappear while a descendant
        // remains topmost. If application code cancelled the close, force only
        // the native role teardown needed to preserve xdg-shell ordering.
        if (childWindows.Contains(child) &&
            (child->visible ||
             child->popup ||
             child->xdgSurface ||
             child->popupSyncCallback))
        {
            child->Hide(false);
        }
    }
}

bool WGacNativeWindow::CreateXdgSurface()
{
    if (xdgSurface) return true;
    if (!IsPopupMode() || !parentWindow)
    {
        return false;
    }

    xdgSurface = xdg_wm_base_get_xdg_surface(display->GetXdgWmBase(), surface);
    if (!xdgSurface) {
        return false;
    }
    xdg_surface_add_listener(xdgSurface, &xdg_surface_listener_, this);

    auto parentXdgSurface = parentWindow->GetXdgSurface();
    if (!parentXdgSurface)
    {
        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
        return false;
    }

    xdg_positioner* positioner =
        xdg_wm_base_create_positioner(display->GetXdgWmBase());
    if (!positioner) {
        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
        return false;
    }

    // GacUI stores normal-window popup positions in content coordinates.
    // xdg_positioner uses parent window-geometry coordinates, so include the
    // platform frame offset supplied by libdecor.
    int32_t relativeX;
    int32_t relativeY;
    if (parentWindow->IsPopupMode()) {
        relativeX = posX;
        relativeY = posY;
    } else {
        relativeX = posX - parentWindow->posX;
        relativeY = posY - parentWindow->posY;
        if (parentWindow->libdecorFrame)
        {
            libdecor_frame_translate_coordinate(
                parentWindow->libdecorFrame,
                relativeX,
                relativeY,
                &relativeX,
                &relativeY);
        }
    }

    xdg_positioner_set_size(
        positioner,
        currentWidth > 0 ? currentWidth : 100,
        currentHeight > 0 ? currentHeight : 100);
    xdg_positioner_set_anchor_rect(
        positioner,
        relativeX,
        relativeY,
        1,
        1);
    xdg_positioner_set_anchor(positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
    xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
    xdg_positioner_set_constraint_adjustment(
        positioner,
        XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
        XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y |
        XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X |
        XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y);

    popup = xdg_surface_get_popup(
        xdgSurface,
        parentXdgSurface,
        positioner);
    xdg_positioner_destroy(positioner);

    if (!popup) {
        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
        return false;
    }
    xdg_popup_add_listener(popup, &xdg_popup_listener_, this);

    // Tell both xdg-shell and libdecor about a genuine input-triggered popup
    // grab so a click on the platform frame dismisses the chain correctly.
    // Tooltips are never grabbing popups.
    WaylandSeat* seat = display->GetWaylandSeat();
    if (mode != WindowMode::Tooltip &&
        popupActivationSerial != 0 &&
        seat &&
        seat->GetSeat())
    {
        xdg_popup_grab(
            popup,
            seat->GetSeat(),
            popupActivationSerial);
        if (parentWindow->libdecorFrame && !seat->GetName().empty())
        {
            libdecor_frame_popup_grab(
                parentWindow->libdecorFrame,
                seat->GetName().c_str());
            popupGrabParent = parentWindow;
        }
    }

    return true;
}

void WGacNativeWindow::Destroy()
{
    ClearPressedCaptionButton();
    ClearBorderOverrideCursor(false);
    DismissPopupChildren();
    while (childWindows.Count() > 0)
    {
        childWindows[childWindows.Count() - 1]->SetParent(nullptr);
    }
    ReleasePopupGrab();
    SetParent(nullptr);

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

    if (popupSyncCallback) {
        wl_callback_destroy(popupSyncCallback);
        popupSyncCallback = nullptr;
    }

    if (popup) {
        xdg_popup_destroy(popup);
        popup = nullptr;
    }

    DestroyLibdecorFrame();

    if (xdgSurface) {
        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
    }

    delete view;
    view = nullptr;

    delete bufferPool;
    bufferPool = nullptr;

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

void WGacNativeWindow::libdecor_frame_configure(
    libdecor_frame* frame,
    libdecor_configuration* configuration,
    void* data)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    if (frame != self->libdecorFrame)
    {
        return;
    }

    libdecor_window_state windowState = LIBDECOR_WINDOW_STATE_NONE;
    libdecor_configuration_get_window_state(configuration, &windowState);
    const bool floatingConfiguration =
        (windowState &
            (LIBDECOR_WINDOW_STATE_MAXIMIZED |
             LIBDECOR_WINDOW_STATE_FULLSCREEN |
             LIBDECOR_WINDOW_STATE_TILED_LEFT |
             LIBDECOR_WINDOW_STATE_TILED_RIGHT |
             LIBDECOR_WINDOW_STATE_TILED_TOP |
             LIBDECOR_WINDOW_STATE_TILED_BOTTOM)) == 0;

    // A fresh floating role is allowed to configure without suggesting a
    // content size. This also happens after hiding a maximized window and
    // showing it restored, where currentWidth/currentHeight still describe
    // the old maximized configure. Seed the new floating configure from the
    // preserved floating size instead.
    int width = floatingConfiguration
        ? self->lastFloatingWidth
        : self->currentWidth;
    int height = floatingConfiguration
        ? self->lastFloatingHeight
        : self->currentHeight;
    int configuredWidth = 0;
    int configuredHeight = 0;
    if (libdecor_configuration_get_content_size(
        configuration,
        frame,
        &configuredWidth,
        &configuredHeight))
    {
        if (configuredWidth > 0)
        {
            width = configuredWidth;
        }
        if (configuredHeight > 0)
        {
            height = configuredHeight;
        }
    }

    self->sizeState = WindowSizeState::Restored;
    if (windowState &
        (LIBDECOR_WINDOW_STATE_MAXIMIZED |
         LIBDECOR_WINDOW_STATE_FULLSCREEN))
    {
        self->sizeState = WindowSizeState::Maximized;
    }

    auto state = libdecor_state_new(width, height);
    if (!state)
    {
        self->display->ReportError(
            "libdecor failed to allocate a configured window state.");
        return;
    }

    ::libdecor_frame_commit(frame, state, configuration);
    libdecor_state_free(state);
    wl_surface_commit(self->surface);

    const bool firstConfigure = !self->configured;
    const bool sizeChanged =
        self->currentWidth != width ||
        self->currentHeight != height;
    self->currentWidth = width;
    self->currentHeight = height;
    self->configured = true;
    if (libdecor_frame_is_floating(frame))
    {
        self->lastFloatingWidth = width;
        self->lastFloatingHeight = height;
    }
    self->ResizeBufferForCurrentSize();
    self->UpdateNativeParent();
    self->UpdatePlatformFrame(false);

    for (vint i = 0; i < self->childWindows.Count(); i++)
    {
        self->childWindows[i]->UpdateNativeParent();
    }

    if (sizeChanged)
    {
        for (vint i = 0; i < self->listeners.Count(); i++)
        {
            self->listeners[i]->Moved();
        }
    }
    self->RefreshBorderOverrideCursor();

    if (firstConfigure && self->visible)
    {
        if (self->pendingFrame)
        {
            self->pendingFrame = false;
            self->hasFirstFrame = false;
        }
        self->RequestFrame();
    }
}

void WGacNativeWindow::libdecor_frame_close(libdecor_frame* frame, void* data)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    if (frame == self->libdecorFrame)
    {
        self->RequestClose();
    }
}

void WGacNativeWindow::libdecor_frame_commit(libdecor_frame* frame, void* data)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    if (frame == self->libdecorFrame && self->surface)
    {
        wl_surface_commit(self->surface);
    }
}

void WGacNativeWindow::libdecor_frame_dismiss_popup(
    libdecor_frame* frame,
    const char*,
    void* data)
{
    auto* self = static_cast<WGacNativeWindow*>(data);
    if (frame == self->libdecorFrame)
    {
        self->DismissPopupChildren();
    }
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
    self->DismissPopupChildren();

    // Save parent reference before clearing focus
    auto* parent = self->parentWindow;

    // A destroyed popup cannot borrow its parent's pointer identity: the
    // popup surface and enter serial are no longer a valid pair. Wait for a
    // real parent enter before applying the parent's cursor.
    if (self->display && self->display->GetWaylandSeat()) {
        self->display->GetWaylandSeat()->ClearFocusFor(self);
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
    self->ReleasePopupGrab();
    self->popupActivationSerial = 0;
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

    const bool canApplySize =
        !libdecorFrame ||
        !configured ||
        libdecor_frame_is_floating(libdecorFrame);
    if (canApplySize &&
        (newWidth != currentWidth || newHeight != currentHeight))
    {
        currentWidth = newWidth;
        currentHeight = newHeight;
        lastFloatingWidth = newWidth;
        lastFloatingHeight = newHeight;
        ResizeBufferForCurrentSize();
        UpdatePlatformFrame();
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
    const bool canApplySize =
        !libdecorFrame ||
        !configured ||
        libdecor_frame_is_floating(libdecorFrame);
    if (canApplySize)
    {
        currentWidth = size.x.value;
        currentHeight = size.y.value;
        lastFloatingWidth = currentWidth;
        lastFloatingHeight = currentHeight;
        ResizeBufferForCurrentSize();
        UpdatePlatformFrame();
    }
    for (vint i = 0; i < listeners.Count(); i++) { listeners[i]->Moved(); }
}
NativeRect WGacNativeWindow::GetClientBoundsInScreen() {
    // For popup windows, return bounds relative to self (0,0) since rendering is in window-local coords
    if (IsPopupMode()) {
        return NativeRect(0, 0, currentWidth, currentHeight);
    }
    return NativeRect(posX, posY, posX + currentWidth, posY + currentHeight);
}

void WGacNativeWindow::SuggestMinClientSize(NativeSize size) {
    minWidth = size.x.value;
    minHeight = size.y.value;
    UpdatePlatformFrame();
}

WString WGacNativeWindow::GetTitle() { return title; }
void WGacNativeWindow::SetTitle(const WString& _title) {
    title = _title;
    if (libdecorFrame) {
        AString aTitle = wtoa(title);
        libdecor_frame_set_title(libdecorFrame, aTitle.Buffer());
        if (configured && surface)
        {
            wl_surface_commit(surface);
        }
    }
    NotifyWGacNativeWindowTitleChanged(this);
}

void WGacNativeWindow::SetApplicationId(const AString& applicationId) {
    if (libdecorFrame) {
        libdecor_frame_set_app_id(libdecorFrame, applicationId.Buffer());
        if (configured && surface)
        {
            wl_surface_commit(surface);
        }
    }
}

INativeCursor* WGacNativeWindow::GetWindowCursor() { return compositionCursor; }
void WGacNativeWindow::SetWindowCursor(INativeCursor* cursor) {
    if (auto resourceService = GetCursorResourceService())
    {
        compositionCursor = resourceService->ResolveSystemCursor(cursor);
    }
    else
    {
        compositionCursor = nullptr;
    }
    ApplyEffectiveCursor();
}
NativePoint WGacNativeWindow::GetCaretPoint() { return caretPoint; }
void WGacNativeWindow::SetCaretPoint(NativePoint point) {
    caretPoint = point;
    // TODO: Text input is temporarily disabled to debug focus issues
    // Will re-enable once button click handling is fixed
}

INativeWindow* WGacNativeWindow::GetParent() { return parentWindow; }
void WGacNativeWindow::SetParent(INativeWindow* parent) {
    auto newParent = dynamic_cast<WGacNativeWindow*>(parent);
    if (parentWindow == newParent)
    {
        UpdateNativeParent();
        return;
    }

    if (parentWindow && parentWindow->childWindows.Contains(this))
    {
        parentWindow->childWindows.Remove(this);
    }
    parentWindow = newParent;
    if (parentWindow && !parentWindow->childWindows.Contains(this))
    {
        parentWindow->childWindows.Add(this);
    }
    UpdateNativeParent();
    if (configured && surface)
    {
        wl_surface_commit(surface);
    }
}
INativeWindow::WindowMode WGacNativeWindow::GetWindowMode() { return mode; }
void WGacNativeWindow::EnableCustomFrameMode() {
    customFrameMode = true;
    UpdatePlatformFrame();
    RefreshBorderOverrideCursor();
}
void WGacNativeWindow::DisableCustomFrameMode() {
    ClearPressedCaptionButton();
    customFrameMode = false;
    ClearBorderOverrideCursor(true);
    UpdatePlatformFrame();
}
bool WGacNativeWindow::IsCustomFrameModeEnabled() { return customFrameMode; }
NativeMargin WGacNativeWindow::GetCustomFramePadding() { return sizeBox || titleBar ? NativeMargin(5, 5, 5, 5) : NativeMargin(0, 0, 0, 0); }

Ptr<GuiImageData> WGacNativeWindow::GetIcon() { return nullptr; }
void WGacNativeWindow::SetIcon(Ptr<GuiImageData> icon) {}

INativeWindow::WindowSizeState WGacNativeWindow::GetSizeState() { return sizeState; }

void WGacNativeWindow::Show() {
    closed = false;
    visible = true;

    if (IsPopupMode())
    {
        auto seat = display ? display->GetWaylandSeat() : nullptr;
        popupActivationSerial = seat
            ? seat->GetCurrentInputSerial()
            : 0;
        if (popupActivationSerial == 0 &&
            parentWindow &&
            parentWindow->IsPopupMode())
        {
            popupActivationSerial = parentWindow->popupActivationSerial;
        }
        if (!xdgSurface) {
            if (!popupSyncCallback)
            {
                // Delay creation using a sync callback so SetBounds can finish.
                popupSyncCallback = wl_display_sync(display->GetDisplay());
                wl_callback_add_listener(
                    popupSyncCallback,
                    &popup_sync_listener,
                    this);
            }
            return;
        }
    }
    else
    {
        if (!libdecorFrame && !CreateLibdecorFrame())
        {
            visible = false;
            throw std::runtime_error(
                "wGac failed to recreate a libdecor platform frame: " +
                display->GetLastError());
        }
        if (sizeState == WindowSizeState::Maximized)
        {
            libdecor_frame_set_maximized(libdecorFrame);
        }
        else if (sizeState == WindowSizeState::Minimized)
        {
            libdecor_frame_set_minimized(libdecorFrame);
        }
        libdecor_frame_map(libdecorFrame);
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
void WGacNativeWindow::ShowRestored() {
    sizeState = WindowSizeState::Restored;
    if (!visible)
    {
        Show();
    }
    if (libdecorFrame && GetXdgToplevel()) {
        libdecor_frame_unset_maximized(libdecorFrame);
    }
    RefreshBorderOverrideCursor();
}
void WGacNativeWindow::ShowMaximized() {
    sizeState = WindowSizeState::Maximized;
    ClearBorderOverrideCursor(true);
    if (!visible)
    {
        Show();
    }
    if (libdecorFrame && GetXdgToplevel()) {
        libdecor_frame_set_maximized(libdecorFrame);
    }
}
void WGacNativeWindow::ShowMinimized() {
    sizeState = WindowSizeState::Minimized;
    ClearBorderOverrideCursor(true);
    if (!visible)
    {
        Show();
    }
    if (libdecorFrame && GetXdgToplevel()) {
        libdecor_frame_set_minimized(libdecorFrame);
    }
}

bool WGacNativeWindow::RequestClose() {
    bool cancel = false;
    for (vint i = 0; i < listeners.Count(); i++) {
        listeners[i]->BeforeClosing(cancel);
    }
    if (cancel) {
        return false;
    }

    for (vint i = 0; i < listeners.Count(); i++) {
        listeners[i]->AfterClosing();
    }

    auto* parent = parentWindow;
    bool parentVisible = parent ? parent->visible : false;
    Hide(false);
    closed = true;

    if (parent && parentVisible) {
        parent->OnFocusChanged(true);
    }

    for (vint i = 0; i < listeners.Count(); i++) {
        listeners[i]->Closed();
    }
    return true;
}

void WGacNativeWindow::Hide(bool closeWindow) {
    ClearPressedCaptionButton();
    ClearBorderOverrideCursor(false);
    if (closeWindow) {
        RequestClose();
        return;
    }

    // If already hidden or never shown, just update state
    if (!visible &&
        !xdgSurface &&
        !popup &&
        !popupSyncCallback &&
        !libdecorFrame)
    {
        return;
    }

    visible = false;

    // Clear the focused surface and its enter serial before destroying roles.
    // A later real enter will establish a valid target/serial pair.
    if (display && display->GetWaylandSeat()) {
        display->GetWaylandSeat()->ClearFocusFor(this);
    }

    // Cancel any pending frame callback
    if (frameCallback) {
        wl_callback_destroy(frameCallback);
        frameCallback = nullptr;
    }
    pendingFrame = false;

    if (IsPopupMode()) {
        // Reset state so next Show() works correctly
        hasKeyboardFocus = false;
        capturing = false;
        DismissPopupChildren();
        ReleasePopupGrab();
        popupActivationSerial = 0;

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
    } else {
        // Wayland has no toplevel unmap request. Releasing the libdecor frame
        // destroys the role while preserving the application-content surface;
        // Show() decorates and maps the same content surface again.
        DismissPopupChildren();
        DestroyLibdecorFrame();
        configured = false;
        hasFirstFrame = false;
        if (surface) {
            wl_surface_attach(surface, nullptr, 0, 0);
            wl_surface_commit(surface);
        }
        display->Flush();
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
void WGacNativeWindow::SetMaximizedBox(bool visible) {
    maximizedBox = visible;
    UpdatePlatformFrame();
}
bool WGacNativeWindow::GetMinimizedBox() { return minimizedBox; }
void WGacNativeWindow::SetMinimizedBox(bool visible) {
    minimizedBox = visible;
    UpdatePlatformFrame();
}
bool WGacNativeWindow::GetBorder() { return border; }
void WGacNativeWindow::SetBorder(bool visible) {
    border = visible;
    UpdatePlatformFrame();
}
bool WGacNativeWindow::GetSizeBox() { return sizeBox; }
void WGacNativeWindow::SetSizeBox(bool visible) {
    sizeBox = visible;
    UpdatePlatformFrame();
    if (sizeBox)
    {
        RefreshBorderOverrideCursor();
    }
    else
    {
        ClearBorderOverrideCursor(true);
    }
}
bool WGacNativeWindow::GetIconVisible() { return false; }
void WGacNativeWindow::SetIconVisible(bool) { iconVisible = false; }
bool WGacNativeWindow::GetTitleBar() { return titleBar; }
void WGacNativeWindow::SetTitleBar(bool visible) {
    titleBar = visible;
    UpdatePlatformFrame();
}
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
    auto seat = display ? display->GetWaylandSeat() : nullptr;
    if (!seat ||
        seat->GetPointerFocus() != this ||
        seat->GetPointerX() != x ||
        seat->GetPointerY() != y)
    {
        return;
    }
    UpdateBorderOverrideCursor(x, y);
    // wl_pointer.enter makes the pointer image undefined. Always submit the
    // effective cursor with this enter's serial, even if its type is unchanged.
    ApplyEffectiveCursor();
}

void WGacNativeWindow::OnMouseLeave() {
    ClearPressedCaptionButton();
    ClearBorderOverrideCursor(false);
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
    auto seat = display ? display->GetWaylandSeat() : nullptr;
    if (!seat ||
        seat->GetPointerFocus() != this ||
        seat->GetPointerX() != info.x ||
        seat->GetPointerY() != info.y)
    {
        return;
    }
    UpdateBorderOverrideCursor(info.x, info.y);
}

void WGacNativeWindow::OnMouseButton(const MouseEventInfo& info, bool pressed) {
    const uint32_t buttonPressSerial = info.buttonPressSerial;
    auto* const actionSeat = display ? display->GetWaylandSeat() : nullptr;
    auto* const actionFrame = libdecorFrame;
    const bool leftButton =
        info.button == static_cast<uint32_t>(MouseButton::Left);

    auto hitTestResult = INativeWindowListener::NoDecision;
    auto captionPress = INativeWindowListener::NoDecision;
    auto resizeEdge = LIBDECOR_RESIZE_EDGE_NONE;
    if (leftButton)
    {
        if (pressed)
        {
            ClearPressedCaptionButton();
        }
        else
        {
            captionPress = pressedCaptionButton;
            ClearPressedCaptionButton();
        }

        hitTestResult = PerformCustomFrameHitTest(info.x, info.y);
        if (pressed)
        {
            switch (hitTestResult)
            {
            case INativeWindowListener::ButtonMinimum:
            case INativeWindowListener::ButtonMaximum:
            case INativeWindowListener::ButtonClose:
                pressedCaptionButton = hitTestResult;
                break;
            default:
                break;
            }
        }

        switch (hitTestResult)
        {
        case INativeWindowListener::BorderLeft:
            resizeEdge = LIBDECOR_RESIZE_EDGE_LEFT;
            break;
        case INativeWindowListener::BorderRight:
            resizeEdge = LIBDECOR_RESIZE_EDGE_RIGHT;
            break;
        case INativeWindowListener::BorderTop:
            resizeEdge = LIBDECOR_RESIZE_EDGE_TOP;
            break;
        case INativeWindowListener::BorderBottom:
            resizeEdge = LIBDECOR_RESIZE_EDGE_BOTTOM;
            break;
        case INativeWindowListener::BorderLeftTop:
            resizeEdge = LIBDECOR_RESIZE_EDGE_TOP_LEFT;
            break;
        case INativeWindowListener::BorderRightTop:
            resizeEdge = LIBDECOR_RESIZE_EDGE_TOP_RIGHT;
            break;
        case INativeWindowListener::BorderLeftBottom:
            resizeEdge = LIBDECOR_RESIZE_EDGE_BOTTOM_LEFT;
            break;
        case INativeWindowListener::BorderRightBottom:
            resizeEdge = LIBDECOR_RESIZE_EDGE_BOTTOM_RIGHT;
            break;
        default:
            break;
        }
    }

    const bool canRequestInteractiveAction =
        leftButton &&
        pressed &&
        buttonPressSerial != 0 &&
        mode == WindowMode::Normal &&
        customFrameMode &&
        visible &&
        configured &&
        actionSeat &&
        display &&
        display->GetWaylandSeat() == actionSeat &&
        actionSeat->GetSeat() &&
        actionSeat->GetPointerFocus() == this &&
        actionFrame &&
        libdecorFrame == actionFrame &&
        GetXdgToplevel();
    const bool requestTitleMove =
        canRequestInteractiveAction &&
        hitTestResult == INativeWindowListener::Title &&
        titleBar;
    const bool requestBorderResize =
        canRequestInteractiveAction &&
        resizeEdge != LIBDECOR_RESIZE_EDGE_NONE &&
        sizeBox &&
        sizeState == WindowSizeState::Restored &&
        libdecor_frame_is_floating(actionFrame);

    NativeWindowMouseInfo nativeInfo = {};  // Zero-initialize all fields
    nativeInfo.x = info.x;
    nativeInfo.y = info.y;
    nativeInfo.ctrl = info.ctrl;
    nativeInfo.shift = info.shift;
    nativeInfo.wheel = 0;
    // A compositor-owned move/resize takes the pointer grab and may withhold
    // the release from this surface. Keep the callback, but prevent GacUI from
    // retaining a client capture that it could never release.
    nativeInfo.nonClient = requestTitleMove || requestBorderResize;
    nativeInfo.left = info.left;
    nativeInfo.middle = info.middle;
    nativeInfo.right = info.right;

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

    if (leftButton && pressed)
    {
        const bool actionIsStillValid =
            nativeInfo.nonClient &&
            mode == WindowMode::Normal &&
            customFrameMode &&
            visible &&
            configured &&
            actionSeat &&
            display &&
            display->GetWaylandSeat() == actionSeat &&
            actionSeat->GetSeat() &&
            actionSeat->GetPointerFocus() == this &&
            actionFrame &&
            libdecorFrame == actionFrame &&
            GetXdgToplevel();
        if (!actionIsStillValid)
        {
            return;
        }

        if (requestTitleMove)
        {
            if (titleBar)
            {
                libdecor_frame_move(
                    actionFrame,
                    actionSeat->GetSeat(),
                    buttonPressSerial);
            }
            return;
        }

        if (!requestBorderResize ||
            !sizeBox ||
            sizeState != WindowSizeState::Restored ||
            !libdecor_frame_is_floating(actionFrame))
        {
            return;
        }

        libdecor_frame_resize(
            actionFrame,
            actionSeat->GetSeat(),
            buttonPressSerial,
            resizeEdge);
        return;
    }

    if (!leftButton ||
        pressed ||
        captionPress == INativeWindowListener::NoDecision ||
        hitTestResult != captionPress ||
        mode != WindowMode::Normal ||
        !customFrameMode ||
        !visible ||
        !configured ||
        !actionFrame ||
        libdecorFrame != actionFrame)
    {
        return;
    }

    switch (captionPress)
    {
    case INativeWindowListener::ButtonMinimum:
        if (minimizedBox)
        {
            ShowMinimized();
        }
        break;
    case INativeWindowListener::ButtonMaximum:
        if (maximizedBox)
        {
            if (GetSizeState() == WindowSizeState::Maximized)
            {
                ShowRestored();
            }
            else
            {
                ShowMaximized();
            }
        }
        break;
    case INativeWindowListener::ButtonClose:
        Hide(true);
        break;
    default:
        break;
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
    if (!focused)
    {
        ClearPressedCaptionButton();
    }
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
