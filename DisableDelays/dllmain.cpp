// #include "../../__include__/H3API/single_header/H3API.hpp"
#include "patcher_x86_commented.hpp"

#include <windows.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

typedef unsigned int HSAMPLE;

// Implementation of disabling sound directly via mss32.dll
//typedef void* HSTREAM;
//typedef unsigned int HAUDIO;
//
////using AIL_GetDigVol_Fn = long(__stdcall*)(void* /*driver*/);
////using AIL_SetDigVol_Fn = void(__stdcall*)(void* /*driver*/, long /*vol*/);
//using AIL_GetDigVol_Fn = long(__stdcall*)(HAUDIO);
//using AIL_SetDigVol_Fn = void(__stdcall*)(HAUDIO, long /*vol*/);
//using FP_reset_dig = void(__stdcall*)(HAUDIO);
//using FP_quick_halt = void(__stdcall*)(HAUDIO);
//using FP_end_sample = void(__stdcall*)(HSAMPLE*);
//using FP_stream_status = int(__stdcall*)(HSTREAM);
//using FP_serve = void(__stdcall*)(void);
//
//HMODULE hMss;

#pragma warning(disable : 4996)
#include "H3API.hpp"

#define activePlayerID *(int*)0x69CCF4
#define gameMgr h3::H3Main::Get()
#define battleMgr h3::H3CombatManager::Get() 

int disableSoundToo;

using namespace h3;

typedef h3::H3CombatMonster _BattleStack_;

#include "pch.h"
#include "era.h"

const int ADV_MAP = 37;
const int CTRL_LMB = 4;
const int LMB_PUSH = 12;

#define PINSTANCE_MAIN "Z_DisableAITurnDelay"

#include<sstream>

Patcher* globalPatcher;
PatcherInstance* Z_DisableAITurnDelay;

struct config
{
	bool goSoloDisableSounds;
	bool keepAIMonsterDefeatDelayOnFastestSpeed;
	bool keepAIMonsterDefeatDelayAllSpeeds;
	bool keepDelayForHumanPlayer;
	bool disableWaitingForFadeIn;
} config;

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

/*
void __stdcall StopAllSamples_0059A090(HiHook* h, H3SoundManager* soundManager, int stopMusicToo)
{
	int noSound = *(int*)0x006992E0;
	//H3SoundManager* soundManager = (H3SoundManager*)*(int*)0x699414;
	if (!noSound && soundManager->driver && (soundManager->clickSoundVar || P_AutoSolo))
	{
		auto pAIL_get_digital_master_volume = reinterpret_cast<AIL_GetDigVol_Fn>(GetProcAddress(hMss, "AIL_get_digital_master_volume"));
		auto pAIL_set_digital_master_volume = reinterpret_cast<AIL_SetDigVol_Fn>(GetProcAddress(hMss, "AIL_set_digital_master_volume"));

		pAIL_set_digital_master_volume(soundManager->driver, 0);
		/*
		auto pAIL_quick_halt = (FP_quick_halt)GetProcAddress(hMss, "_AIL_quick_halt@4");
		pAIL_quick_halt(soundManager->driver);

		auto pAIL_serve = (FP_serve)GetProcAddress(hMss, "_AIL_serve@0");
		pAIL_serve();

		soundManager->MP3Playing = false;
		*(HSTREAM*)0x0069FED8 = 0;

		BYTE* MP3Position   = reinterpret_cast <BYTE*>(0x006A0024);
		memset(MP3Position, 0, 3235);

		BYTE* PausedSamples = reinterpret_cast <BYTE*>(0x006A32B8);
		memset(PausedSamples, 0, 0x38);

		auto pAIL_reset_digital_driver = (FP_reset_dig)GetProcAddress(hMss, "_AIL_reset_digital_driver@4");
		pAIL_reset_digital_driver(soundManager->driver);

		pAIL_serve();
		*/
		/*
		for (int i = 0; i < soundManager->sampleNum; i++)
		{
			auto pAIL_end_sample = (FP_end_sample)GetProcAddress(hMss, "_AIL_end_sample@4");
			pAIL_end_sample(soundManager->hSamples[i]);
		}

		if (stopMusicToo)
		{
			HSTREAM MP3Stream = *(HSTREAM*)0x0069FED8;
			auto pAIL_stream_status = (FP_stream_status)GetProcAddress(hMss, "_AIL_stream_status@4");
			int streamStatus = pAIL_stream_status(MP3Stream);

			if (MP3Stream && streamStatus == 4)
			{
				auto pAIL_serve = (FP_serve)GetProcAddress(hMss, "_AIL_serve@0");
				pAIL_serve();
				Sleep(5);
				THISCALL_2(int, 0x0059B56A, MP3Stream, 1);
			}
		}
		*/
		/*
			}
		}
		*/

