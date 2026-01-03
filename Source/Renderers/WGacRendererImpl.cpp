#include "WGacRenderer.h"
#include "../WGacController.h"
#include "../WGacNativeWindow.h"
#include "../WGacGacView.h"
#include "../Wayland/WaylandDisplay.h"
#include "../Services/WGacImageService.h"
#include <functional>

using namespace vl::collections;

namespace vl {
namespace presentation {
namespace elements {
namespace wgac {

// Forward declaration
class WGacLayoutProvider;

// WGacParagraph - Pango-based paragraph implementation with proper Unicode handling
class WGacParagraph : public Object, public IGuiGraphicsParagraph
{
protected:
    WGacLayoutProvider* provider;
    IGuiGraphicsRenderTarget* renderTarget;
    IGuiGraphicsParagraphCallback* paragraphCallback;
    WString text;
    AString utf8Text;  // Cached UTF-8 version
    PangoLayout* layout;
    cairo_surface_t* layoutSurface;
    cairo_t* layoutCr;

    bool wrapLine;
    vint maxWidth;
    Alignment paragraphAlignment;

    // Cached alignment offset from last Render() call
    vint lastAlignOffsetX;
    vint lastRenderWidth;

    vint caretPos;
    Color caretColor;
    bool caretVisible;
    bool caretFrontSide;

    // Text formatting - fragment-based like Uniscribe
    struct TextFragment
    {
        vint start;          // Character position
        vint length;         // Character length
        WString fontFamily;
        vint fontSize;
        bool bold;
        bool italic;
        bool underline;
        bool strikeline;
        Color textColor;
        Color backgroundColor;
        bool hasBackgroundColor;

        TextFragment()
            : start(0), length(0), fontSize(12)
            , bold(false), italic(false), underline(false), strikeline(false)
            , textColor(0, 0, 0), backgroundColor(255, 255, 255), hasBackgroundColor(false)
        {}
    };

    // Inline object tracking
    struct InlineObject
    {
        vint start;
        vint length;
        InlineObjectProperties properties;
        Rect cachedBounds;  // Cached bounds during last layout

        InlineObject()
            : start(0), length(0)
        {}
    };

    List<TextFragment> fragments;
    List<InlineObject> inlineObjects;
    FontProperties defaultFont;

    // Byte position cache for fast conversion
    Array<vint> charToByteMap;  // char index -> byte offset
    Array<vint> byteToCharMap;  // byte offset -> char index (sparse, only valid at char boundaries)

    void BuildPositionMaps()
    {
        charToByteMap.Resize(text.Length() + 1);
        utf8Text = wtoa(text);

        vint byteOffset = 0;
        for (vint i = 0; i < text.Length(); i++)
        {
            charToByteMap[i] = byteOffset;
            // Calculate UTF-8 byte length for this character
            wchar_t ch = text[i];
            if (ch < 0x80) byteOffset += 1;
            else if (ch < 0x800) byteOffset += 2;
            else if (ch >= 0xD800 && ch <= 0xDBFF)
            {
                // Surrogate pair - handle as 4 bytes
                byteOffset += 4;
                if (i + 1 < text.Length()) i++; // Skip low surrogate
                charToByteMap[i] = byteOffset;
            }
            else byteOffset += 3;
        }
        charToByteMap[text.Length()] = byteOffset;

        // Build reverse map
        byteToCharMap.Resize(utf8Text.Length() + 1);
        for (vint i = 0; i <= text.Length(); i++)
        {
            if (charToByteMap[i] <= utf8Text.Length())
            {
                byteToCharMap[charToByteMap[i]] = i;
            }
        }
    }

    vint CharToBytePos(vint charPos) const
    {
        if (charPos <= 0) return 0;
        if (charPos >= text.Length()) return utf8Text.Length();
        return charToByteMap[charPos];
    }

    vint ByteToCharPos(vint bytePos) const
    {
        if (bytePos <= 0) return 0;
        if (bytePos >= utf8Text.Length()) return text.Length();

        // Find the character containing this byte
        for (vint i = 0; i < text.Length(); i++)
        {
            if (charToByteMap[i] <= bytePos && bytePos < charToByteMap[i + 1])
            {
                return i;
            }
        }
        return text.Length();
    }

    void RebuildLayout()
    {
        if (!layout) return;

        pango_layout_set_text(layout, utf8Text.Buffer(), utf8Text.Length());

        if (wrapLine && maxWidth > 0)
        {
            // Wrap mode: set width for both wrapping and alignment
            pango_layout_set_width(layout, maxWidth * PANGO_SCALE);
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

            PangoAlignment pangoAlign = PANGO_ALIGN_LEFT;
            switch (paragraphAlignment)
            {
                case Alignment::Left: pangoAlign = PANGO_ALIGN_LEFT; break;
                case Alignment::Center: pangoAlign = PANGO_ALIGN_CENTER; break;
                case Alignment::Right: pangoAlign = PANGO_ALIGN_RIGHT; break;
            }
            pango_layout_set_alignment(layout, pangoAlign);
        }
        else
        {
            // No wrap: use infinite width, alignment handled in Render()
            pango_layout_set_width(layout, -1);
            pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        }

        // Build attributes from fragments
        PangoAttrList* attrList = pango_attr_list_new();

        for (vint i = 0; i < fragments.Count(); i++)
        {
            TextFragment& frag = fragments[i];
            if (frag.length <= 0) continue;

            guint startByte = CharToBytePos(frag.start);
            guint endByte = CharToBytePos(frag.start + frag.length);

            AString fontFamily = wtoa(frag.fontFamily.Length() > 0 ? frag.fontFamily : defaultFont.fontFamily);

            PangoAttribute* attr;

            // Font family
            attr = pango_attr_family_new(fontFamily.Buffer());
            attr->start_index = startByte;
            attr->end_index = endByte;
            pango_attr_list_insert(attrList, attr);

            // Font size (use absolute size to match font description)
            attr = pango_attr_size_new_absolute((frag.fontSize > 0 ? frag.fontSize : defaultFont.size) * PANGO_SCALE);
            attr->start_index = startByte;
            attr->end_index = endByte;
            pango_attr_list_insert(attrList, attr);

            // Bold
            if (frag.bold)
            {
                attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
                attr->start_index = startByte;
                attr->end_index = endByte;
                pango_attr_list_insert(attrList, attr);
            }

            // Italic
            if (frag.italic)
            {
                attr = pango_attr_style_new(PANGO_STYLE_ITALIC);
                attr->start_index = startByte;
                attr->end_index = endByte;
                pango_attr_list_insert(attrList, attr);
            }

            // Underline
            if (frag.underline)
            {
                attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
                attr->start_index = startByte;
                attr->end_index = endByte;
                pango_attr_list_insert(attrList, attr);
            }

            // Strikethrough
            if (frag.strikeline)
            {
                attr = pango_attr_strikethrough_new(TRUE);
                attr->start_index = startByte;
                attr->end_index = endByte;
                pango_attr_list_insert(attrList, attr);
            }

            // Text color
            attr = pango_attr_foreground_new(frag.textColor.r * 257, frag.textColor.g * 257, frag.textColor.b * 257);
            attr->start_index = startByte;
            attr->end_index = endByte;
            pango_attr_list_insert(attrList, attr);

            // Background color
            if (frag.hasBackgroundColor)
            {
                attr = pango_attr_background_new(frag.backgroundColor.r * 257,
                                                  frag.backgroundColor.g * 257,
                                                  frag.backgroundColor.b * 257);
                attr->start_index = startByte;
                attr->end_index = endByte;
                pango_attr_list_insert(attrList, attr);
            }
        }

        // Add shape attributes for inline objects
        // GacUI uses placeholder text like "[Image]" (7 chars) or "[EmbeddedObject]" (16 chars)
        // for inline objects. The shape attribute applies to each character individually,
        // so we need to distribute the width across all characters in the range.
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];
            if (obj.length <= 0) continue;

