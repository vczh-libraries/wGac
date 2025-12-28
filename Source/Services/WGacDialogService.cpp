#include "WGacDialogService.h"

namespace vl {
namespace presentation {
namespace wayland {

INativeDialogService::MessageBoxButtonsOutput WGacDialogService::ShowMessageBox(
    INativeWindow* window,
    const WString& text,
    const WString& title,
    MessageBoxButtonsInput buttons,
    MessageBoxDefaultButton defaultButton,
    MessageBoxIcons icon,
    MessageBoxModalOptions modal)
{
    // TODO: Implement native dialog
    return MessageBoxButtonsOutput::SelectOK;
}

bool WGacDialogService::ShowColorDialog(
    INativeWindow* window,
    Color& selection,
    bool selected,
    ColorDialogCustomColorOptions customColorOptions,
    Color* customColors)
{
    // TODO: Implement
    return false;
}

bool WGacDialogService::ShowFontDialog(
    INativeWindow* window,
    FontProperties& selectionFont,
    Color& selectionColor,
    bool selected,
    bool showEffect,
    bool forceFontExist)
{
    // TODO: Implement
    return false;
}

bool WGacDialogService::ShowFileDialog(
    INativeWindow* window,
    collections::List<WString>& selectionFileNames,
    vint& selectionFilterIndex,
    FileDialogTypes dialogType,
    const WString& title,
    const WString& initialFileName,
    const WString& initialDirectory,
    const WString& defaultExtension,
    const WString& filter,
    FileDialogOptions options)
{
    // TODO: Implement using xdg-portal
    return false;
}

}
}
}
