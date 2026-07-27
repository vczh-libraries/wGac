#ifndef WGAC_AUTOMATIONSERVICE_H
#define WGAC_AUTOMATIONSERVICE_H

#include "../WGacController.h"

namespace vl
{
	namespace presentation
	{
		class AutomationService;
		class AutomationServiceHosted;

		namespace wayland
		{
			template<typename TBase>
			class WGacAutomationServiceBase : public TBase
			{
			protected:
				WString										RunIOCommandInternal(Nullable<WString> windowId, const WString& ioCommand) override;

			public:
				template<typename ...TArgs>
				WGacAutomationServiceBase(TArgs&& ...args)
					: TBase(std::forward<TArgs>(args)...)
				{
				}

				void										Stop() override;
				INativeAutomationService::IOCommandAvailability
															CanRunIOCommands() override;
			};

			class WGacAutomationService : public WGacAutomationServiceBase<AutomationService>
			{
			protected:
				Nullable<WString>							GetNativeWindowId(INativeWindow* window) override;
				INativeWindow*								GetNativeWindow(Nullable<WString> windowId) override;

			public:
				WGacAutomationService();
				~WGacAutomationService();
			};

			class WGacAutomationServiceHosted : public WGacAutomationServiceBase<AutomationServiceHosted>
			{
			public:
				WGacAutomationServiceHosted();
				~WGacAutomationServiceHosted();
			};

			class WGacAutomationServiceRenderer : public WGacAutomationServiceBase<AutomationServiceRenderer>
			{
			public:
				WGacAutomationServiceRenderer(remote_renderer::GuiRemoteRendererSingle* renderer);
				~WGacAutomationServiceRenderer();

				INativeAutomationService::IOCommandAvailability
															CanRunIOCommands() override;
			};
		}
	}
}

#endif