            guint startByte = CharToBytePos(obj.start);
            guint endByte = CharToBytePos(obj.start + obj.length);

            // Calculate how many UTF-8 bytes are in this range
            vint byteLength = endByte - startByte;
            if (byteLength <= 0) continue;

            // Use the size from properties (convert to Pango units)
            int totalWidth = (int)(obj.properties.size.x * PANGO_SCALE);
            int height = (int)(obj.properties.size.y * PANGO_SCALE);

            // Baseline: distance from top of object to the text baseline
            // If baseline is -1, the baseline is at the bottom of the object
            int baseline;
            if (obj.properties.baseline < 0)
            {
                baseline = height;  // Baseline at bottom
            }
            else
            {
                baseline = height - (int)(obj.properties.baseline * PANGO_SCALE);
            }

            // Apply shape attribute to each byte in the range
            // The first byte gets the full shape dimensions,
            // subsequent bytes get zero width to avoid accumulating space
            for (vint b = startByte; b < (vint)endByte; b++)
            {
                PangoRectangle inkRect;
                PangoRectangle logicalRect;

                if (b == (vint)startByte)
                {
                    // First byte: full width and height
                    inkRect.x = 0;
                    inkRect.y = -baseline;
                    inkRect.width = totalWidth;
                    inkRect.height = height;
                }
                else
                {
                    // Subsequent bytes: zero width, same height (for line height calculation)
                    inkRect.x = 0;
                    inkRect.y = -baseline;
                    inkRect.width = 0;
                    inkRect.height = height;
                }
                logicalRect = inkRect;

                PangoAttribute* attr = pango_attr_shape_new(&inkRect, &logicalRect);
                attr->start_index = b;
                attr->end_index = b + 1;
                pango_attr_list_insert(attrList, attr);
            }

            // Make the placeholder text invisible (fully transparent)
            // so it doesn't render on top of the inline object
            PangoAttribute* fgAttr = pango_attr_foreground_alpha_new(0);
            fgAttr->start_index = startByte;
            fgAttr->end_index = endByte;
            pango_attr_list_insert(attrList, fgAttr);
        }

        pango_layout_set_attributes(layout, attrList);
        pango_attr_list_unref(attrList);
    }

    // Fragment manipulation helpers (like Uniscribe's CutFragment)
    void SplitFragmentAt(vint position)
    {
        for (vint i = 0; i < fragments.Count(); i++)
        {
            TextFragment& frag = fragments[i];
            if (frag.start < position && position < frag.start + frag.length)
            {
                // Need to split this fragment
                TextFragment newFrag = frag;
                newFrag.start = position;
                newFrag.length = frag.start + frag.length - position;
                frag.length = position - frag.start;
                fragments.Insert(i + 1, newFrag);
                return;
            }
        }
    }

    void ApplyStyleToRange(vint start, vint length, std::function<void(TextFragment&)> modifier)
    {
        if (length <= 0) return;
        vint end = start + length;

        // First, split fragments at boundaries if needed
        SplitFragmentAt(start);
        SplitFragmentAt(end);

        // Then apply the modifier to all fragments in range
        for (vint i = 0; i < fragments.Count(); i++)
        {
            TextFragment& frag = fragments[i];
            if (frag.start >= start && frag.start + frag.length <= end)
            {
                modifier(frag);
            }
        }
    }

    // Get line info for a character position
    bool GetLineFromCharPos(vint charPos, int& lineIndex, int& lineStartChar, int& lineEndChar)
    {
        if (!layout) return false;

        vint bytePos = CharToBytePos(charPos);
        int xPos;
        pango_layout_index_to_line_x(layout, bytePos, FALSE, &lineIndex, &xPos);

        PangoLayoutIter* iter = pango_layout_get_iter(layout);
        int currentLine = 0;
        do
        {
            if (currentLine == lineIndex)
            {
                PangoLayoutLine* line = pango_layout_iter_get_line_readonly(iter);
                lineStartChar = ByteToCharPos(line->start_index);
                lineEndChar = ByteToCharPos(line->start_index + line->length);
                pango_layout_iter_free(iter);
                return true;
            }
            currentLine++;
        } while (pango_layout_iter_next_line(iter));

        pango_layout_iter_free(iter);
        return false;
    }

    // Get caret position from X coordinate on a specific line
    vint GetCaretFromXWithLine(int x, int lineIndex)
    {
        PangoLayoutLine* line = pango_layout_get_line_readonly(layout, lineIndex);
        if (!line) return -1;

        int index, trailing;
        pango_layout_line_x_to_index(line, x, &index, &trailing);

        vint charPos = ByteToCharPos(index);
        if (trailing > 0) charPos++;
        return charPos;
    }

public:
    WGacParagraph(WGacLayoutProvider* _provider, const WString& _text,
                  IGuiGraphicsRenderTarget* _renderTarget,
                  IGuiGraphicsParagraphCallback* _callback)
        : provider(_provider)
        , renderTarget(_renderTarget)
        , paragraphCallback(_callback)
        , text(_text)
        , layout(nullptr)
        , layoutSurface(nullptr)
        , layoutCr(nullptr)
        , wrapLine(false)
        , maxWidth(-1)
        , paragraphAlignment(Alignment::Left)
        , lastAlignOffsetX(0)
        , lastRenderWidth(0)
        , caretPos(-1)
        , caretVisible(false)
        , caretFrontSide(false)
    {
        defaultFont = GetCurrentController()->ResourceService()->GetDefaultFont();

        // Build position maps for UTF-8 conversion
        BuildPositionMaps();

        // Create a dummy surface for layout calculations
        layoutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        layoutCr = cairo_create(layoutSurface);
        layout = pango_cairo_create_layout(layoutCr);

        // Set default font
        PangoFontDescription* fontDesc = pango_font_description_new();
        AString family = wtoa(defaultFont.fontFamily);
        pango_font_description_set_family(fontDesc, family.Buffer());
        pango_font_description_set_absolute_size(fontDesc, defaultFont.size * PANGO_SCALE);
        pango_layout_set_font_description(layout, fontDesc);
        pango_font_description_free(fontDesc);

        // Initialize with a single fragment covering all text
        if (text.Length() > 0)
        {
            TextFragment frag;
            frag.start = 0;
            frag.length = text.Length();
            frag.fontFamily = defaultFont.fontFamily;
            frag.fontSize = defaultFont.size;
            frag.textColor = Color(0, 0, 0);
            fragments.Add(frag);
        }

        RebuildLayout();
    }

