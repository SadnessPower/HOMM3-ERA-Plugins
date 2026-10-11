#define _H3API_PLUGINS_

#include "framework.h"
#include "era.h"
#include "ObjectExtenderAPI.hpp"

using namespace h3;

Patcher* globalPatcher = nullptr;
PatcherInstance* cannonYardPatcher = nullptr;

namespace
{
	constexpr int CANNON_YARD_OBJECT_TYPE = eObject::WAR_MACHINE_FACTORY;
	constexpr int CANNON_YARD_OBJECT_SUBTYPE = 1;

	class CannonYardObjectExtender final : public extender::ObjectExtender
	{
	public:
		explicit CannonYardObjectExtender(PatcherInstance* patcher) : ObjectExtender(patcher)
		{
			AddUniqueObjectInfo(CANNON_YARD_OBJECT_TYPE, CANNON_YARD_OBJECT_SUBTYPE);
		}
	};
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	static bool initialized = false;
	if (reason == DLL_PROCESS_ATTACH && !initialized)
	{
		initialized = true;
		globalPatcher = GetPatcher();
		cannonYardPatcher = globalPatcher->CreateInstance("EraPlugin.CannonYardObjectExtender");
		Era::ConnectEra(module, "EraPlugin.CannonYardObjectExtender");

		static CannonYardObjectExtender cannonYardExtender(cannonYardPatcher);
		cannonYardExtender.Register();
	}

	return TRUE;
}

