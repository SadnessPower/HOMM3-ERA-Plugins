// Development-only tests, called on the game thread after media/window initialization.
// Not compiled into the normal Release build; never alter an actual hero or savegame.
void __stdcall RunNativeTests(Era::TEvent*)
{
    char logPath[MAX_PATH];
    if (!GetEnvironmentVariableA("ACM_LEADERSHIP_TEST_LOG", logPath, MAX_PATH))
        return;
    FILE* log = nullptr;
    if (fopen_s(&log, logPath, "w") || !log)
        return;
    const int previousMaster = Era::GetAssocVarIntValue("H3_Leadership_0_Hero0");
    const int previousGrandmaster = Era::GetAssocVarIntValue("H3_Leadership_1_Hero0");
    unsigned checks = 0, failures = 0;
    h3::H3Hero hero{};
    hero.id = 0;
    hero.owner = -1;
    hero.level = 0;
    const int armyTypes[][7] = { { 0, 14, 28, 42, 56, 70, 84 }, { 0, 251, 28, 42, 56, 70, 84 } };
    for (const auto& creatures : armyTypes)
    for (int level = 0; level <= 5; ++level)
    {
        hero.secSkill[kLeadership] = static_cast<signed char>((std::min)(3, level));
        Era::SetAssocVarIntValue("H3_Leadership_0_Hero0", level >= 4);
        Era::SetAssocVarIntValue("H3_Leadership_1_Hero0", level >= 5);
        const int heroMorale = THISCALL_4(int, 0x004E3C20, &hero, nullptr, FALSE, FALSE);
        ++checks;
        if (heroMorale != level)
        {
            ++failures;
            fprintf(log, "FAIL native hero morale level=%d actual=%d\n", level, heroMorale);
        }
        for (int slots = 1; slots <= 7; ++slots)
            for (int alliance = 0; alliance <= 1; ++alliance)
            {
                h3::H3Army army;
                for (int slot = 0; slot < 7; ++slot)
                {
                    army.type[slot] = slot < slots ? creatures[slot] : -1;
                    army.count[slot] = slot < slots ? 1 : 0;
                }
                const int count = EffectiveAlignmentCount(&army, alliance);
                const int adjusted = acm::AdjustedAlignments(count, level);
                const int base = ArmyMorale(&army, nullptr, nullptr, nullptr, nullptr, FALSE, alliance, FALSE);
                const int morale = ArmyMorale(&army, &hero, nullptr, nullptr, nullptr, FALSE, alliance, FALSE);
                ++checks;
                if (morale != base + level + count - adjusted)
                {
                    ++failures;
                    fprintf(log, "FAIL army morale lvl=%d slots=%d AA=%d base=%d actual=%d\n", level, slots, alliance, base, morale);
                }
                h3::H3String description;
                THISCALL_10(h3::H3String*, 0x0044B630, &army, &description, 0, morale,
                    &hero, nullptr, nullptr, nullptr, FALSE, alliance);
                const std::string text(description.String(), description.Length());
                const char* sameAlignmentRow = *reinterpret_cast<const char**>(0x006A589C);
                const char* undeadRow = *reinterpret_cast<const char**>(0x006A58A0);
                ++checks;
                if ((text.find(sameAlignmentRow) != std::string::npos) != (adjusted == 1))
                {
                    ++failures;
                    fprintf(log, "FAIL same-alignment row lvl=%d slots=%d AA=%d count=%d adjusted=%d text=%s\n", level, slots, alliance, count, adjusted, text.c_str());
                }
                ++checks;
                if (slots >= 5 && text.find(undeadRow) == std::string::npos)
                {
                    ++failures;
                    fprintf(log, "FAIL missing undead row lvl=%d slots=%d AA=%d text=%s\n", level, slots, alliance, text.c_str());
                }
                if (level >= 3)
                {
                    ++checks;
                    if (text.find(Explanation(level)) == std::string::npos)
                    {
                        ++failures;
                        fprintf(log, "FAIL missing explanation lvl=%d slots=%d AA=%d text=%s\n", level, slots, alliance, text.c_str());
                    }
                }
                if (level > 3)
                {
                    ++checks;
                    if (text.find(Era::tr(level == 5 ? "AC_Misc.Leadership_Morale_Grandmaster" : "AC_Misc.Leadership_Morale_Master")) == std::string::npos)
                    {
                        ++failures;
                        fprintf(log, "FAIL missing mastery modifier lvl=%d slots=%d AA=%d text=%s\n", level, slots, alliance, text.c_str());
                    }
                }
                if (slots == 1 || (slots == 3 && level == 4))
                    fprintf(log, "SAMPLE lvl=%d slots=%d AA=%d morale=%d\n%s\n", level, slots, alliance, morale, text.c_str());
            }
    }
    Era::SetAssocVarIntValue("H3_Leadership_0_Hero0", previousMaster);
    Era::SetAssocVarIntValue("H3_Leadership_1_Hero0", previousGrandmaster);
    const int previousExtendedMaster = Era::GetAssocVarIntValue("H3_Leadership_0_Hero1023");
    const int previousExtendedGrandmaster = Era::GetAssocVarIntValue("H3_Leadership_1_Hero1023");
    hero.id = 1023;
    hero.secSkill[kLeadership] = 3;
    Era::SetAssocVarIntValue("H3_Leadership_0_Hero1023", 1);
    Era::SetAssocVarIntValue("H3_Leadership_1_Hero1023", 1);
    ++checks;
    if (GetLeadershipLevel(&hero) != 5 || THISCALL_4(int, 0x004E3C20, &hero, nullptr, FALSE, FALSE) != 5)
    {
        ++failures;
        fprintf(log, "FAIL extended hero 1023 mastery\n");
    }
    Era::SetAssocVarIntValue("H3_Leadership_0_Hero1023", previousExtendedMaster);
    Era::SetAssocVarIntValue("H3_Leadership_1_Hero1023", previousExtendedGrandmaster);
    fprintf(log, "Native checks=%u failures=%u\n", checks, failures);
    fclose(log);
}