    ~WGacParagraph()
    {
        if (layout) g_object_unref(layout);
        if (layoutCr) cairo_destroy(layoutCr);
        if (layoutSurface) cairo_surface_destroy(layoutSurface);
    }

    IGuiGraphicsLayoutProvider* GetProvider() override;

    IGuiGraphicsRenderTarget* GetRenderTarget() override { return renderTarget; }

    bool GetWrapLine() override { return wrapLine; }

    void SetWrapLine(bool value) override
    {
        if (wrapLine != value)
        {
            wrapLine = value;
            RebuildLayout();
        }
    }

    vint GetMaxWidth() override { return maxWidth; }

    void SetMaxWidth(vint value) override
    {
        if (maxWidth != value)
        {
            maxWidth = value;
            RebuildLayout();
        }
    }

    Alignment GetParagraphAlignment() override { return paragraphAlignment; }

    void SetParagraphAlignment(Alignment value) override
    {
        if (paragraphAlignment != value)
        {
            paragraphAlignment = value;
            RebuildLayout();
        }
    }

    bool SetFont(vint start, vint length, const WString& value) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        ApplyStyleToRange(start, length, [&](TextFragment& frag) { frag.fontFamily = value; });
        RebuildLayout();
        return true;
    }

    bool SetSize(vint start, vint length, vint value) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        ApplyStyleToRange(start, length, [&](TextFragment& frag) { frag.fontSize = value; });
        RebuildLayout();
        return true;
    }

