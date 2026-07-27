#ifndef WGAC_DIALOGSERVICE_H
#define WGAC_DIALOGSERVICE_H

#include "GacUI.h"

namespace vl {
namespace presentation {
namespace wayland {

class WGacDialogService : public Object, public INativeDialogService
{
public:
    MessageBoxButtonsOutput ShowMessageBox(
        INativeWindow* window,
        const WString& text,
        const WString& title,
        MessageBoxButtonsInput buttons,
        MessageBoxDefaultButton defaultButton,
        MessageBoxIcons icon,
        MessageBoxModalOptions modal
    ) override;

    bool ShowColorDialog(
        INativeWindow* window,
        Color& selection,
        bool selected,
        ColorDialogCustomColorOptions customColorOptions,
        Color* customColors
    ) override;

    bool ShowFontDialog(
        INativeWindow* window,
        FontProperties& selectionFont,
        Color& selectionColor,
        bool selected,
        bool showEffect,
        bool forceFontExist
    ) override;

    bool ShowFileDialog(
        INativeWindow* window,
        collections::List<WString>& selectionFileNames,
        vint& selectionFilterIndex,
        FileDialogTypes dialogType,
        const WString& title,
        const WString& initialFileName,
        const WString& initialDirectory,
        const WString& defaultExtension,
        const WString& filter,
        FileDialogOptions options
    ) override;
};

}
}
}

#endif // WGAC_DIALOGSERVICE_H
