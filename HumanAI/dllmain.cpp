// #include "../../__include__/H3API/single_header/H3API.hpp"
#include "patcher_x86_commented.hpp"

#pragma warning(disable : 4996)
#include "H3API.hpp"

#define activePlayerID *(int*)0x69CCF4
#define gameMgr h3::H3Main::Get()
#define battleMgr h3::H3CombatManager::Get() 

using namespace h3;

typedef h3::H3CombatMonster _BattleStack_;

#include "pch.h"
#include "era.h"

const int ADV_MAP = 37;
const int CTRL_LMB = 4;
const int LMB_PUSH = 12;

#define PINSTANCE_MAIN "SadnessPower_HumanAI"

#include<sstream>

Patcher* globalPatcher;
PatcherInstance* Z_HumanAI;

typedef h3::H3CombatMonster _BattleStack_;

// ----------------------------------------------

/* Retrieving data from decompiled source

   REGISTERS:
   eax
   ecx
   edx
   ebx
   esi
   edi

   -> can contains value or pointer

   int& myVariable = c->eax;         // value
   int& myVariable = *(int*)c->eax;  // pointer
*/

// ----------------------------------------------

/*
   MEMORY POINTERS:
   ebp
   esp (i've never used this, please use ebp)

   -> can contains pointer or pointer to pointer

   int& myVariable = *(int*)c->eax;    // pointer
   int*& myVariable = *(int**)c->eax;  // pointer to pointer
*/

bool town_isEnemyNearby[48];
void __stdcall type_town_threat_checker_check_towns_004280E0(HiHook* h, int* _this)
{
	THISCALL_1(int, h->GetDefaultFunc(), _this);
	memset(town_isEnemyNearby, 0, sizeof(town_isEnemyNearby));
}

_LHF_(markTown_EnemyNearby_004283BE)
{
	h3::H3Town* town = (h3::H3Town*) c->esi;
	town_isEnemyNearby[town->number] = true;
	return EXEC_DEFAULT;
}

extern "C" __declspec(dllexport) BOOL isFunnelTroopsSafeForTown(int townId)
{
	return !town_isEnemyNearby[townId];
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{

			globalPatcher = GetPatcher();
			Z_HumanAI = globalPatcher->CreateInstance(PINSTANCE_MAIN);
			Era::ConnectEra(hModule, PINSTANCE_MAIN);

			Z_HumanAI->WriteHiHook(0x004280E0, SPLICE_, EXTENDED_, THISCALL_, type_town_threat_checker_check_towns_004280E0);
			Z_HumanAI->WriteLoHook(0x004283BE, markTown_EnemyNearby_004283BE);

			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