    bool SetStyle(vint start, vint length, TextStyle value) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        ApplyStyleToRange(start, length, [&](TextFragment& frag) {
            frag.bold = (value & Bold) != 0;
            frag.italic = (value & Italic) != 0;
            frag.underline = (value & Underline) != 0;
            frag.strikeline = (value & Strikeline) != 0;
        });
        RebuildLayout();
        return true;
    }

    bool SetColor(vint start, vint length, Color value) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        ApplyStyleToRange(start, length, [&](TextFragment& frag) { frag.textColor = value; });
        RebuildLayout();
        return true;
    }

    bool SetBackgroundColor(vint start, vint length, Color value) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        ApplyStyleToRange(start, length, [&](TextFragment& frag) {
            frag.backgroundColor = value;
            frag.hasBackgroundColor = (value.a != 0);
        });
        RebuildLayout();
        return true;
    }

    bool SetInlineObject(vint start, vint length, const InlineObjectProperties& properties) override
    {
        if (length == 0) return true;
        if (start < 0 || start + length > text.Length()) return false;

        // Check if this range overlaps with existing inline objects
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];
            if (start < obj.start + obj.length && obj.start < start + length)
            {
                // Overlapping range - fail
                return false;
            }
        }

        // Add new inline object
        InlineObject newObj;
        newObj.start = start;
        newObj.length = length;
        newObj.properties = properties;
        inlineObjects.Add(newObj);

        // Set render target on background image if present
        if (properties.backgroundImage)
        {
            IGuiGraphicsRenderer* renderer = properties.backgroundImage->GetRenderer();
            if (renderer)
            {
                renderer->SetRenderTarget(renderTarget);
            }
        }

        RebuildLayout();
        return true;
    }

    bool ResetInlineObject(vint start, vint length) override
    {
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];
            if (obj.start == start && obj.length == length)
            {
                // Clear render target on background image if present
                if (obj.properties.backgroundImage)
                {
                    IGuiGraphicsRenderer* renderer = obj.properties.backgroundImage->GetRenderer();
                    if (renderer)
                    {
                        renderer->SetRenderTarget(nullptr);
                    }
                }
                inlineObjects.RemoveAt(i);
                RebuildLayout();
                return true;
            }
        }
        return false;
    }

    Size GetSize() override
    {
        if (!layout) return Size(0, 0);
        int width, height;
        pango_layout_get_pixel_size(layout, &width, &height);
        // Add 2 pixels for the caret at the end of text
        // This ensures the document width is slightly larger than the text width,
        // so EnsureRectVisible doesn't skip scrolling when caret is at the end.
        // The extra pixel provides a buffer for rounding errors.
        return Size(width + 2, height);
    }

    bool OpenCaret(vint caret, Color color, bool frontSide) override
    {
        caretPos = caret;
        caretColor = color;
        caretVisible = true;
        caretFrontSide = frontSide;
        return true;
    }

    bool CloseCaret() override
    {
        caretVisible = false;
        caretPos = -1;
        return true;
    }

    void Render(Rect bounds) override
    {
        auto* target = dynamic_cast<IWGacRenderTarget*>(renderTarget);
        if (!target) return;

        cairo_t* cr = target->GetCairoContext();
        if (!cr || !layout) return;

        cairo_save(cr);

        // Calculate alignment offset when not wrapping
        vint alignOffsetX = 0;
        vint availableWidth = bounds.x2 - bounds.x1;

        PangoRectangle logicalRect;
        pango_layout_get_pixel_extents(layout, nullptr, &logicalRect);
        vint textWidth = logicalRect.width;

        // Use the available width (from bounds) for alignment
        vint alignWidth = availableWidth;

        if (!wrapLine && alignWidth > textWidth)
        {
            switch (paragraphAlignment)
            {
                case Alignment::Center:
                    alignOffsetX = (alignWidth - textWidth) / 2;
                    break;
                case Alignment::Right:
                    alignOffsetX = alignWidth - textWidth;
                    break;
                default: // Left
                    break;
            }
        }

        // Cache for GetCaretBounds/GetCaretFromPoint
        lastAlignOffsetX = alignOffsetX;
        lastRenderWidth = alignWidth;

        // Render the text
        cairo_move_to(cr, bounds.x1 + alignOffsetX, bounds.y1);
        pango_cairo_show_layout(cr, layout);

        // Render inline objects
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];

            // Get the position of this inline object from the layout
            vint bytePos = CharToBytePos(obj.start);
            PangoRectangle pos;
            pango_layout_index_to_pos(layout, bytePos, &pos);

            // Convert to pixel coordinates and apply bounds offset with alignment
            int objX = bounds.x1 + alignOffsetX + pos.x / PANGO_SCALE;
            int objY = bounds.y1 + pos.y / PANGO_SCALE;
            int objWidth = pos.width / PANGO_SCALE;
            int objHeight = pos.height / PANGO_SCALE;

            // Cache the bounds for hit testing (includes alignment offset)
            obj.cachedBounds = Rect(Point(alignOffsetX + pos.x / PANGO_SCALE, pos.y / PANGO_SCALE),
                                    Size(objWidth, objHeight));

            // Render background image if present
            if (obj.properties.backgroundImage)
            {
                IGuiGraphicsRenderer* graphicsRenderer = obj.properties.backgroundImage->GetRenderer();
                if (graphicsRenderer)
                {
                    graphicsRenderer->Render(Rect(Point(objX, objY), Size(objWidth, objHeight)));
                }
            }

            // Call callback if present
            if (obj.properties.callbackId != -1 && paragraphCallback)
            {
                // Location is relative to paragraph origin (includes alignment offset)
                Rect location(Point(alignOffsetX + pos.x / PANGO_SCALE, pos.y / PANGO_SCALE), Size(objWidth, objHeight));
                Size newSize = paragraphCallback->OnRenderInlineObject(obj.properties.callbackId, location);
                // Update the properties with new size if it changed
                if (newSize.x != obj.properties.size.x || newSize.y != obj.properties.size.y)
                {
                    obj.properties.size = newSize;
                }
            }
        }

        // Render caret if visible
        if (caretVisible && caretPos >= 0)
        {
            PangoRectangle strongPos, weakPos;
            vint bytePos = CharToBytePos(caretPos);
            pango_layout_get_cursor_pos(layout, bytePos, &strongPos, &weakPos);

            int cx = bounds.x1 + alignOffsetX + strongPos.x / PANGO_SCALE;
            int cy = bounds.y1 + strongPos.y / PANGO_SCALE;
            int ch = strongPos.height / PANGO_SCALE;

            cairo_set_source_rgba(cr, caretColor.r / 255.0, caretColor.g / 255.0,
                                  caretColor.b / 255.0, caretColor.a / 255.0);
            cairo_set_line_width(cr, 1);
            cairo_move_to(cr, cx + 0.5, cy);
            cairo_line_to(cr, cx + 0.5, cy + ch);
            cairo_stroke(cr);
        }

        cairo_restore(cr);
    }

    vint GetCaret(vint comparingCaret, CaretRelativePosition position, bool& preferFrontSide) override
    {
        if (!layout) return -1;

        vint textLen = text.Length();

        if (position == CaretFirst)
        {
            preferFrontSide = false;
            return 0;
        }
        if (position == CaretLast)
        {
            preferFrontSide = true;
            return textLen;
        }
        if (!IsValidCaret(comparingCaret)) return -1;

        switch (position)
        {
            case CaretLineFirst:
            {
                int lineIndex, lineStart, lineEnd;
                if (GetLineFromCharPos(comparingCaret, lineIndex, lineStart, lineEnd))
                {
                    preferFrontSide = false;
                    return lineStart;
                }
                return comparingCaret;
            }
            case CaretLineLast:
            {
                int lineIndex, lineStart, lineEnd;
                if (GetLineFromCharPos(comparingCaret, lineIndex, lineStart, lineEnd))
                {
                    preferFrontSide = true;
                    return lineEnd;
                }
                return comparingCaret;
            }
            case CaretMoveLeft:
            {
                if (comparingCaret == 0) return 0;
                // Use visual cursor movement for proper BiDi support
                vint bytePos = CharToBytePos(comparingCaret);
                int newIndex, newTrailing;
                pango_layout_move_cursor_visually(layout, TRUE, bytePos, 0, -1, &newIndex, &newTrailing);
                if (newIndex < 0) return 0;
                if (newIndex >= (int)utf8Text.Length()) return textLen;
                preferFrontSide = false;
                return ByteToCharPos(newIndex) + (newTrailing > 0 ? 1 : 0);
            }
            case CaretMoveRight:
            {
                if (comparingCaret >= textLen) return textLen;
                // Use visual cursor movement for proper BiDi support
                vint bytePos = CharToBytePos(comparingCaret);
                int newIndex, newTrailing;
                pango_layout_move_cursor_visually(layout, TRUE, bytePos, 0, 1, &newIndex, &newTrailing);
                if (newIndex < 0) return 0;
                if (newIndex >= (int)utf8Text.Length()) return textLen;
                preferFrontSide = true;
                return ByteToCharPos(newIndex) + (newTrailing > 0 ? 1 : 0);
            }
            case CaretMoveUp:
            {
                int lineIndex, lineStart, lineEnd;
                if (!GetLineFromCharPos(comparingCaret, lineIndex, lineStart, lineEnd)) return comparingCaret;

                if (lineIndex == 0) return comparingCaret;  // Already on first line

                // Get X position of current caret
                Rect caretBounds = GetCaretBounds(comparingCaret, preferFrontSide);
                preferFrontSide = true;
                return GetCaretFromXWithLine(caretBounds.x1 * PANGO_SCALE, lineIndex - 1);
            }
            case CaretMoveDown:
            {
                int lineIndex, lineStart, lineEnd;
                if (!GetLineFromCharPos(comparingCaret, lineIndex, lineStart, lineEnd)) return comparingCaret;

                int lineCount = pango_layout_get_line_count(layout);
                if (lineIndex >= lineCount - 1) return comparingCaret;  // Already on last line

                // Get X position of current caret
                Rect caretBounds = GetCaretBounds(comparingCaret, preferFrontSide);
                preferFrontSide = false;
                return GetCaretFromXWithLine(caretBounds.x1 * PANGO_SCALE, lineIndex + 1);
            }
            default:
                break;
        }
        return -1;
    }

    Rect GetCaretBounds(vint caret, bool frontSide) override
    {
        if (!layout) return Rect();
        if (!IsValidCaret(caret)) return Rect();
        if (text.Length() == 0)
        {
            Size s = GetSize();
            // For empty text, position based on alignment
            vint x = lastAlignOffsetX;
            if (lastRenderWidth > 0)
            {
                switch (paragraphAlignment)
                {
                    case Alignment::Center:
                        x = lastRenderWidth / 2;
                        break;
                    case Alignment::Right:
                        x = lastRenderWidth;
                        break;
                    default:
                        x = 0;
                        break;
                }
            }
            return Rect(Point(x, 0), Size(0, s.y > 0 ? s.y : defaultFont.size));
        }

        PangoRectangle strongPos, weakPos;
        vint bytePos = CharToBytePos(caret);
        pango_layout_get_cursor_pos(layout, bytePos, &strongPos, &weakPos);

        // Use strong position for LTR, could use weak for RTL context
        // Add alignment offset
        return Rect(
            lastAlignOffsetX + strongPos.x / PANGO_SCALE,
            strongPos.y / PANGO_SCALE,
            lastAlignOffsetX + strongPos.x / PANGO_SCALE + 1,
            (strongPos.y + strongPos.height) / PANGO_SCALE
        );
    }

    vint GetCaretFromPoint(Point point) override
    {
        if (!layout) return -1;

        // Adjust point for alignment offset
        vint adjustedX = point.x - lastAlignOffsetX;
        if (adjustedX < 0) adjustedX = 0;

        int index, trailing;
        pango_layout_xy_to_index(layout, adjustedX * PANGO_SCALE, point.y * PANGO_SCALE, &index, &trailing);

        vint charPos = ByteToCharPos(index);
        if (trailing > 0) charPos++;
        return charPos;
    }

    Nullable<InlineObjectProperties> GetInlineObjectFromPoint(Point point, vint& start, vint& length) override
    {
        start = -1;
        length = 0;

        if (!layout) return {};

        // First, use Pango to find the text position at this point
        int index, trailing;
        pango_layout_xy_to_index(layout, point.x * PANGO_SCALE, point.y * PANGO_SCALE, &index, &trailing);
        vint charPos = ByteToCharPos(index);

        // Check if this position falls within any inline object
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];
            if (charPos >= obj.start && charPos < obj.start + obj.length)
            {
                // Found matching inline object
                start = obj.start;
                length = obj.length;
                return obj.properties;
            }
        }

        // Also check using cached bounds for better hit testing
        for (vint i = 0; i < inlineObjects.Count(); i++)
        {
            InlineObject& obj = inlineObjects[i];
            if (obj.cachedBounds.Contains(point))
            {
                start = obj.start;
                length = obj.length;
                return obj.properties;
            }
        }

        return {};
    }

    vint GetNearestCaretFromTextPos(vint textPos, bool frontSide) override
    {
        if (textPos < 0) return 0;
        if (textPos > text.Length()) return text.Length();
        // All positions are valid carets in simple text
        return textPos;
    }

    bool IsValidCaret(vint caret) override
    {
        return caret >= 0 && caret <= text.Length();
    }

    bool IsValidTextPos(vint textPos) override
    {
        return textPos >= 0 && textPos <= text.Length();
    }
};

