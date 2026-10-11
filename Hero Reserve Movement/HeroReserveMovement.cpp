#include "patcher_x86_commented.hpp"
#include "era.h"

#include <cstdio>

namespace
{
    constexpr int HERO_GET_MOBILITY_LAND_SCAN = 0x004E4E32;
    constexpr int HERO_ID_OFFSET = 0x1A;
    constexpr int HERO_FIRST = 0;
    constexpr int HERO_LAST_WOG = 155;
    constexpr int CREATURE_NONE = -1;
    constexpr int CREATURE_LAST_WOG = 196;

    constexpr int HERO_SPECIFIC_ABILITIES_POINTER = 0x00679C80;
    constexpr int CREATURE_TRAITS_POINTER = 0x006747B0;
    constexpr int CREATURE_TRAIT_SIZE = 0x74;
    constexpr int CREATURE_SPEED_OFFSET = 0x50;
    constexpr int GET_MONSTER_UPGRADE = 0x0047AAD0;
    constexpr int SPEED_COMPARISON_OPCODE = 0x004E4EB3;
    constexpr unsigned char JLE_SHORT_OPCODE = 0x7E;

    struct HeroSpecificAbility
    {
        int specialityType;
        int specializedCreature;
        int unused[8];
    };

    using GetMonsterUpgrade = int(__fastcall*)(int creatureType);

    Patcher* g_patcher = nullptr;
    PatcherInstance* g_instance = nullptr;

    int GetReserveValue(int heroId, int slot, const char* field)
    {
        char variableName[128];
        sprintf_s(
            variableName,
            "treb_hre_hero%d_slot%d_monReserve%s",
            heroId,
            slot,
            field);
        return Era::GetAssocVarIntValue(variableName);
    }

    _LHF_(IncludeReserveSpeedInHeroMovement_004E4E32)
    {
        if (!c || !c->ebx || !c->ebp || !Era::GetAssocVarIntValue)
            return EXEC_DEFAULT;

        const int heroId = *reinterpret_cast<const int*>(c->ebx + HERO_ID_OFFSET);
		if (heroId < HERO_FIRST || heroId > HERO_LAST_WOG)
			return EXEC_DEFAULT;

		char reserveArmyVariableName[96];
		sprintf_s(reserveArmyVariableName, "treb_hre_hero%d_hasReserveArmy", heroId);
		const bool heroHasReserveArmy = Era::GetAssocVarIntValue(reserveArmyVariableName) > 0;
		if (!heroHasReserveArmy)
            return EXEC_DEFAULT;

        int& movementSpeed = *reinterpret_cast<int*>(c->ebp - 4);
        const auto creatureTraits = *reinterpret_cast<const unsigned char**>(CREATURE_TRAITS_POINTER);
        const auto heroSpecificAbilities =
            *reinterpret_cast<const HeroSpecificAbility**>(HERO_SPECIFIC_ABILITIES_POINTER);

        if (!creatureTraits || !heroSpecificAbilities)
            return EXEC_DEFAULT;

        const HeroSpecificAbility& speciality = heroSpecificAbilities[heroId];
        int specializedUpgrade = CREATURE_NONE;
        if (speciality.specialityType == 1 &&
            speciality.specializedCreature > CREATURE_NONE &&
            speciality.specializedCreature <= CREATURE_LAST_WOG)
        {
            specializedUpgrade =
                reinterpret_cast<GetMonsterUpgrade>(GET_MONSTER_UPGRADE)(speciality.specializedCreature);
        }

        // speed4Diozia changes this JGE into JLE when configured to use the fastest creature. Reading the live opcode also covers the native mode.
        const bool useFastestCreature =
            *reinterpret_cast<const unsigned char*>(SPEED_COMPARISON_OPCODE) == JLE_SHORT_OPCODE;

        for (int slot = 0; slot < 7; ++slot)
        {
            const int creatureType = GetReserveValue(heroId, slot, "Type");
            const int creatureQuantity = GetReserveValue(heroId, slot, "Qty");
            if (creatureType <= CREATURE_NONE ||
                creatureType > CREATURE_LAST_WOG ||
                creatureQuantity <= 0)
            {
                continue;
            }

			int reserveSpeed = *reinterpret_cast<const int*>(creatureTraits + creatureType * CREATURE_TRAIT_SIZE + CREATURE_SPEED_OFFSET);
            if (speciality.specialityType == 1 &&
                (creatureType == speciality.specializedCreature ||
                 creatureType == specializedUpgrade))
            {
                ++reserveSpeed;
            }

            if (useFastestCreature)
            {
                if (reserveSpeed > movementSpeed)
                    movementSpeed = reserveSpeed;
            }
            else if (reserveSpeed < movementSpeed)
            {
                movementSpeed = reserveSpeed;
            }
        }

        return EXEC_DEFAULT;
    }
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        Era::ConnectEra(module, "HeroMovementCalculationForReserveSlots");
        g_patcher = GetPatcher();
        g_instance = g_patcher->CreateInstance("HeroMovementCalculationForReserveSlots");
        g_instance->WriteLoHook(
            HERO_GET_MOBILITY_LAND_SCAN,
            IncludeReserveSpeedInHeroMovement_004E4E32);
    }

    return TRUE;
}
