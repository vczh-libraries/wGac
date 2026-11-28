#include "TuiWGacClipboardService.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <poll.h>
#include <chrono>
#include <cerrno>
#include <algorithm>
#include <map>
#include <vector>

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
		Atom				document = 0;
		Atom				png = 0;
		Atom				requested = 0;
		std::vector<Atom>	available;
		using Bytes = Ptr<stream::MemoryStream>;
		std::map<Atom, Bytes> published;
		struct Transfer
		{
			Atom target;
			Bytes bytes;
			vint offset;
		};
		std::map<std::pair<::Window, Atom>, Transfer> outgoing;
		FakeClipboardService	local;
		Bytes				received;
		bool				waiting = false;
		bool				incremental = false;
		bool				succeeded = false;
		int					selectionEvent = -1;
		bool				clipboardChanged = false;
		::Window			observedOwner = 0;

		bool allowPrivateClipboard;

		Impl(bool allowPrivate) : allowPrivateClipboard(allowPrivate)
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
			document = XInternAtom(display, "application/x-gacui-document", False);
			png = XInternAtom(display, "image/png", False);
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
			else if (result == Success && requested == targets && type == XA_ATOM && format == 32 && remaining == 0)
			{
				if (count) available.insert(available.end(), reinterpret_cast<Atom*>(bytes), reinterpret_cast<Atom*>(bytes) + count);
				if (!incremental || count == 0) { waiting = false; succeeded = true; }
			}
			else if (result == Success && type == requested && format == 8 && remaining == 0)
			{
				if (count) received->Write(bytes, static_cast<vint>(count));
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

		vint ChunkSize() const
		{
			return static_cast<vint>((XMaxRequestSize(display) - 64) * 4);
		}

		void SendSelection(const XSelectionRequestEvent& request)
		{
			XEvent reply{};
			reply.xselection = { SelectionNotify, 0, True, display, request.requestor, request.selection, request.target, 0, request.time };
			auto destination = request.property ? request.property : request.target;
			if (request.selection == clipboard && request.target == targets)
			{
				std::vector<Atom> types{ targets };
				for (const auto& [type, bytes] : published) types.push_back(type);
				XChangeProperty(display, request.requestor, destination, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(types.data()), static_cast<int>(types.size()));
				reply.xselection.property = destination;
			}
			else if (auto it = published.find(request.target); request.selection == clipboard && it != published.end())
			{
				auto bytes = it->second;
				if (bytes->Size() <= ChunkSize())
				{
					XChangeProperty(display, request.requestor, destination, request.target, 8, PropModeReplace, static_cast<const unsigned char*>(bytes->GetInternalBuffer()), static_cast<int>(bytes->Size()));
				}
				else
				{
					unsigned long size = static_cast<unsigned long>(bytes->Size());
					XSelectInput(display, request.requestor, PropertyChangeMask | StructureNotifyMask);
					outgoing[{request.requestor, destination}] = { request.target, bytes, 0 };
					XChangeProperty(display, request.requestor, destination, incr, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&size), 1);
				}
				reply.xselection.property = destination;
			}
			XSendEvent(display, request.requestor, False, 0, &reply);
			XFlush(display);
		}

		void SendNextChunk(const XPropertyEvent& event)
		{
			auto it = outgoing.find({event.window, event.atom});
			if (it == outgoing.end()) return;
			auto& transfer = it->second;
			auto size = std::min(ChunkSize(), static_cast<vint>(transfer.bytes->Size()) - transfer.offset);
			auto data = static_cast<const unsigned char*>(transfer.bytes->GetInternalBuffer());
			XChangeProperty(display, event.window, event.atom, transfer.target, 8, PropModeReplace, data + transfer.offset, static_cast<int>(size));
			transfer.offset += size;
			if (size == 0) outgoing.erase(it);
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
					SendSelection(event.xselectionrequest);
				}
				else if (event.type == PropertyNotify && event.xproperty.state == PropertyDelete)
				{
					SendNextChunk(event.xproperty);
				}
				else if (event.type == DestroyNotify)
				{
					std::erase_if(outgoing, [&](const auto& item) { return item.first.first == event.xdestroywindow.window; });
				}
				else if (event.type == SelectionNotify && waiting && event.xselection.selection == clipboard && event.xselection.target == requested)
				{
					if (event.xselection.property) ReadProperty();
					else waiting = false;
				}
				else if (event.type == PropertyNotify && waiting && incremental && event.xproperty.window == window && event.xproperty.atom == property && event.xproperty.state == PropertyNewValue)
				{
					ReadProperty();
				}
			}
		}

		Bytes ReadSelection(Atom target)
		{
			requested = target;
			received = Ptr(new stream::MemoryStream);
			available.clear();
			incremental = succeeded = false;
			waiting = true;
			XConvertSelection(display, clipboard, target, property, window, CurrentTime);
			XFlush(display);
			auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
			while (waiting)
			{
				PumpEvents();
				if (!waiting) break;
				auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
				if (remaining <= 0) break;
				pollfd descriptor = { ConnectionNumber(display), POLLIN, 0 };
				auto result = poll(&descriptor, 1, static_cast<int>(remaining));
				CHECK_ERROR(result >= 0 || errno == EINTR, L"TuiWGacClipboardService#Failed to wait for clipboard selection.");
			}
			waiting = false;
			auto value = std::move(received);
			if (!succeeded) return nullptr;
			value->SeekFromBegin(0);
			return value;
		}

		class Reader : public Object, public INativeClipboardReader
		{
			Ptr<Impl> owner;
			::Window selectionOwner;
			std::vector<Atom> formats;

			bool Contains(Atom target) const
			{
				return std::find(formats.begin(), formats.end(), target) != formats.end();
			}

			Bytes Read(Atom target)
			{
				if (!Contains(target) || XGetSelectionOwner(owner->display, owner->clipboard) != selectionOwner) return nullptr;
				return owner->ReadSelection(target);
			}
		public:
			Reader(Ptr<Impl> _owner, ::Window _selectionOwner) : owner(_owner), selectionOwner(_selectionOwner)
			{
				if (selectionOwner && owner->ReadSelection(owner->targets)) formats = owner->available;
			}
			bool ContainsText() override { return Contains(owner->utf8); }
			WString GetText() override
			{
				auto bytes = Read(owner->utf8);
				if (!bytes || bytes->Size() == 0) return {};
				return u8tow(U8String::CopyFrom(static_cast<const char8_t*>(bytes->GetInternalBuffer()), static_cast<vint>(bytes->Size())));
			}
			bool ContainsDocument() override { return Contains(owner->document); }
			Ptr<DocumentModel> GetDocument() override
			{
				auto bytes = Read(owner->document);
				return bytes ? LoadDocumentFromClipboardStream(*bytes.Obj()) : nullptr;
			}
			bool ContainsImage() override { return Contains(owner->png); }
			Ptr<INativeImage> GetImage() override
			{
				auto bytes = Read(owner->png);
				return bytes ? GetCurrentController()->ImageService()->CreateImageFromStream(*bytes.Obj()) : nullptr;
			}
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
				if (!owner->display && !owner->allowPrivateClipboard) return false;
				if (!writer->Submit()) return false;
				if (!owner->display) return true;
				auto data = owner->local.ReadClipboard();
				owner->published.clear();
				if (data->ContainsText())
				{
					auto text = wtou8(data->GetText());
					auto bytes = Ptr(new stream::MemoryStream);
					if (text.Length()) bytes->Write(const_cast<char8_t*>(text.Buffer()), text.Length());
					owner->published[owner->utf8] = bytes;
				}
				if (data->ContainsDocument())
				{
					auto bytes = Ptr(new stream::MemoryStream);
					SaveDocumentToClipboardStream(data->GetDocument(), *bytes.Obj());
					owner->published[owner->document] = bytes;
				}
				if (data->ContainsImage())
				{
					auto bytes = Ptr(new stream::MemoryStream);
					data->GetImage()->SaveToStream(*bytes.Obj(), INativeImage::Png);
					owner->published[owner->png] = bytes;
				}
				XSetSelectionOwner(owner->display, owner->clipboard, owner->window, CurrentTime);
				XFlush(owner->display);
				return XGetSelectionOwner(owner->display, owner->clipboard) == owner->window;
			}
		};
	};

	TuiWGacClipboardService::TuiWGacClipboardService(bool allowPrivateClipboard) : impl(Ptr(new Impl(allowPrivateClipboard))) {}
	TuiWGacClipboardService::~TuiWGacClipboardService() {}

	bool TuiWGacClipboardService::PumpClipboardEvents()
	{
		impl->PumpEvents();
		auto changed = impl->clipboardChanged;
		impl->clipboardChanged = false;
		return changed;
	}

	Ptr<INativeClipboardReader> TuiWGacClipboardService::ReadClipboard()
	{
		if (!impl->display) return impl->local.ReadClipboard();
		auto owner = XGetSelectionOwner(impl->display, impl->clipboard);
		if (owner == impl->window) return impl->local.ReadClipboard();
		return Ptr(new Impl::Reader(impl, owner));
	}

	Ptr<INativeClipboardWriter> TuiWGacClipboardService::WriteClipboard()
	{
		return Ptr(new Impl::Writer(impl));
	}
}