// WGacLayoutProvider implementation
class WGacLayoutProvider : public Object, public IGuiGraphicsLayoutProvider
{
public:
    Ptr<IGuiGraphicsParagraph> CreateParagraph(const WString& text,
                                                IGuiGraphicsRenderTarget* renderTarget,
                                                IGuiGraphicsParagraphCallback* callback) override
    {
        return Ptr(new WGacParagraph(this, text, renderTarget, callback));
    }
};

IGuiGraphicsLayoutProvider* WGacParagraph::GetProvider()
{
    return provider;
}

} // namespace wgac
} // namespace elements
} // namespace presentation
} // namespace vl

namespace vl {
namespace presentation {
namespace elements {
namespace wgac {

namespace {
    IWGacRenderTarget* g_currentRenderTarget = nullptr;
    IWGacObjectProvider* g_wGacObjectProvider = nullptr;
    IWGacResourceManager* g_wGacResourceManager = nullptr;
}

// WGacRenderTarget implementation
class WGacRenderTarget : public IWGacRenderTarget
{
protected:
    wayland::WGacNativeWindow* window;
    wayland::WGacView* view;
    List<Rect> clippers;
    vint clipperCoverWholeTargetCounter;
    bool movedWhileRendering;

public:
    WGacRenderTarget(INativeWindow* _window)
        : clipperCoverWholeTargetCounter(0)
        , movedWhileRendering(false)
    {
        window = dynamic_cast<wayland::WGacNativeWindow*>(_window);
        if (window) {
            view = window->GetGacView();
        }
    }

    void StartRendering() override
    {
        if (view) {
            view->StartRendering();
        }
        SetCurrentRenderTarget(this);
        cairo_t* cr = GetCairoContext();
        if (cr) {
            cairo_save(cr);
            // Apply HiDPI scaling
            vl::presentation::wayland::WaylandDisplay* wlDisplay = vl::presentation::wayland::GetWaylandDisplay();
            if (wlDisplay) {
                int32_t scale = wlDisplay->GetOutputScale();
                if (scale > 1) {
                    cairo_scale(cr, scale, scale);
                }
            }
        }
    }

    RenderTargetFailure StopRendering() override
    {
        cairo_t* cr = GetCairoContext();
        if (cr) {
            cairo_restore(cr);
        }
        if (view) {
            view->StopRendering();
        }
        // Commit the buffer to Wayland surface after rendering
        if (window) {
            window->CommitBuffer();
        }
        SetCurrentRenderTarget(nullptr);
        bool moved = movedWhileRendering;
        movedWhileRendering = false;
        return moved ? RenderTargetFailure::ResizeWhileRendering : RenderTargetFailure::None;
    }

    void PushClipper(Rect clipper, reflection::DescriptableObject* generator) override
    {
        if (clipperCoverWholeTargetCounter > 0) {
            clipperCoverWholeTargetCounter++;
        } else {
            Rect previousClipper = GetClipper();
            Rect currentClipper;
            currentClipper.x1 = previousClipper.x1 > clipper.x1 ? previousClipper.x1 : clipper.x1;
            currentClipper.y1 = previousClipper.y1 > clipper.y1 ? previousClipper.y1 : clipper.y1;
            currentClipper.x2 = previousClipper.x2 < clipper.x2 ? previousClipper.x2 : clipper.x2;
            currentClipper.y2 = previousClipper.y2 < clipper.y2 ? previousClipper.y2 : clipper.y2;

            if (currentClipper.x1 < currentClipper.x2 && currentClipper.y1 < currentClipper.y2) {
                clippers.Add(currentClipper);
                cairo_t* cr = GetCairoContext();
                if (cr) {
                    cairo_save(cr);
                    cairo_rectangle(cr, currentClipper.Left(), currentClipper.Top(),
                                    currentClipper.Width(), currentClipper.Height());
                    cairo_clip(cr);
                }
            } else {
                clipperCoverWholeTargetCounter++;
            }
        }
    }

    void PopClipper(reflection::DescriptableObject* generator) override
    {
        if (clippers.Count() > 0) {
            if (clipperCoverWholeTargetCounter > 0) {
                clipperCoverWholeTargetCounter--;
            } else {
                clippers.RemoveAt(clippers.Count() - 1);
                cairo_t* cr = GetCairoContext();
                if (cr) {
                    cairo_restore(cr);
                }
            }
        }
    }

    Rect GetClipper() override
    {
        if (clippers.Count() == 0) {
            if (window) {
                auto size = window->Convert(window->GetClientSize());
                return Rect(Point(0, 0), size);
            }
            return Rect(0, 0, 800, 600);
        } else {
            return clippers[clippers.Count() - 1];
        }
    }

    bool IsClipperCoverWholeTarget() override
    {
        return clipperCoverWholeTargetCounter > 0;
    }

    cairo_t* GetCairoContext() override
    {
        return view ? view->GetCairoContext() : nullptr;
    }

    bool IsInHostedRendering() override { return false; }
    void StartHostedRendering() override {}
    RenderTargetFailure StopHostedRendering() override { return RenderTargetFailure::None; }

    void SetMovedWhileRendering() { movedWhileRendering = true; }
};

// WGacObjectProvider implementation
class WGacObjectProvider : public IWGacObjectProvider
{
public:
    void RecreateRenderTarget(INativeWindow* window) override {}

    IWGacRenderTarget* GetWGacRenderTarget(INativeWindow* window) override
    {
        auto* wgacWindow = dynamic_cast<wayland::WGacNativeWindow*>(window);
        if (wgacWindow) {
            return dynamic_cast<IWGacRenderTarget*>(wgacWindow->GetGraphicsHandler());
        }
        return nullptr;
    }

    IWGacRenderTarget* GetBindedRenderTarget(INativeWindow* window) override
    {
        return GetWGacRenderTarget(window);
    }

    void SetBindedRenderTarget(INativeWindow* window, IWGacRenderTarget* renderTarget) override
    {
        auto* wgacWindow = dynamic_cast<wayland::WGacNativeWindow*>(window);
        if (wgacWindow) {
            wgacWindow->SetGraphicsHandler(renderTarget);
        }
    }
};

// WGacResourceManager implementation
class WGacResourceManager : public GuiGraphicsResourceManager, public INativeControllerListener, public IWGacResourceManager
{
protected:
    SortedList<Ptr<WGacRenderTarget>> renderTargets;
    Dictionary<WString, PangoFontDescription*> fontCache;
    Ptr<WGacLayoutProvider> layoutProvider;

public:
    WGacResourceManager()
    {
        g_wGacObjectProvider = new WGacObjectProvider();
        layoutProvider = Ptr(new WGacLayoutProvider());
    }

