#include "framework.h"
#include "LeadershipRules.h"
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr int kLeadership = 6;
    Patcher* patcher = nullptr;
    PatcherInstance* patches = nullptr;
    thread_local const char* leadershipMoralePopupText = nullptr;
    thread_local int leadershipMoralePopupTextWidth = 0;

    // Verified prefix of vanilla's normal-dialog descriptor; the rest stays native.
    struct NativeDialogTextLayout
    {
        h3::H3String text;
        int x, y, width, height;
        int textX, textY, textWidth, textHeight;
    };
    static_assert(offsetof(NativeDialogTextLayout, textWidth) == 0x28);
    static_assert(sizeof(NativeDialogTextLayout) == 0x30);

    struct ArmyDescriptionContext
    {
        const h3::H3Army* army;
        const h3::H3Hero* hero;
        BOOL alliance;
    };
    thread_local const ArmyDescriptionContext* descriptionContext = nullptr;

    int GetLeadershipLevel(const h3::H3Hero* hero)
    {
        if (!hero)
            return 0;
        const int nativeLevel = hero->secSkill[kLeadership];
        if (nativeLevel < 3 || hero->id < 0)
            return acm::LeadershipLevel(nativeLevel, false, false);
        char variable[64];
        sprintf_s(variable, "H3_Leadership_0_Hero%d", hero->id);
        const bool master = Era::GetAssocVarIntValue(variable) != 0;
        sprintf_s(variable, "H3_Leadership_1_Hero%d", hero->id);
        const bool grandmaster = Era::GetAssocVarIntValue(variable) != 0;
        return acm::LeadershipLevel(nativeLevel, master, grandmaster);
    }

    const char* Explanation(int level)
    {
        static const char* keys[] = { "AC_Misc.Leadership_Alignment_Expert",
            "AC_Misc.Leadership_Alignment_Master", "AC_Misc.Leadership_Alignment_Grandmaster" };
        return level >= 3 && level <= 5 ? Era::tr(keys[level - 3]) : "";
    }

    int ArmyMorale(const h3::H3Army* army, const h3::H3Hero* hero, const h3::H3Town* town,
        const h3::H3Hero* otherHero, const h3::H3Army* otherArmy, BOOL cursed, BOOL alliance, BOOL limits)
    {
        return THISCALL_8(int, 0x0044AB30, army, hero, town, otherHero, otherArmy, cursed, alliance, limits);
    }

    int EffectiveAlignmentCount(const h3::H3Army* army, BOOL alliance)
    {
        signed char legacyCounters[10] = {};
        int count = THISCALL_2(int, 0x0044A880, army, legacyCounters);
        if (alliance && count > 1)
        {
            // Ask the installed native/new_towns calculation how many factions Angelic Alliance merges.
            // Null owners disable our Leadership adjustment; other army modifiers cancel in the difference.
            const int withoutAlliance = ArmyMorale(army, nullptr, nullptr, nullptr, nullptr, FALSE, FALSE, FALSE);
            const int withAlliance = ArmyMorale(army, nullptr, nullptr, nullptr, nullptr, FALSE, TRUE, FALSE);
            count -= withAlliance - withoutAlliance;
        }
        return (std::max)(0, (std::min)(7, count));
    }

    _LHF_(HeroMorale_UseActualLeadershipLevel_004E3C48)
    {
        const auto hero = reinterpret_cast<const h3::H3Hero*>(c->edi);
        const int level = GetLeadershipLevel(hero);
        if (level <= 3)
            return EXEC_DEFAULT;
        // Replace only the seven-byte table load; keep native specialty scaling, artifacts and TEST flags.
        c->ebx = level;
        return SKIP_DEFAULT;
    }

    int __stdcall ArmyMorale_ApplyLeadershipAlignmentReduction_0044AB30(HiHook* hook,
        const h3::H3Army* army, const h3::H3Hero* hero, const h3::H3Town* town,
        const h3::H3Hero* otherHero, const h3::H3Army* otherArmy, BOOL cursed, BOOL alliance, BOOL limits)
    {
        int morale = THISCALL_8(int, hook->GetDefaultFunc(), army, hero, town,
            otherHero, otherArmy, cursed, alliance, FALSE);
        const int level = GetLeadershipLevel(hero);
        if (!cursed && level >= 3)
        {
            const int count = EffectiveAlignmentCount(army, alliance);
            morale += count - acm::AdjustedAlignments(count, level);
        }
        return limits ? (std::max)(-3, (std::min)(3, morale)) : morale;
    }

    _LHF_(HeroMoraleDescription_LabelActualLeadershipLevel_004DCBA6)
    {
        const auto hero = *reinterpret_cast<const h3::H3Hero**>(c->ebp - 0x14);
        const int level = GetLeadershipLevel(hero);
        if (level > 3)
        {
            auto& text = *reinterpret_cast<h3::H3String*>(c->ebp - 0x28);
            const char* expertRow = *reinterpret_cast<const char**>(0x006A587C);
            std::string modified(text.String(), text.Length());
            const size_t row = expertRow && *expertRow ? modified.find(expertRow) : std::string::npos;
            if (row != std::string::npos)
            {
                const char* replacement = Era::tr(level == 5 ?
                    "AC_Misc.Leadership_Morale_Grandmaster" : "AC_Misc.Leadership_Morale_Master");
                modified.replace(row, strlen(expertRow), replacement);
                text.Assign(modified.c_str(), static_cast<unsigned>(modified.size()));
            }
            // EBP-10h accumulates already-explained morale; prevent +4/+5 being reported as a temporary bonus.
            *reinterpret_cast<int*>(c->ebp - 0x10) += level - 3;
        }
        return EXEC_DEFAULT;
    }

    _LHF_(HeroMoraleDescription_AddAlignmentExplanation_004DCCC7)
    {
        const auto hero = *reinterpret_cast<const h3::H3Hero**>(c->ebp - 0x14);
        const int level = GetLeadershipLevel(hero);
        if (level >= 3)
        {
            auto& text = *reinterpret_cast<h3::H3String*>(c->ebp - 0x28);
            text.Append('\n');
            text.Append(Explanation(level));
        }
        return EXEC_DEFAULT;
    }

    h3::H3String* __stdcall ArmyMoraleDescription_PreserveOriginalContext_0044B630(HiHook* hook,
        const h3::H3Army* army, h3::H3String* result, int creature, int morale,
        const h3::H3Hero* hero, const h3::H3Town* town, const h3::H3Hero* otherHero,
        const h3::H3Army* otherArmy, int ground, BOOL alliance)
    {
        const ArmyDescriptionContext context = { army, hero, alliance };
        struct Scope
        {
            const ArmyDescriptionContext* previous;
            explicit Scope(const ArmyDescriptionContext* current) : previous(descriptionContext)
            { descriptionContext = current; }
            ~Scope() { descriptionContext = previous; }
        } scope(&context);
        // Vanilla overwrites EBP+28h (the alliance argument) with the actual morale at 0044B704.
        // Preserve the real argument rather than reading the reused slot, including nested descriptions.
        return THISCALL_10(h3::H3String*, hook->GetDefaultFunc(), army, result, creature, morale,
            hero, town, otherHero, otherArmy, ground, alliance);
    }

    int RouteNativeAlignmentRow(HookContext* c)
    {
        const auto hero = *reinterpret_cast<const h3::H3Hero**>(c->ebp + 0x14);
        const int count = descriptionContext ? EffectiveAlignmentCount(descriptionContext->army,
            descriptionContext->alliance) : c->edi;
        c->edi = acm::AdjustedAlignments(count, GetLeadershipLevel(hero));
        if (c->edi >= 3)
            c->return_address = c->edi < 5 ? 0x0044B986 : 0x0044B97F;
        else
            c->return_address = c->edi == 1 ? 0x0044B9CD : 0x0044B9E9;
        // Jump past either hooked comparison, so reduction runs once. Formatting and undead row stay native.
        return SKIP_DEFAULT;
    }

    _LHF_(ArmyMoraleDescription_AdjustMixedAlignments_0044B97A)
    { return RouteNativeAlignmentRow(c); }

    _LHF_(ArmyMoraleDescription_AdjustFewAlignments_0044B9C8)
    { return RouteNativeAlignmentRow(c); }

    bool IsScopedLeadershipMoralePopup(const NativeDialogTextLayout* dialog)
    {
        return leadershipMoralePopupText && dialog && dialog->text.String() &&
            strcmp(dialog->text.String(), leadershipMoralePopupText) == 0;
    }

    _LHF_(MoralePopup_SetMinimumTextWidth_004F658A)
    {
        const auto dialog = reinterpret_cast<const NativeDialogTextLayout*>(c->esi);
        if (IsScopedLeadershipMoralePopup(dialog))
            c->eax = acm::MoralePopupTextWidth(static_cast<int>(c->eax), leadershipMoralePopupTextWidth);
        // Vanilla stores this trial width, then calculates wrapped height and may widen it.
        return EXEC_DEFAULT;
    }

    _LHF_(MoralePopup_PreserveMinimumTextWidth_004F663F)
    {
        const auto dialog = reinterpret_cast<NativeDialogTextLayout*>(c->esi);
        if (IsScopedLeadershipMoralePopup(dialog))
            dialog->textWidth = acm::MoralePopupTextWidth(dialog->textWidth, leadershipMoralePopupTextWidth);
        // Font measurement can shrink the trial width to the longest line. Keep the floor
        // before vanilla adds borders and positions the text, icons and complete dialog.
        return EXEC_DEFAULT;
    }

    void __stdcall MoralePopup_MoveAlignmentExplanation_004F6C00(HiHook* hook,
        const char* originalText, int buttons, int x, int y, int picture1, int subtype1,
        int picture2, int subtype2, int picture3, int subtype3, int timeout, int flags)
    {
        std::string text;
        const char* displayedText = originalText;
        bool hasLeadershipExplanation = false;
        int minimumTextWidth = 0;
        if (originalText && picture1 >= 14 && picture1 <= 16)
        {
            const char* intro = reinterpret_cast<const char* const*>(0x006A5824)[picture1 - 14];
            if (intro)
            {
                text = originalText;
                bool changed = acm::RemoveLegacyAlignmentExplanation(text);
                for (int level = 3; level <= 5; ++level)
                {
                    const char* explanation = Explanation(level);
                    if (*explanation && text.find(explanation) != std::string::npos)
                    {
                        hasLeadershipExplanation = true;
                        if (const auto font = h3::H3MediumFont::Get())
                        {
                            // Measure visible glyphs, not the translated sentence's color tags.
                            const std::string visibleExplanation = acm::WithoutFontColorMarkup(explanation);
                            minimumTextWidth = font->GetMaxLineWidth(visibleExplanation.c_str());
                        }
                        changed = acm::MoveExplanation(text, intro, explanation) || changed;
                        break;
                    }
                }
                if (changed)
                    displayedText = text.c_str();
            }
        }
        struct PopupScope
        {
            const char* previous;
            int previousWidth;
            PopupScope(const char* current, int width)
                : previous(leadershipMoralePopupText), previousWidth(leadershipMoralePopupTextWidth)
            {
                leadershipMoralePopupText = current;
                leadershipMoralePopupTextWidth = width;
            }
            ~PopupScope()
            {
                leadershipMoralePopupText = previous;
                leadershipMoralePopupTextWidth = previousWidth;
            }
        } scope(hasLeadershipExplanation ? displayedText : nullptr, minimumTextWidth);
        // Preserve all native/HD dialog parameters, ownership and right-click dismissal behavior.
        FASTCALL_12(void, hook->GetDefaultFunc(), displayedText, buttons, x, y, picture1, subtype1,
            picture2, subtype2, picture3, subtype3, timeout, flags);
    }

