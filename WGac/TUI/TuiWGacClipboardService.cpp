#include "TuiWGacClipboardService.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <poll.h>
#include <chrono>
#include <cerrno>

namespace vl::presentation::wayland
{
	class TuiWGacClipboardService::Impl : public Object
	{
	public:
		Display*				display = nullptr;
		::Window			window = 0;
		Atom				clipboard = 0;
		Atom				utf8 = 0;
		Atom				targets = 0;
		Atom				property = 0;
		Atom				incr = 0;
		FakeClipboardService	local;
		AString				received;
		bool				waiting = false;
		bool				incremental = false;
		bool				succeeded = false;
		int					selectionEvent = -1;
		bool				clipboardChanged = false;
		::Window			observedOwner = 0;

		Impl()
		{
			// The terminal owns Wayland focus and its input serial. A separate
			// Wayland client cannot use that serial to own the clipboard.
			display = XOpenDisplay(nullptr);
			if (!display) return;
			window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
			XSelectInput(display, window, PropertyChangeMask);
			clipboard = XInternAtom(display, "CLIPBOARD", False);
			utf8 = XInternAtom(display, "UTF8_STRING", False);
			targets = XInternAtom(display, "TARGETS", False);
			property = XInternAtom(display, "GacUI.TuiClipboard", False);
			incr = XInternAtom(display, "INCR", False);
			observedOwner = XGetSelectionOwner(display, clipboard);
			int eventBase, errorBase;
			if (XFixesQueryExtension(display, &eventBase, &errorBase))
			{
				selectionEvent = eventBase + XFixesSelectionNotify;
				XFixesSelectSelectionInput(display, window, clipboard,
					XFixesSetSelectionOwnerNotifyMask | XFixesSelectionWindowDestroyNotifyMask | XFixesSelectionClientCloseNotifyMask);
			}
		}

		~Impl()
		{
			if (display)
			{
				XDestroyWindow(display, window);
				XCloseDisplay(display);
			}
		}

		void ReadProperty()
		{
			Atom type;
			int format;
			unsigned long count, remaining;
			unsigned char* bytes = nullptr;
			auto result = XGetWindowProperty(display, window, property, 0, 0x1FFFFFFF, True, AnyPropertyType, &type, &format, &count, &remaining, &bytes);
			if (result == Success && type == incr)
			{
				incremental = true;
			}
			else if (result == Success && type == utf8 && format == 8 && remaining == 0)
			{
				received += AString::CopyFrom(reinterpret_cast<char*>(bytes), (vint)count);
				if (!incremental || count == 0)
				{
					waiting = false;
					succeeded = true;
				}
			}
			else waiting = false;
			if (bytes) XFree(bytes);
			XFlush(display);
		}

		void PumpEvents()
		{
			if (!display) return;
			if (selectionEvent == -1)
			{
				auto owner = XGetSelectionOwner(display, clipboard);
				clipboardChanged |= owner != observedOwner;
				observedOwner = owner;
			}
			while (XPending(display))
			{
				XEvent event;
				XNextEvent(display, &event);
				if (event.type == selectionEvent)
				{
					clipboardChanged = true;
				}
				else if (event.type == SelectionRequest)
				{
					auto& request = event.xselectionrequest;
					XEvent reply = {};
					reply.xselection = { SelectionNotify, 0, True, display, request.requestor, request.selection, request.target, 0, request.time };
					auto destination = request.property ? request.property : request.target;
					if (request.selection == clipboard && request.target == targets)
					{
						Atom supported[] = { targets, utf8 };
						XChangeProperty(display, request.requestor, destination, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(supported), 2);
						reply.xselection.property = destination;
					}
					else if (request.selection == clipboard && request.target == utf8)
					{
						auto text = wtou8(local.ReadClipboard()->GetText());
						if (text.Length() <= (XMaxRequestSize(display) - 64) * 4)
						{
							XChangeProperty(display, request.requestor, destination, utf8, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(text.Buffer()), (int)text.Length());
							reply.xselection.property = destination;
						}
					}
					XSendEvent(display, request.requestor, False, 0, &reply);
					XFlush(display);
				}
				else if (event.type == SelectionNotify && waiting && event.xselection.selection == clipboard)
				{
					if (event.xselection.property) ReadProperty();
					else waiting = false;
				}
				else if (event.type == PropertyNotify && waiting && incremental && event.xproperty.atom == property && event.xproperty.state == PropertyNewValue)
				{
					ReadProperty();
				}
			}
		}

		class Reader : public Object, public INativeClipboardReader
		{
		public:
			Nullable<WString>	text;
			bool ContainsText() override { return text; }
			WString GetText() override { return text ? text.Value() : WString(); }
			bool ContainsDocument() override { return false; }
			Ptr<DocumentModel> GetDocument() override { return nullptr; }
			bool ContainsImage() override { return false; }
			Ptr<INativeImage> GetImage() override { return nullptr; }
		};

		class Writer : public Object, public INativeClipboardWriter
		{
			Ptr<Impl>					owner;
			Ptr<INativeClipboardWriter>	writer;
		public:
			Writer(Ptr<Impl> _owner) : owner(_owner), writer(owner->local.WriteClipboard()) {}
			void SetText(const WString& value) override { writer->SetText(value); }
			void SetDocument(Ptr<DocumentModel> value) override { writer->SetDocument(value); }
			void SetImage(Ptr<INativeImage> value) override { writer->SetImage(value); }
			bool Submit() override
			{
				if (!writer->Submit()) return false;
				if (!owner->display) return true;
				XSetSelectionOwner(owner->display, owner->clipboard, owner->window, CurrentTime);
				XFlush(owner->display);
				return XGetSelectionOwner(owner->display, owner->clipboard) == owner->window;
			}
		};
	};

	TuiWGacClipboardService::TuiWGacClipboardService() : impl(Ptr(new Impl)) {}
	TuiWGacClipboardService::~TuiWGacClipboardService() {}

	bool TuiWGacClipboardService::PumpEvents()
	{
		impl->PumpEvents();
		auto changed = impl->clipboardChanged;
		impl->clipboardChanged = false;
		return changed;
	}

	Ptr<INativeClipboardReader> TuiWGacClipboardService::ReadClipboard()
	{
		if (!impl->display || XGetSelectionOwner(impl->display, impl->clipboard) == impl->window) return impl->local.ReadClipboard();
		auto external = Ptr(new Impl::Reader);
		impl->received = AString();
		impl->incremental = false;
		impl->succeeded = false;
		impl->waiting = true;
		XConvertSelection(impl->display, impl->clipboard, impl->utf8, impl->property, impl->window, CurrentTime);
		XFlush(impl->display);
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
		while (impl->waiting)
		{
			impl->PumpEvents();
			if (!impl->waiting) break;
			auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
			if (remaining <= 0) break;
			pollfd descriptor = { ConnectionNumber(impl->display), POLLIN, 0 };
			auto result = poll(&descriptor, 1, (int)remaining);
			CHECK_ERROR(result >= 0 || errno == EINTR, L"TuiWGacClipboardService#Failed to wait for clipboard selection.");
		}
		impl->waiting = false;
		if (impl->succeeded)
		{
			external->text = u8tow(U8String::CopyFrom(reinterpret_cast<const char8_t*>(impl->received.Buffer()), impl->received.Length()));
		}
		return external;
	}

	Ptr<INativeClipboardWriter> TuiWGacClipboardService::WriteClipboard()
	{
		return Ptr(new Impl::Writer(impl));
	}
}