    ~WGacResourceManager()
    {
        for (auto& pair : fontCache) {
            pango_font_description_free(pair.value);
        }
        delete g_wGacObjectProvider;
    }

    IGuiGraphicsRenderTarget* GetRenderTarget(INativeWindow* window) override
    {
        return GetWGacObjectProvider()->GetBindedRenderTarget(window);
    }

    void RecreateRenderTarget(INativeWindow* window) override
    {
        NativeWindowDestroying(window);
        GetWGacObjectProvider()->RecreateRenderTarget(window);
        NativeWindowCreated(window);
    }

    void ResizeRenderTarget(INativeWindow* window) override
    {
        // Handle resize
    }

    IGuiGraphicsLayoutProvider* GetLayoutProvider() override
    {
        return layoutProvider.Obj();
    }

    Ptr<IGuiGraphicsElement> CreateRawElement() override
    {
        return nullptr;  // Raw elements not supported in Wayland backend
    }

    void NativeWindowCreated(INativeWindow* window) override
    {
        auto renderTarget = Ptr(new WGacRenderTarget(window));
        renderTargets.Add(renderTarget);
        GetWGacObjectProvider()->SetBindedRenderTarget(window, renderTarget.Obj());
    }

    void NativeWindowDestroying(INativeWindow* window) override
    {
        auto* renderTarget = dynamic_cast<WGacRenderTarget*>(GetWGacObjectProvider()->GetBindedRenderTarget(window));
        GetWGacObjectProvider()->SetBindedRenderTarget(window, nullptr);
        renderTargets.Remove(renderTarget);
    }

    PangoFontDescription* CreateWGacFont(const FontProperties& fontProperties) override
    {
        WString key = fontProperties.fontFamily + L"_" + itow(fontProperties.size) +
                      (fontProperties.bold ? L"_B" : L"") +
                      (fontProperties.italic ? L"_I" : L"");

        vint index = fontCache.Keys().IndexOf(key);
        if (index >= 0) {
            return fontCache.Values()[index];
        }

        auto* font = pango_font_description_new();
        AString family = wtoa(fontProperties.fontFamily);
        pango_font_description_set_family(font, family.Buffer());
        pango_font_description_set_absolute_size(font, fontProperties.size * PANGO_SCALE);
        pango_font_description_set_weight(font, fontProperties.bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
        pango_font_description_set_style(font, fontProperties.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

        fontCache.Add(key, font);
        return font;
    }
};

// Global accessors
void SetCurrentRenderTarget(IWGacRenderTarget* renderTarget)
{
    g_currentRenderTarget = renderTarget;
}

IWGacRenderTarget* GetCurrentRenderTarget()
{
    return g_currentRenderTarget;
}

IWGacObjectProvider* GetWGacObjectProvider()
{
    return g_wGacObjectProvider;
}

void SetWGacObjectProvider(IWGacObjectProvider* provider)
{
    g_wGacObjectProvider = provider;
}

IWGacResourceManager* GetWGacResourceManager()
{
    return g_wGacResourceManager;
}

void SetWGacResourceManager(IWGacResourceManager* manager)
{
    g_wGacResourceManager = manager;
}

// Element Renderers
class GuiSolidBorderElementRenderer : public GuiElementRendererBase<GuiSolidBorderElement, GuiSolidBorderElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiSolidBorderElement, GuiSolidBorderElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        Color c = element->GetColor();
        cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
        cairo_set_line_width(cr, 1);

        auto shape = element->GetShape();
        switch (shape.shapeType) {
            case ElementShapeType::Rectangle:
                cairo_rectangle(cr, bounds.x1 + 0.5, bounds.y1 + 0.5, bounds.Width() - 1, bounds.Height() - 1);
                cairo_stroke(cr);
                break;
            case ElementShapeType::RoundRect:
            {
                double degrees = M_PI / 180.0;
                double radius = shape.radiusX;
                cairo_new_sub_path(cr);
                cairo_arc(cr, bounds.x2 - radius - 0.5, bounds.y1 + radius + 0.5, radius, -90 * degrees, 0);
                cairo_arc(cr, bounds.x2 - radius - 0.5, bounds.y2 - radius - 0.5, radius, 0, 90 * degrees);
                cairo_arc(cr, bounds.x1 + radius + 0.5, bounds.y2 - radius - 0.5, radius, 90 * degrees, 180 * degrees);
                cairo_arc(cr, bounds.x1 + radius + 0.5, bounds.y1 + radius + 0.5, radius, 180 * degrees, 270 * degrees);
                cairo_close_path(cr);
                cairo_stroke(cr);
            }
            break;
            case ElementShapeType::Ellipse:
                cairo_save(cr);
                cairo_translate(cr, bounds.x1 + bounds.Width() / 2.0, bounds.y1 + bounds.Height() / 2.0);
                cairo_scale(cr, bounds.Width() / 2.0, bounds.Height() / 2.0);
                cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
                cairo_restore(cr);
                cairo_stroke(cr);
                break;
        }
    }

    void OnElementStateChanged() override {}
};

class GuiSolidBackgroundElementRenderer : public GuiElementRendererBase<GuiSolidBackgroundElement, GuiSolidBackgroundElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiSolidBackgroundElement, GuiSolidBackgroundElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        Color c = element->GetColor();
        cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);

        auto shape = element->GetShape();
        switch (shape.shapeType) {
            case ElementShapeType::Rectangle:
                cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
                cairo_fill(cr);
                break;
            case ElementShapeType::RoundRect:
            {
                double degrees = M_PI / 180.0;
                double radius = shape.radiusX;
                cairo_new_sub_path(cr);
                cairo_arc(cr, bounds.x2 - radius, bounds.y1 + radius, radius, -90 * degrees, 0);
                cairo_arc(cr, bounds.x2 - radius, bounds.y2 - radius, radius, 0, 90 * degrees);
                cairo_arc(cr, bounds.x1 + radius, bounds.y2 - radius, radius, 90 * degrees, 180 * degrees);
                cairo_arc(cr, bounds.x1 + radius, bounds.y1 + radius, radius, 180 * degrees, 270 * degrees);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            break;
            case ElementShapeType::Ellipse:
                cairo_save(cr);
                cairo_translate(cr, bounds.x1 + bounds.Width() / 2.0, bounds.y1 + bounds.Height() / 2.0);
                cairo_scale(cr, bounds.Width() / 2.0, bounds.Height() / 2.0);
                cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
                cairo_restore(cr);
                cairo_fill(cr);
                break;
        }
    }

    void OnElementStateChanged() override {}
};

class GuiSolidLabelElementRenderer : public GuiElementRendererBase<GuiSolidLabelElement, GuiSolidLabelElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiSolidLabelElement, GuiSolidLabelElementRenderer, IWGacRenderTarget>;

    PangoLayout* layout = nullptr;
    WString oldText;
    FontProperties oldFont;
    vint oldMaxWidth = -1;