#ifdef ACM_LEADERSHIP_TESTING
#include "NativeTests.h"
#endif

    void __stdcall InstallHooks(Era::TEvent*)
    {
        static bool installed = false;
        if (installed)
            return;
        installed = true;

        // Retire this legacy DLL's hooks through its patcher instance, including its repeated installer.
        // No executable byte arrays are restored and no unrelated plugin's patches are removed.
        if (auto legacy = patcher->GetInstance("EraPlugin.Combat_LeadershipDecreasesFactionsNum.daemon_n"))
            legacy->UndoAll();

        patches->WriteLoHook(0x004E3C48, HeroMorale_UseActualLeadershipLevel_004E3C48);
        patches->WriteHiHook(0x0044AB30, SPLICE_, EXTENDED_, THISCALL_, ArmyMorale_ApplyLeadershipAlignmentReduction_0044AB30);
        patches->WriteHiHook(0x0044B630, SPLICE_, EXTENDED_, THISCALL_, ArmyMoraleDescription_PreserveOriginalContext_0044B630);
        patches->WriteLoHook(0x004DCBA6, HeroMoraleDescription_LabelActualLeadershipLevel_004DCBA6);
        patches->WriteLoHook(0x004DCCC7, HeroMoraleDescription_AddAlignmentExplanation_004DCCC7);
        patches->WriteLoHook(0x0044B97A, ArmyMoraleDescription_AdjustMixedAlignments_0044B97A);
        patches->WriteLoHook(0x0044B9C8, ArmyMoraleDescription_AdjustFewAlignments_0044B9C8);
        patches->WriteHiHook(0x004F6C00, SPLICE_, EXTENDED_, FASTCALL_, MoralePopup_MoveAlignmentExplanation_004F6C00);
        patches->WriteLoHook(0x004F658A, MoralePopup_SetMinimumTextWidth_004F658A);
        patches->WriteLoHook(0x004F663F, MoralePopup_PreserveMinimumTextWidth_004F663F);
#ifdef ACM_LEADERSHIP_TESTING
        Era::RegisterHandler(RunNativeTests, "OnAfterCreateWindow");
#endif
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        Era::ConnectEra(module, "ACM_Leadership");
        patcher = GetPatcher();
        patches = patcher->CreateInstance("ACM_Leadership");
        Era::RegisterHandler(InstallHooks, "OnAfterWoG");
    }
    return TRUE;
}
