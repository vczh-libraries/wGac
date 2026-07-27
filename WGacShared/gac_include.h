#ifndef WGAC_SHARED_GAC_INCLUDE
#define WGAC_SHARED_GAC_INCLUDE

#include "GacUI.h"
#include "Wayland/WaylandDisplay.h"

using namespace vl;
using namespace vl::collections;
using namespace vl::presentation;
using namespace vl::presentation::elements;
using namespace vl::presentation::compositions;
using namespace vl::presentation::controls;
using namespace vl::presentation::theme;
using namespace vl::presentation::templates;

template<typename T>
void RunGacWindow()
{
    GuiWindow* window = new T();
    GetApplication()->Run(window);
    delete window;
}

#include "Skins/DarkSkin/DarkSkin.h"

class WGacSkinPlugin : public Object, public IGuiPlugin
{
public:
    GUI_PLUGIN_NAME(Custom_WGacSkinPlugin)
    {
        GUI_PLUGIN_DEPEND(GacGen_DarkSkinResourceLoader);
    }

    void Load(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins) override
    {
        auto theme = Ptr(new darkskin::Theme);
        if (!GetHostedApplication())
        {
            auto display = wayland::GetWaylandDisplay();
            theme->PreferCustomFrameWindow = Nullable<bool>(
                !display || display->PreferCustomFrameWindow()
                );
        }
        RegisterTheme(theme);
    }

    void Unload(bool controllerUnrelatedPlugins, bool controllerRelatedPlugins) override
    {
    }
};

GUI_REGISTER_PLUGIN(WGacSkinPlugin)

#endif