    void UpdateMinSize()
    {
        if (renderTarget)
        {
            int text_width = 0;
            int text_height = 0;

            if (element->GetWrapLine())
            {
                if (element->GetWrapLineHeightCalculation())
                {
                    if (oldMaxWidth == -1 || oldText.Length() == 0)
                    {
                        pango_layout_set_text(layout, "", -1);
                    }
                    else
                    {
                        pango_layout_set_width(layout, oldMaxWidth * PANGO_SCALE);
                        AString text = wtoa(oldText);
                        pango_layout_set_text(layout, text.Buffer(), -1);
                    }
                }
            }
            else
            {
                AString text = wtoa(oldText.Length() == 0 ? L"" : oldText);
                pango_layout_set_text(layout, text.Buffer(), -1);
            }

            pango_layout_get_pixel_size(layout, &text_width, &text_height);
            minSize = Size((element->GetEllipse() ? 0 : text_width), text_height);
        }
        else
        {
            minSize = Size(0, 0);
        }
    }

    void InitializeInternal()
    {
        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
        cairo_t* cr = cairo_create(surface);
        layout = pango_cairo_create_layout(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }

    void FinalizeInternal()
    {
        if (layout) {
            g_object_unref(layout);
            layout = nullptr;
        }
    }

    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*)
    {
        UpdateMinSize();
    }

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr || !layout) return;

        auto font = GetWGacResourceManager()->CreateWGacFont(element->GetFont());
        pango_layout_set_font_description(layout, font);

        AString text = wtoa(element->GetText());
        pango_layout_set_text(layout, text.Buffer(), -1);

        // Set wrap mode and width
        if (element->GetWrapLine())
        {
            pango_layout_set_width(layout, bounds.Width() * PANGO_SCALE);
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        }
        else
        {
            pango_layout_set_width(layout, -1);
        }

        // Set ellipsize
        if (element->GetEllipse())
        {
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
        }
        else
        {
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
        }

        // Set Pango alignment for text within the layout
        PangoAlignment pangoAlign = PANGO_ALIGN_LEFT;
        switch (element->GetHorizontalAlignment()) {
            case Alignment::Center:
                pangoAlign = PANGO_ALIGN_CENTER;
                break;
            case Alignment::Right:
                pangoAlign = PANGO_ALIGN_RIGHT;
                break;
            default:
                break;
        }
        pango_layout_set_alignment(layout, pangoAlign);

        Color c = element->GetColor();
        cairo_set_source_rgba(cr, c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);

        double x = bounds.x1;
        double y = bounds.y1;

        // For wrapped text, Pango handles horizontal alignment within layout width,
        // so we only adjust x position for non-wrapped text
        if (!element->GetWrapLine())
        {
            switch (element->GetHorizontalAlignment()) {
                case Alignment::Center:
                    x = bounds.x1 + (bounds.Width() - minSize.x) / 2.0;
                    break;
                case Alignment::Right:
                    x = bounds.x2 - minSize.x;
                    break;
                default:
                    break;
            }
        }

        // Vertical alignment
        switch (element->GetVerticalAlignment()) {
            case Alignment::Center:
                y = bounds.y1 + (bounds.Height() - minSize.y) / 2.0;
                break;
            case Alignment::Bottom:
                y = bounds.y2 - minSize.y;
                break;
            default:
                break;
        }

        if (oldMaxWidth != bounds.Width())
        {
            oldMaxWidth = bounds.Width();
            UpdateMinSize();
        }

        cairo_move_to(cr, x, y);
        pango_cairo_show_layout(cr, layout);
    }

    void OnElementStateChanged() override
    {
        oldText = element->GetText();
        FontProperties font = element->GetFont();
        if (oldFont != font)
        {
            oldFont = font;
            auto gFont = GetWGacResourceManager()->CreateWGacFont(font);
            pango_layout_set_font_description(layout, gFont);
        }
        UpdateMinSize();
    }
};

class GuiGradientBackgroundElementRenderer : public GuiElementRendererBase<GuiGradientBackgroundElement, GuiGradientBackgroundElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiGradientBackgroundElement, GuiGradientBackgroundElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        Color c1 = element->GetColor1();
        Color c2 = element->GetColor2();

        cairo_pattern_t* pattern = nullptr;
        auto direction = element->GetDirection();

        switch (direction) {
            case GuiGradientBackgroundElement::Horizontal:
                pattern = cairo_pattern_create_linear(bounds.x1, bounds.y1, bounds.x2, bounds.y1);
                break;
            case GuiGradientBackgroundElement::Vertical:
                pattern = cairo_pattern_create_linear(bounds.x1, bounds.y1, bounds.x1, bounds.y2);
                break;
            case GuiGradientBackgroundElement::Slash:
                pattern = cairo_pattern_create_linear(bounds.x2, bounds.y1, bounds.x1, bounds.y2);
                break;
            case GuiGradientBackgroundElement::Backslash:
                pattern = cairo_pattern_create_linear(bounds.x1, bounds.y1, bounds.x2, bounds.y2);
                break;
        }

        if (pattern) {
            cairo_pattern_add_color_stop_rgba(pattern, 0, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
            cairo_pattern_add_color_stop_rgba(pattern, 1, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);

            cairo_set_source(cr, pattern);
            cairo_rectangle(cr, bounds.x1, bounds.y1, bounds.Width(), bounds.Height());
            cairo_fill(cr);
            cairo_pattern_destroy(pattern);
        }
    }

    void OnElementStateChanged() override {}
};

class Gui3DBorderElementRenderer : public GuiElementRendererBase<Gui3DBorderElement, Gui3DBorderElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<Gui3DBorderElement, Gui3DBorderElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        Color c1 = element->GetColor1();
        Color c2 = element->GetColor2();

        cairo_set_line_width(cr, 1);

        // Top and left edges
        cairo_set_source_rgba(cr, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
        cairo_move_to(cr, bounds.x1 + 0.5, bounds.y2 - 0.5);
        cairo_line_to(cr, bounds.x1 + 0.5, bounds.y1 + 0.5);
        cairo_line_to(cr, bounds.x2 - 0.5, bounds.y1 + 0.5);
        cairo_stroke(cr);

        // Bottom and right edges
        cairo_set_source_rgba(cr, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);
        cairo_move_to(cr, bounds.x1 + 0.5, bounds.y2 - 0.5);
        cairo_line_to(cr, bounds.x2 - 0.5, bounds.y2 - 0.5);
        cairo_line_to(cr, bounds.x2 - 0.5, bounds.y1 + 0.5);
        cairo_stroke(cr);
    }

    void OnElementStateChanged() override {}
};