void loadConfig()
{
	int const DEFAULT_DISABLE_SOUND_TOO = 2;

	static constexpr LPCSTR INI_FILE_PATH = "Runtime/DisableDelay.ini";
	static constexpr LPCSTR SETTINGS_INI_SECTION = "Settings";
	static const std::string DEFAULT_CONTENT =
		std::string("[Settings]") +
		"\n; Will disable pick-up resource and defeat monster sounds for gosolo" +
		"\nGoSoloDisableSounds=0"
		"\n"
		"\n; Keep AI monster defeat fade and delay on fastest speed (will give more time to player to see which monster type has been defeated)" +
		"\nKeepAIMonsterDefeatDelayOnFastestSpeed=1"
		"\n"
		"\nKeepAIMonsterDefeatDelayAllSpeeds=1"
		"\n"
		"\n; Keep delay after picking resource / defeating monsters for the human player"
		"\nKeepDelayForHumanPlayer=0"
		"\n"
		"\n; Disable any waiting for fade in (main menu and interface will be faster)"
		"\nDisableWaitingForFadeIn=1";

	namespace fs = std::filesystem;
	fs::path p(INI_FILE_PATH);

	// controlla esistenza (senza lanciare eccezioni)
	std::error_code ec;
	if (fs::exists(p, ec))
	{
		bool result = Era::ReadStrFromIni("DisableSoundGoSolo", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
		if (result)
			config.goSoloDisableSounds = atoi(h3_TextBuffer);

		result = Era::ReadStrFromIni("KeepAIMonsterDefeatDelayOnFastestSpeed", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
		if (result)
			config.keepAIMonsterDefeatDelayOnFastestSpeed = atoi(h3_TextBuffer);

		result = Era::ReadStrFromIni("KeepAIMonsterDefeatDelayAllSpeeds", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
		if (result)
			config.keepAIMonsterDefeatDelayAllSpeeds = atoi(h3_TextBuffer);

		result = Era::ReadStrFromIni("KeepDelayForHumanPlayer", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
		if (result)
			config.keepDelayForHumanPlayer = atoi(h3_TextBuffer);

		result = Era::ReadStrFromIni("DisableWaitingForFadeIn", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
		if (result)
			config.disableWaitingForFadeIn = atoi(h3_TextBuffer);
	}
	else
		// crea il file
	{
		std::ofstream file(INI_FILE_PATH);
		if (file.is_open())
		{
			file << DEFAULT_CONTENT;
			file.close();
		}
	}
}

bool isAIFastestSpeed()
{
	return (*(int*)0x006987A8 == 4);
}

bool isHumanPlayerActive()
{
	return gameMgr->players[P_ActivePlayer->ownerID].is_human;
}

bool shouldKeepAIMonsterDelay()
{
	return (
			     config.keepAIMonsterDefeatDelayAllSpeeds
			 || (config.keepAIMonsterDefeatDelayOnFastestSpeed && isAIFastestSpeed())
    	   );
}

// ----------------------------------------------
_LHF_(DisableDelayHook)
{
	BOOL8 goSolo = *(BOOL8*)0x00691259;
	if ((!P_ActivePlayer()->is_human || goSolo) && shouldKeepAIMonsterDelay() && c->return_address != 0x004A8194)
		return EXEC_DEFAULT;

	if (   (!isHumanPlayerActive() && !goSolo)
		|| (config.goSoloDisableSounds && goSolo)
	   )
		*(int*)0x00698A10 = false;

	if (c->return_address == 0x004ADF4A)
		return SKIP_DEFAULT;
	else
		return EXEC_DEFAULT;
}

int monsterDefeatAddresses[] =
{
	0x0049DEA3, 0x004A69DC, 0x004A6BCE, 0x004A6E3C, 0x004A71B0, 0x004AC91E, 0
};

bool isMonsterDefeatAddress(int addr)
{
	for (int i = 0; monsterDefeatAddresses[i] != 0; i++)
	{
		if (addr == monsterDefeatAddresses[i])
			return true;
	}
	return false;
}


_LHF_(PlaySoundAndDisableDelayHook)
{
	if (config.keepDelayForHumanPlayer && P_ActivePlayer->is_human)
		return EXEC_DEFAULT;

	int instructionBytes = 0x06;
	int hookAddress = c->return_address - instructionBytes;

	if (isMonsterDefeatAddress(hookAddress) && shouldKeepAIMonsterDelay())
		return EXEC_DEFAULT;

	if (hookAddress == 0x004A3DDA)
		c->return_address = hookAddress + 0x5f;
	else
	if (hookAddress == 0x004A71B0)
		c->return_address = hookAddress + 0x62;
	else
		c->return_address = hookAddress + 0x61;

	return SKIP_DEFAULT;
}

_LHF_(DisableBuildingConstructionDelay_005D71C0)
{
	c->return_address = 0x005D71C5;
	return SKIP_DEFAULT;
}

_LHF_(DisableBuildingConstructionDelay_005D723F)
{
	c->return_address = 0x005D7244;
	return SKIP_DEFAULT;
}

_LHF_(DisableBuildingConstructionDelay_005D70D3)
{
	c->return_address = 0x005D70D8;
	return SKIP_DEFAULT;
}

_LHF_(DisableBuildingConstructionDelay_005D7154)
{
	c->return_address = 0x005D7159;
	return SKIP_DEFAULT;
}



_LHF_(DisableWaitingForFadeIn_00603196)
{
	c->return_address = 0x0060319D;
	return SKIP_DEFAULT;
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
			Z_DisableAITurnDelay = globalPatcher->CreateInstance(PINSTANCE_MAIN);
			Era::ConnectEra(hModule, PINSTANCE_MAIN);

			/*
			hMss = LoadLibraryA("mss32.dll");
			auto pAIL_reset_digital_driver = (FP_reset_dig)GetProcAddress(hMss, "_AIL_reset_digital_driver@4");
			auto pAIL_quick_halt = (FP_quick_halt)GetProcAddress(hMss, "_AIL_quick_halt@4");
			auto pAIL_end_sample = (FP_end_sample)GetProcAddress(hMss, "_AIL_end_sample@4");
			auto pAIL_stream_status = (FP_stream_status)GetProcAddress(hMss, "_AIL_stream_status@4");
			auto pAIL_serve = (FP_serve)GetProcAddress(hMss, "_AIL_serve@0");

			if (!hMss || !pAIL_reset_digital_driver || !pAIL_quick_halt || !pAIL_end_sample || !pAIL_stream_status || !pAIL_serve)
			{
				DWORD err = GetLastError();

				LPVOID msgBuffer;
				FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr,
					err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPSTR)&msgBuffer,
					0,
					nullptr
				);

				std::cerr << "LoadLibrary error: code " << err
					<< "\nDescription: " << (char*)msgBuffer << "\n";

				LocalFree(msgBuffer);
				return 1;
			}

			assert(pAIL_end_sample && "AIL_end_sample non trovato");
			assert(pAIL_stream_status && "AIL_stream_status non trovato");
			assert(pAIL_serve && "AIL_serve non trovato");
			*/


			// ini file handler			
			loadConfig();

			// Remove fading sound delay everywhere
			Z_DisableAITurnDelay->WriteHexPatch(0x0059B4FF, "EB");
			//			Z_DisableAITurnDelay->WriteHiHook(0x0059A090, SPLICE_, EXTENDED_, THISCALL_, StopAllSamples_0059A090);

						// Remove stop all sound before combat
			//			Z_DisableAITurnDelay->WriteHexPatch(0x0046264D, "90 90 90 90 90");

			Z_DisableAITurnDelay->WriteLoHook(0x004A818F, DisableDelayHook);  // DisableAIAllEventsDelay
			Z_DisableAITurnDelay->WriteLoHook(0x004ADF40, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004AE5D1, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004AE5C2, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004AE1FE, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A6996, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A75BC, DisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A6B88, DisableDelayHook);

			// Remove delay while playing sound
			// jump +0x61
			Z_DisableAITurnDelay->WriteLoHook(0x0049DEA3, PlaySoundAndDisableDelayHook); // Pick up Artifact
			Z_DisableAITurnDelay->WriteLoHook(0x0049E622, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x0049E930, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x0049EBEC, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x0049F3B8, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A0ABE, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A0E89, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A2052, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A4619, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A47D1, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A49AF, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A4C06, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A4E7D, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A505D, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A5A8B, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A5C90, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A62FA, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A69DC, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A6BCE, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A6E3C, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004A9877, PlaySoundAndDisableDelayHook);
			Z_DisableAITurnDelay->WriteLoHook(0x004AC91E, PlaySoundAndDisableDelayHook); // FizzleCenter function

			// jump +0x5F
			Z_DisableAITurnDelay->WriteLoHook(0x004A3DDA, PlaySoundAndDisableDelayHook);
			// jump +0x62
			Z_DisableAITurnDelay->WriteLoHook(0x004A71B0, PlaySoundAndDisableDelayHook);

			// Disable building construction delay
			//Z_DisableAITurnDelay->WriteLoHook(0x005D71C0, DisableBuildingConstructionDelay_005D71C0);
			//Z_DisableAITurnDelay->WriteLoHook(0x005D723F, DisableBuildingConstructionDelay_005D723F);
			//Z_DisableAITurnDelay->WriteLoHook(0x005D70D3, DisableBuildingConstructionDelay_005D70D3);
			//Z_DisableAITurnDelay->WriteLoHook(0x005D7154, DisableBuildingConstructionDelay_005D7154);
			
			if (config.disableWaitingForFadeIn)
				Z_DisableAITurnDelay->WriteLoHook(0x00603196, DisableWaitingForFadeIn_00603196);		

			/*
			Z_DisableAITurnDelay->WriteHexPatch(0x0049DEA3, "E9 5C 00 00 00"); // Pick up Artifact
			Z_DisableAITurnDelay->WriteHexPatch(0x0049E622, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x0049E930, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x0049EBEC, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x0049F3B8, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A0ABE, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A0E89, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A2052, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A4619, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A47D1, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A49AF, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A4C06, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A4E7D, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A505D, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A5A8B, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A5C90, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A62FA, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A69DC, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A6BCE, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A6E3C, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A9877, "E9 5C 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004AC91E, "E9 5C 00 00 00"); // FizzleCenter function
			Z_DisableAITurnDelay->WriteHexPatch(0x004A3DDA, "E9 5A 00 00 00");
			Z_DisableAITurnDelay->WriteHexPatch(0x004A71B0, "E9 5D 00 00 00");
			*/
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

