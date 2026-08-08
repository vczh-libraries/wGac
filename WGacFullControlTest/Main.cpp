#include "gac_include.h"
#include "FullControlTest.h"
#include "../WGac/Renderers/WGacRenderer.h"

#include <cstring>
#include <VlppOS.h>

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::remoting;

int main(int argc, char* argv[])
{
    bool hosted = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--hosted") == 0)
        {
            hosted = true;
        }
    }

    if (hosted)
    {
        return elements::wgac::SetupWGacHostedRenderer();
    }
    return elements::wgac::SetupWGacRenderer();
}

void GuiMain()
{
    demo::MainWindow window;
    window.ForceCalculateSizeImmediately();
    window.MoveToScreenCenter();

    auto socketServer = inter_process::async_tcp_socket::CreateDefaultAsyncSocketServer(8888);
    StartMiniHttpAutomationService(socketServer, WString::Unmanaged(L"Test_FullControlTest"));
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
