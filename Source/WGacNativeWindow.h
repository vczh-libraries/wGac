#ifndef WGAC_NATIVE_WINDOW_H
#define WGAC_NATIVE_WINDOW_H

#include "GacUI.h"
#include "Wayland/WaylandDisplay.h"
#include "Wayland/WaylandBuffer.h"
#include "Wayland/WaylandSeat.h"
#include "Wayland/IWaylandWindow.h"
#include <cairo/cairo.h>

namespace vl {
namespace presentation {
namespace wayland {

class WGacView;
class WGacController;

class WGacNativeWindow : public Object, public INativeWindow, public IWaylandWindow
{
    using WindowListenerList = collections::List<INativeWindowListener*>;

protected:
    WaylandDisplay* display;
    wl_surface* surface;
    xdg_surface* xdgSurface;
    xdg_toplevel* toplevel;
    xdg_popup* popup;
    zxdg_toplevel_decoration_v1* decoration;
    wl_callback* frameCallback;
    wl_callback* popupSyncCallback;

    WaylandBufferPool* bufferPool;
    WGacView* view;

    WGacNativeWindow* parentWindow;
    INativeCursor* cursor;
    Interface* graphicsHandler;
    WindowListenerList listeners;
    WString title;
    WindowMode mode;

    int32_t currentWidth;
    int32_t currentHeight;
    int32_t posX;
    int32_t posY;
    int32_t currentBufferScale;

    bool configured;
    bool visible;
    bool closed;
    bool pendingFrame;
    bool hasFirstFrame;

    bool customFrameMode;
    bool enabled;
    bool capturing;
    bool border;
    bool sizeBox;
    bool topMost;
    bool titleBar;
    bool iconVisible;
    bool maximizedBox;
    bool minimizedBox;

    WindowSizeState sizeState;
    NativePoint caretPoint;
    bool textInputEnabled;
    bool hasKeyboardFocus;

    void RequestFrame();
    void OnFrame();
    bool CreateXdgSurface();

public:
    // Wayland callbacks
    static void xdg_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    static void xdg_toplevel_configure(void* data, xdg_toplevel* toplevel,
                                        int32_t width, int32_t height, wl_array* states);
    static void xdg_toplevel_close(void* data, xdg_toplevel* toplevel);
    static void xdg_toplevel_configure_bounds(void* data, xdg_toplevel* toplevel,
                                               int32_t width, int32_t height);
    static void xdg_toplevel_wm_capabilities(void* data, xdg_toplevel* toplevel,
                                              wl_array* capabilities);
    static void xdg_popup_configure(void* data, xdg_popup* popup,
                                     int32_t x, int32_t y, int32_t width, int32_t height);
    static void xdg_popup_done(void* data, xdg_popup* popup);
    static void frame_done(void* data, wl_callback* callback, uint32_t time);
    static void popup_sync_done(void* data, wl_callback* callback, uint32_t time);

public:
    WGacNativeWindow(WaylandDisplay* display, INativeWindow::WindowMode mode);
    virtual ~WGacNativeWindow();

    bool Create();
    void Destroy();

    // IWaylandWindow implementation
    wl_surface* GetSurface() const override { return surface; }
    WGacView* GetGacView() const { return view; }
    void SetGraphicsHandler(Interface* handler);
    Interface* GetGraphicsHandler() const;
    void CommitBuffer();

    // INativeWindow implementation
    bool IsActivelyRefreshing() override;
    NativeSize GetRenderingOffset() override;
    bool IsRenderingAsActivated() override;

    Point Convert(NativePoint value) override;
    NativePoint Convert(Point value) override;
    Size Convert(NativeSize value) override;
    NativeSize Convert(Size value) override;
    Margin Convert(NativeMargin value) override;
    NativeMargin Convert(Margin value) override;

    NativeRect GetBounds() override;
    void SetBounds(const NativeRect& bounds) override;
    NativeSize GetClientSize() override;
    void SetClientSize(NativeSize size) override;
    NativeRect GetClientBoundsInScreen() override;
    void SuggestMinClientSize(NativeSize size) override;

    WString GetTitle() override;
    void SetTitle(const WString& title) override;
    INativeCursor* GetWindowCursor() override;
    void SetWindowCursor(INativeCursor* cursor) override;
    NativePoint GetCaretPoint() override;
    void SetCaretPoint(NativePoint point) override;

    INativeWindow* GetParent() override;
    void SetParent(INativeWindow* parent) override;
    WindowMode GetWindowMode() override;
    void EnableCustomFrameMode() override;
    void DisableCustomFrameMode() override;
    bool IsCustomFrameModeEnabled() override;
    NativeMargin GetCustomFramePadding() override;

    Ptr<GuiImageData> GetIcon() override;
    void SetIcon(Ptr<GuiImageData> icon) override;

    WindowSizeState GetSizeState() override;
    void Show() override;
    void ShowDeactivated() override;
    void ShowRestored() override;
    void ShowMaximized() override;
    void ShowMinimized() override;
    void Hide(bool closeWindow) override;
    bool IsVisible() override;

    void Enable() override;
    void Disable() override;
    bool IsEnabled() override;
    void SetActivate() override;
    bool IsActivated() override;

    void ShowInTaskBar() override;
    void HideInTaskBar() override;
    bool IsAppearedInTaskBar() override;
    void EnableActivate() override;
    void DisableActivate() override;
    bool IsEnabledActivate() override;

    bool RequireCapture() override;
    bool ReleaseCapture() override;
    bool IsCapturing() override;

    bool GetMaximizedBox() override;
    void SetMaximizedBox(bool visible) override;
    bool GetMinimizedBox() override;
    void SetMinimizedBox(bool visible) override;
    bool GetBorder() override;
    void SetBorder(bool visible) override;
    bool GetSizeBox() override;
    void SetSizeBox(bool visible) override;
    bool GetIconVisible() override;
    void SetIconVisible(bool visible) override;
    bool GetTitleBar() override;
    void SetTitleBar(bool visible) override;
    bool GetTopMost() override;
    void SetTopMost(bool topMost) override;

    void SupressAlt() override;
    bool InstallListener(INativeWindowListener* listener) override;
    bool UninstallListener(INativeWindowListener* listener) override;
    void RedrawContent() override;

    // IWaylandWindow input event handlers
    void OnMouseEnter(int32_t x, int32_t y) override;
    void OnMouseLeave() override;
    void OnMouseMove(const MouseEventInfo& info) override;
    void OnMouseButton(const MouseEventInfo& info, bool pressed) override;
    void OnMouseScroll(const ScrollEventInfo& info) override;
    void OnKeyEvent(const KeyEventInfo& info) override;
    void OnFocusChanged(bool focused) override;

    // IME event handlers
    void OnTextInputPreedit(const PreeditInfo& info) override;
    void OnTextInputCommit(const std::string& text) override;
};

}
}
}

#endif // WGAC_NATIVE_WINDOW_H
