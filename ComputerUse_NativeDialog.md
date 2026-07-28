## Linux Specific

Use the GacUI automation service for all GacUI-owned windows. On wGac, normal
and hosted applications expose `Controls` and `IO`; the native remote renderer
exposes `Dom` and renderer-side `IO`. Linux computer use is the fallback for an
OS-native modal window, not a replacement for those endpoints.

A native portal dialog runs a synchronous nested modal loop while the portal
backend shows a window from another process. wGac keeps Wayland events, timers,
and queued MiniHTTP main-thread work advancing during that loop, but the native
dialog still belongs to the portal process and is absent from GacUI
`Controls`. Inspect and operate it from a separate process with AT-SPI. Avoid
driving controls in the underlying GacUI window until the modal call finishes,
because doing so would re-enter application code.

On GNOME Wayland, wGac's XDG Desktop Portal file chooser is normally exposed
through AT-SPI by `xdg-desktop-portal-gnome`. The accessible application and
PID therefore need not match `Test_FullControlTest`. Locate the window by its
application-supplied title and inspect its descendants. The Full Control Test
uses `The Title`; do not assume that title for another application.
Hosted Full Control Test uses `FakeDialogService`, so its dialogs stay in the
GacUI `Controls` tree and must be operated through `IO`, not AT-SPI.

Install the AT-SPI Python introspection packages if they are unavailable:

```bash
sudo apt install python3-gi gir1.2-atspi-2.0
```

For an unattended visual check on Wayland, call the XDG Desktop Portal
`Screenshot` method with `interactive=false`. This captures the complete
desktop without opening a rectangle picker or requiring user input:

```bash
/usr/bin/python3 - "/tmp/gacui-linux-desktop.png" <<'PY'
import os, sys, uuid, gi
gi.require_version("Gio", "2.0")
from gi.repository import Gio, GLib

OUTPUT = os.path.abspath(sys.argv[1])
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
token = "gacui_" + uuid.uuid4().hex
sender = bus.get_unique_name()[1:].replace(".", "_")
request_path = f"/org/freedesktop/portal/desktop/request/{sender}/{token}"
result = {}
loop = GLib.MainLoop()

def on_response(connection, sender_name, object_path, interface_name, signal_name, parameters):
    code, values = parameters.unpack()
    result["code"] = code
    result["uri"] = values.get("uri")
    loop.quit()

subscription = bus.signal_subscribe(
    "org.freedesktop.portal.Desktop",
    "org.freedesktop.portal.Request",
    "Response",
    request_path,
    None,
    Gio.DBusSignalFlags.NONE,
    on_response,
)
returned_path = bus.call_sync(
    "org.freedesktop.portal.Desktop",
    "/org/freedesktop/portal/desktop",
    "org.freedesktop.portal.Screenshot",
    "Screenshot",
    GLib.Variant("(sa{sv})", ("", {
        "handle_token": GLib.Variant("s", token),
        "interactive": GLib.Variant("b", False),
    })),
    GLib.VariantType("(o)"),
    Gio.DBusCallFlags.NONE,
    -1,
    None,
).unpack()[0]
if returned_path != request_path:
    raise SystemExit(f"Unexpected portal request path: {returned_path}")
timeout = GLib.timeout_add_seconds(15, lambda: (loop.quit(), False)[1])
loop.run()
GLib.source_remove(timeout)
bus.signal_unsubscribe(subscription)
if result.get("code") != 0 or not result.get("uri"):
    raise SystemExit(f"Screenshot failed: {result}")
source = Gio.File.new_for_uri(result["uri"])
target = Gio.File.new_for_path(OUTPUT)
source.copy(target, Gio.FileCopyFlags.OVERWRITE, None, None)
print(OUTPUT)
PY
```

Require a zero exit code and inspect the saved PNG. The desktop portal may
return a temporary source URI, so keep the explicitly copied output file as
the evidence artifact.

Run inspection from the same logged-in graphical session as the application.
This helper finds every accessible object with the requested title and prints
its complete subtree, including action names:

```bash
/usr/bin/python3 - "The Title" <<'PY'
import sys, gi
gi.require_version("Atspi", "2.0")
from gi.repository import Atspi
TITLE = sys.argv[1]
def children(n):
    try: return [n.get_child_at_index(i) for i in range(max(0, n.get_child_count()))]
    except Exception: return []
def walk(n):
    yield n
    for c in children(n): yield from walk(c)
def showing(n):
    try: return n.get_state_set().contains(Atspi.StateType.SHOWING)
    except Exception: return False
def actions(n):
    try: return [n.get_action_name(i) for i in range(max(0, n.get_n_actions()))]
    except Exception: return []
def describe(n, depth=0):
    try: print(f"{'  '*depth}{n.get_role_name()} name={(n.get_name() or '')!r} actions={actions(n)}")
    except Exception as e: print(f"{'  '*depth}<unavailable: {e}>")
    for c in children(n): describe(c, depth+1)
desktop = Atspi.get_desktop(0)
found=[]
for app in children(desktop):
    for n in walk(app):
        try:
            if n.get_role() == Atspi.Role.DIALOG and (n.get_name() or "") == TITLE and showing(n): found.append((app,n))
        except Exception: pass
if len(found) != 1: raise SystemExit(f"Expected one visible dialog {TITLE!r}, found {len(found)}")
app, dialog = found[0]
print(f"owner={(app.get_name() or '')!r} pid={app.get_process_id()}")
describe(dialog)
PY
```

Read the reported names, roles, hierarchy, and actions before interacting. A
GNOME portal file chooser exposes `Cancel` and `Select` or an operation-specific
accept button as push buttons with a `click` action. When the goal is only to
unblock a test, prefer the least destructive named action, normally `Cancel`.

The following helper clicks one exact push-button name inside one exact titled
window:

```bash
/usr/bin/python3 - "The Title" "Cancel" <<'PY'
import sys, time, gi
gi.require_version("Atspi", "2.0")
from gi.repository import Atspi
TITLE, BUTTON = sys.argv[1:3]
def children(n):
    try: return [n.get_child_at_index(i) for i in range(max(0, n.get_child_count()))]
    except Exception: return []
def walk(n):
    yield n
    for c in children(n): yield from walk(c)
def showing(n):
    try: return n.get_state_set().contains(Atspi.StateType.SHOWING)
    except Exception: return False
def dialogs():
    result=[]
    desktop=Atspi.get_desktop(0)
    for app in children(desktop):
        for n in walk(app):
            try:
                if n.get_role() == Atspi.Role.DIALOG and (n.get_name() or "") == TITLE and showing(n): result.append(n)
            except Exception: pass
    return result
ds=dialogs()
if len(ds) != 1: raise SystemExit(f"Expected one visible dialog {TITLE!r}, found {len(ds)}")
candidates=[]
for n in walk(ds[0]):
    try:
        if n.get_role() == Atspi.Role.PUSH_BUTTON and (n.get_name() or "") == BUTTON and showing(n):
            clicks=[i for i in range(max(0,n.get_n_actions())) if n.get_action_name(i)=="click"]
            if len(clicks)==1: candidates.append((n,clicks[0]))
    except Exception: pass
if len(candidates) != 1: raise SystemExit(f"Expected one visible clickable {BUTTON!r}, found {len(candidates)}")
if not candidates[0][0].do_action(candidates[0][1]): raise SystemExit("AT-SPI click rejected")
deadline=time.monotonic()+5
while dialogs() and time.monotonic()<deadline: time.sleep(0.1)
if dialogs(): raise SystemExit("Dialog remained visible after click")
PY
```

After the action, require the native dialog to disappear. Then resume the
GacUI `Controls`/`Dom` and `IO` checks and verify the expected application
state. If the dialog remains, enumerate the desktop again: a validation error
or a second modal window may have appeared.

Do not use X11-only tools such as `xdotool` as the primary Wayland procedure.
Do not guess screen coordinates when semantic AT-SPI actions are available.
For a different desktop portal backend or a non-portal crash dialog, enumerate
the complete AT-SPI desktop first and identify the actual owning application,
title, prompt, and named actions before choosing a response.