class Gui3DSplitterElementRenderer : public GuiElementRendererBase<Gui3DSplitterElement, Gui3DSplitterElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<Gui3DSplitterElement, Gui3DSplitterElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        Color c1 = element->GetColor1();
        Color c2 = element->GetColor2();

        cairo_set_line_width(cr, 1);

        if (element->GetDirection() == Gui3DSplitterElement::Horizontal) {
            int y = bounds.y1 + bounds.Height() / 2;
            cairo_set_source_rgba(cr, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
            cairo_move_to(cr, bounds.x1, y - 0.5);
            cairo_line_to(cr, bounds.x2, y - 0.5);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);
            cairo_move_to(cr, bounds.x1, y + 0.5);
            cairo_line_to(cr, bounds.x2, y + 0.5);
            cairo_stroke(cr);
        } else {
            int x = bounds.x1 + bounds.Width() / 2;
            cairo_set_source_rgba(cr, c1.r / 255.0, c1.g / 255.0, c1.b / 255.0, c1.a / 255.0);
            cairo_move_to(cr, x - 0.5, bounds.y1);
            cairo_line_to(cr, x - 0.5, bounds.y2);
            cairo_stroke(cr);

            cairo_set_source_rgba(cr, c2.r / 255.0, c2.g / 255.0, c2.b / 255.0, c2.a / 255.0);
            cairo_move_to(cr, x + 0.5, bounds.y1);
            cairo_line_to(cr, x + 0.5, bounds.y2);
            cairo_stroke(cr);
        }
    }

    void OnElementStateChanged() override {}
};

class GuiFocusRectangleElementRenderer : public GuiElementRendererBase<GuiFocusRectangleElement, GuiFocusRectangleElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiFocusRectangleElement, GuiFocusRectangleElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        cairo_set_source_rgba(cr, 0, 0, 0, 1);
        cairo_set_line_width(cr, 1);
        double dashes[] = {1.0, 1.0};
        cairo_set_dash(cr, dashes, 2, 0);
        cairo_rectangle(cr, bounds.x1 + 0.5, bounds.y1 + 0.5, bounds.Width() - 1, bounds.Height() - 1);
        cairo_stroke(cr);
        cairo_set_dash(cr, nullptr, 0, 0);
    }

    void OnElementStateChanged() override {}
};

class GuiInnerShadowElementRenderer : public GuiElementRendererBase<GuiInnerShadowElement, GuiInnerShadowElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiInnerShadowElement, GuiInnerShadowElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        // TODO: Implement inner shadow
    }

    void OnElementStateChanged() override {}
};

class GuiPolygonElementRenderer : public GuiElementRendererBase<GuiPolygonElement, GuiPolygonElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiPolygonElement, GuiPolygonElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        const auto& points = element->GetPointsArray();
        if (points.Count() < 2) return;

        cairo_new_path(cr);
        cairo_move_to(cr, bounds.x1 + points[0].x, bounds.y1 + points[0].y);
        for (vint i = 1; i < points.Count(); i++) {
            cairo_line_to(cr, bounds.x1 + points[i].x, bounds.y1 + points[i].y);
        }
        cairo_close_path(cr);

        Color bc = element->GetBorderColor();
        Color bg = element->GetBackgroundColor();

        cairo_set_source_rgba(cr, bg.r / 255.0, bg.g / 255.0, bg.b / 255.0, bg.a / 255.0);
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, bc.r / 255.0, bc.g / 255.0, bc.b / 255.0, bc.a / 255.0);
        cairo_set_line_width(cr, 1);
        cairo_stroke(cr);
    }

    void OnElementStateChanged() override {}
};

class GuiImageFrameElementRenderer : public GuiElementRendererBase<GuiImageFrameElement, GuiImageFrameElementRenderer, IWGacRenderTarget>
{
    friend class GuiElementRendererBase<GuiImageFrameElement, GuiImageFrameElementRenderer, IWGacRenderTarget>;

    void InitializeInternal() {}
    void FinalizeInternal() {}
    void RenderTargetChangedInternal(IWGacRenderTarget*, IWGacRenderTarget*) {}

    void UpdateMinSize()
    {
        auto image = element->GetImage();
        if (image) {
            auto frame = image->GetFrame(element->GetFrameIndex());
            if (frame) {
                minSize = frame->GetSize();
                return;
            }
        }
        minSize = Size(0, 0);
    }

public:
    void Render(Rect bounds) override
    {
        cairo_t* cr = GetCurrentWGacContextFromRenderTarget();
        if (!cr) return;

        auto image = element->GetImage();
        if (!image) return;

        auto frame = image->GetFrame(element->GetFrameIndex());
        if (!frame) return;

        auto* wgacFrame = dynamic_cast<wayland::WGacImageFrame*>(frame);
        if (!wgacFrame) return;

        cairo_surface_t* surface = wgacFrame->GetSurface();
        if (!surface) return;

        Size imageSize = frame->GetSize();
        if (imageSize.x <= 0 || imageSize.y <= 0) return;

        double x = bounds.x1;
        double y = bounds.y1;
        double w = imageSize.x;
        double h = imageSize.y;

        if (element->GetStretch()) {
            // Stretch to fill bounds
            w = bounds.Width();
            h = bounds.Height();
        } else {
            // Apply alignment
            switch (element->GetHorizontalAlignment()) {
                case Alignment::Center:
                    x = bounds.x1 + (bounds.Width() - imageSize.x) / 2.0;
                    break;
                case Alignment::Right:
                    x = bounds.x2 - imageSize.x;
                    break;
                default:
                    break;
            }

            switch (element->GetVerticalAlignment()) {
                case Alignment::Center:
                    y = bounds.y1 + (bounds.Height() - imageSize.y) / 2.0;
                    break;
                case Alignment::Bottom:
                    y = bounds.y2 - imageSize.y;
                    break;
                default:
                    break;
            }
        }

        cairo_save(cr);

        if (element->GetStretch()) {
            // Scale to fit
            double scaleX = w / imageSize.x;
            double scaleY = h / imageSize.y;
            cairo_translate(cr, x, y);
            cairo_scale(cr, scaleX, scaleY);
            cairo_set_source_surface(cr, surface, 0, 0);
        } else {
            cairo_set_source_surface(cr, surface, x, y);
        }

        cairo_paint(cr);

        // If disabled, apply a gray overlay
        if (!element->GetEnabled()) {
            cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
            cairo_rectangle(cr, x, y, w, h);
            cairo_fill(cr);
        }

        cairo_restore(cr);
    }

    void OnElementStateChanged() override
    {
        UpdateMinSize();
    }
};

// SetupWGacRenderer implementation
int SetupWGacRenderer()
{
    INativeController* controller = wayland::GetWGacController();
    SetNativeController(controller);
    {
        WGacResourceManager resourceManager;
        SetGuiGraphicsResourceManager(&resourceManager);
        SetWGacResourceManager(&resourceManager);
        controller->CallbackService()->InstallListener(&resourceManager);
        {
            GuiSolidLabelElementRenderer::Register();
            GuiSolidBorderElementRenderer::Register();
            GuiSolidBackgroundElementRenderer::Register();
            Gui3DBorderElementRenderer::Register();
            Gui3DSplitterElementRenderer::Register();
            GuiGradientBackgroundElementRenderer::Register();
            GuiImageFrameElementRenderer::Register();
            GuiPolygonElementRenderer::Register();
            GuiInnerShadowElementRenderer::Register();
            GuiFocusRectangleElementRenderer::Register();
            GuiDocumentElementRenderer::Register();
        }
        {
            GuiApplicationMain();
        }
        controller->CallbackService()->UninstallListener(&resourceManager);
    }
    wayland::DestroyWGacController(controller);
    return 0;
}

}
}
}
}
