#include "gac_include.h"
#include "Renderers/WGacRenderer.h"

#include <VlppOS.h>

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::remoting;

int main()
{
    return elements::wgac::SetupWGacRenderer();
}

void GuiMain()
{
    GuiWindow window(theme::ThemeName::Window);
    window.SetText(L"Hello, world!");
    window.SetClientSize(Size(480, 320));
    window.GetBoundsComposition()->SetPreferredMinSize(Size(480, 320));
    window.MoveToScreenCenter();

    auto label = new GuiLabel(theme::ThemeName::Label);
    {
        FontProperties font;
        font.fontFamily = L"Lucida Calligraphy";
        font.antialias = true;
        font.size = 32;
        label->SetFont(font);
        label->SetText(L"Welcome to GacUI Library!");
    }
    window.AddChild(label);

    auto socketServer = inter_process::async_tcp_socket::CreateDefaultAsyncSocketServer(8888);
    StartMiniHttpAutomationService(socketServer, WString::Unmanaged(L"Test_HellWorld_Cpp"));
    try
    {
        GetApplication()->Run(&window);
    }
    catch (...)
    {
        StopMiniHttpAutomationService();
        throw;
    }
    StopMiniHttpAutomationService();
}
