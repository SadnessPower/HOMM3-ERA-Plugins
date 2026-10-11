#include "patcher_x86_commented.hpp"

#pragma warning(disable : 4996)
#include "H3API.hpp"
#include "era.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
    using namespace h3;

    constexpr int CREATURE_INFO_DIALOG_VTABLE = 0x00643C24;
    constexpr int HERO_WINDOW_OPEN = 0x005FF0A0;
    constexpr int HERO_WINDOW_CLOSE = 0x005FF220;
    constexpr int HERO_SCREEN_WINDOW_POINTER = 0x00698AC8;
    constexpr int WINDOW_MANAGER_ACTIVE_DIALOG_OFFSET = 0x58;
    constexpr int PANEL_GAP = 4;
    constexpr int PANEL_MINIMUM_HEIGHT = 64;
    constexpr int PANEL_MARGIN = 20;
    constexpr int PANEL_TITLE_HEIGHT = 19;
    constexpr int PANEL_SECTION_GAP = 8;
    constexpr int PANEL_ABILITY_GAP = 8;
    constexpr int PANEL_SHADOW_SIZE = 8;
    constexpr int PANEL_DARKEN_AMOUNT = 24;
    constexpr int CREATURE_TITLE_COLOR = 7;
    constexpr BYTE CREATURE_NAME_BORDER_RED = 0x6B;
    constexpr BYTE CREATURE_NAME_BORDER_GREEN = 0x5A;
    constexpr BYTE CREATURE_NAME_BORDER_BLUE = 0x4A;

    Patcher* g_patcher = nullptr;
    PatcherInstance* g_instance = nullptr;

    struct SkillDefinition
    {
        std::string id;
        std::string name;
        std::string description;
    };

    struct HighlightedSpan
    {
        std::string color;
        std::string text;
    };

    struct HintRow
    {
        std::string color;
        SkillDefinition skill;
    };

    struct PanelData
    {
        H3LoadedPcx16* image;
        H3LoadedPcx16* savedBackground;
        int x;
        int y;
        int width;
        int height;
        bool isHeroQuickView;
    };

    std::map<H3BaseDlg*, PanelData> g_panels;

    char AsciiLower(char value)
    {
        if (value >= 'A' && value <= 'Z')
            return static_cast<char>(value - 'A' + 'a');
        return value;
    }

    std::string RemoveStandaloneNumbers(const std::string& value)
    {
        std::string result;
        size_t position = 0;
        while (position < value.size())
        {
            if (std::isdigit(static_cast<unsigned char>(value[position])) &&
                (position == 0 || std::isspace(static_cast<unsigned char>(value[position - 1]))))
            {
                size_t end = position;
                while (end < value.size() && std::isdigit(static_cast<unsigned char>(value[end])))
                    ++end;
                if (end == value.size() || std::isspace(static_cast<unsigned char>(value[end])))
                {
                    position = end;
                    while (position < value.size() &&
                           std::isspace(static_cast<unsigned char>(value[position])) &&
                           !result.empty() &&
                           std::isspace(static_cast<unsigned char>(result.back())))
                    {
                        ++position;
                    }
                    continue;
                }
            }

            result.push_back(value[position]);
            ++position;
        }
        return result;
    }

    std::string Trim(const std::string& value)
    {
        size_t first = 0;
        while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
            ++first;

        size_t last = value.size();
        while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
            --last;

        return value.substr(first, last - first);
    }

    bool ContainsTranslatedName(const std::string& text, const std::string& name)
    {
        if (name.empty())
            return false;
        if (text.find(name) != std::string::npos)
            return true;

        const auto containsIgnoringAsciiCase = [&name](const std::string& candidate)
        {
            return std::search(
                candidate.begin(), candidate.end(), name.begin(), name.end(),
                [](char left, char right)
                {
                    return AsciiLower(left) == AsciiLower(right);
                }) != candidate.end();
        };

        if (containsIgnoringAsciiCase(text))
            return true;

        const std::string withoutStandaloneNumbers = RemoveStandaloneNumbers(text);
        return withoutStandaloneNumbers != text && containsIgnoringAsciiCase(withoutStandaloneNumbers);
    }

    bool TranslationExists(const std::string& key, std::string& value)
    {
        const char* translated = Era::tr(key.c_str());
        if (!translated || !*translated || key == translated)
            return false;
        value = translated;
        return true;
    }

    std::vector<SkillDefinition> LoadSkillDefinitions()
    {
        const std::string idsKey = "combatHintDialog.skillIds";
        std::string idsText;
        if (!TranslationExists(idsKey, idsText))
            return {};

        std::vector<SkillDefinition> result;
        size_t start = 0;
        while (start <= idsText.size())
        {
            const size_t separator = idsText.find('|', start);
            const size_t length = separator == std::string::npos ? std::string::npos : separator - start;
            const std::string id = Trim(idsText.substr(start, length));
            if (!id.empty())
            {
                SkillDefinition definition;
                definition.id = id;

                const std::string baseKey = "combatHintDialog.skills." + id;
                if (TranslationExists(baseKey + ".name", definition.name) &&
                    TranslationExists(baseKey + ".description", definition.description))
                {
                    result.push_back(definition);
                }
            }

            if (separator == std::string::npos)
                break;
            start = separator + 1;
        }

        std::sort(result.begin(), result.end(),
            [](const SkillDefinition& left, const SkillDefinition& right)
            {
                return left.name.size() > right.name.size();
            });
        return result;
    }

    bool ReadColorOpening(const std::string& text, size_t position, std::string& color, size_t& consumed)
    {
        if (position >= text.size() || text[position] != '{')
            return false;

        size_t colorStart = position + 1;
        if (colorStart < text.size() && text[colorStart] == '~')
            ++colorStart;

        if (colorStart + 6 >= text.size() || text[colorStart + 6] != '}')
            return false;
        for (size_t i = 0; i < 6; ++i)
        {
			const char value = text[colorStart + i];
			const bool isAsciiHex = (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
			if (!isAsciiHex)
                return false;
        }

        color = text.substr(colorStart, 6);
        consumed = colorStart + 7 - position;
        return true;
    }

    void AddHighlightedSpan(std::vector<HighlightedSpan>& spans, const std::string& color, const std::string& text)
    {
        const std::string trimmed = Trim(text);
        if (!trimmed.empty())
            spans.push_back({ color, trimmed });
    }

    std::vector<HighlightedSpan> ExtractHighlightedSpans(const char* description)
    {
        std::vector<HighlightedSpan> spans;
        if (!description || !*description)
            return spans;

        const std::string text(description);
        bool insideCustomColor = false;
        std::string color;
        std::string current;

        size_t position = 0;
        while (position < text.size())
        {
            std::string openingColor;
            size_t openingLength = 0;
            if (ReadColorOpening(text, position, openingColor, openingLength))
            {
                if (insideCustomColor)
                    AddHighlightedSpan(spans, color, current);
                current.clear();
                color = openingColor;
                insideCustomColor = true;
                position += openingLength;
                continue;
            }

            if (text.compare(position, 3, "{~}") == 0 || text.compare(position, 2, "{}") == 0)
            {
                if (insideCustomColor)
                    AddHighlightedSpan(spans, color, current);
                current.clear();
                color.clear();
                insideCustomColor = false;
                position += text.compare(position, 3, "{~}") == 0 ? 3 : 2;
                continue;
            }

            if (!insideCustomColor && text[position] == '{')
            {
                const size_t closing = text.find('}', position + 1);
                if (closing != std::string::npos)
                {
                    AddHighlightedSpan(spans, "FFD700", text.substr(position + 1, closing - position - 1));
                    position = closing + 1;
                    continue;
                }
            }

            if (insideCustomColor)
                current.push_back(text[position]);
            ++position;
        }

        if (insideCustomColor)
            AddHighlightedSpan(spans, color, current);
        return spans;
    }

    std::vector<std::string> SplitAbilityClauses(const std::string& text)
    {
        std::vector<std::string> result;
        size_t start = 0;
        for (size_t position = 0; position <= text.size(); ++position)
        {
            const bool atEnd = position == text.size();
            const bool isSeparator = !atEnd &&
                (text[position] == '.' || text[position] == ';' || text[position] == '\r' || text[position] == '\n');
            if (!atEnd && !isSeparator)
                continue;

            const std::string clause = Trim(text.substr(start, position - start));
            if (!clause.empty())
                result.push_back(clause);
            start = position + 1;
        }
        return result;
    }

    std::vector<HintRow> BuildHintRows(const char* description, const std::vector<SkillDefinition>& definitions)
    {
        std::vector<HintRow> rows;
        std::set<std::string> usedSkills;

        for (const HighlightedSpan& span : ExtractHighlightedSpans(description))
        {
            for (const std::string& clause : SplitAbilityClauses(span.text))
            {
                for (const SkillDefinition& definition : definitions)
                {
                    if (!ContainsTranslatedName(clause, definition.name))
                        continue;
                    if (usedSkills.insert(definition.id).second)
                        rows.push_back({ span.color, definition });
                    break;
                }
            }
        }
        return rows;
    }

    std::vector<HintRow> BuildHintRowsForCreature(int creatureId)
    {
        if (creatureId < 0)
            return {};

        const std::vector<SkillDefinition> definitions = LoadSkillDefinitions();
        if (definitions.empty())
            return {};

        return BuildHintRows(P_CreatureInformation[creatureId].description, definitions);
    }

    int CalculateAbilityHeight(const HintRow& row, int textWidth, int lineHeight, H3Font* font)
    {
        const std::string rowText = row.skill.name + ": " + row.skill.description;
        if (font)
            return std::max(1, font->GetLinesCountInText(rowText.c_str(), textWidth)) * lineHeight;

        const int approximateCharactersPerLine = std::max(28, textWidth / 6);
        const int lines = std::max(
            1,
            (static_cast<int>(rowText.size()) + approximateCharactersPerLine - 1) /
                approximateCharactersPerLine);
        return lines * lineHeight;
    }

    int CalculatePanelHeight(const std::vector<HintRow>& rows, int textWidth, int maximumHeight)
    {
        int lineHeight = 12;
        H3Font* font = P_TinyFont;
        if (font)
            lineHeight = std::max(1, static_cast<int>(font->height) + 2);

        int abilitiesHeight = 0;
        for (const HintRow& row : rows)
            abilitiesHeight += CalculateAbilityHeight(row, textWidth, lineHeight, font);

        if (rows.size() > 1)
            abilitiesHeight += (static_cast<int>(rows.size()) - 1) * PANEL_ABILITY_GAP;

        const int desiredHeight =
            PANEL_MARGIN + PANEL_TITLE_HEIGHT + PANEL_SECTION_GAP + abilitiesHeight + PANEL_MARGIN;
        return std::min(maximumHeight, std::max(PANEL_MINIMUM_HEIGHT, desiredHeight));
    }

    void DestroyPanelData(PanelData& panel)
    {
        if (panel.savedBackground)
        {
            panel.savedBackground->Destroy(TRUE);
            panel.savedBackground = nullptr;
        }
        if (panel.image)
        {
            panel.image->Destroy(TRUE);
            panel.image = nullptr;
        }
    }

    void DrawInsetRectangle(
        H3LoadedPcx16* image,
        int x,
        int y,
        int width,
        int height,
        BYTE borderRed,
        BYTE borderGreen,
        BYTE borderBlue)
    {
        image->DrawFrame(
            x - 1,
            y - 1,
            width + 2,
            height + 2,
            borderRed,
            borderGreen,
            borderBlue);
        image->DarkenArea(x + 1, y + 1, width - 2, height - 2, PANEL_DARKEN_AMOUNT);
        image->SinkArea(x, y, width, height);
    }

    H3LoadedPcx16* CreatePanelImage(
        const std::vector<HintRow>& rows,
        int panelWidth,
        int panelHeight)
    {
        H3LoadedPcx16* image = H3LoadedPcx16::Create(panelWidth, panelHeight);
        if (!image)
            return nullptr;
        if (!image->BackgroundRegion(0, 0, panelWidth, panelHeight, FALSE) ||
            !image->FrameRegion(0, 0, panelWidth, panelHeight, FALSE, IntAt(0x69CCF4), FALSE))
        {
            image->Destroy(TRUE);
            return nullptr;
        }

        const int boxX = PANEL_MARGIN;
        const int boxWidth = panelWidth - 2 * PANEL_MARGIN;
        const int titleY = PANEL_MARGIN;
        const int abilitiesY = titleY + PANEL_TITLE_HEIGHT + PANEL_SECTION_GAP;
        DrawInsetRectangle(
            image,
            boxX,
            titleY,
            boxWidth,
            PANEL_TITLE_HEIGHT,
            CREATURE_NAME_BORDER_RED,
            CREATURE_NAME_BORDER_GREEN,
            CREATURE_NAME_BORDER_BLUE);

        std::string title = "Creature abilities";
        TranslationExists("combatHintDialog.title", title);
        image->TextDraw(
            P_SmallFont,
            title.c_str(),
            boxX + 4,
            titleY,
            boxWidth - 8,
            PANEL_TITLE_HEIGHT,
            static_cast<eTextColor>(CREATURE_TITLE_COLOR),
            eTextAlignment::MIDDLE_CENTER);

        int lineHeight = 12;
        H3Font* font = P_TinyFont;
        if (font)
            lineHeight = std::max(1, static_cast<int>(font->height) + 2);

        const int textWidth = boxWidth - 8;
        int abilityY = abilitiesY;
        for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
        {
            const HintRow& row = rows[rowIndex];
            const int abilityHeight = CalculateAbilityHeight(row, textWidth, lineHeight, font);
            DrawInsetRectangle(
                image,
                boxX,
                abilityY,
                boxWidth,
                abilityHeight,
                CREATURE_NAME_BORDER_RED,
                CREATURE_NAME_BORDER_GREEN,
                CREATURE_NAME_BORDER_BLUE);

			std::string hintText = "{~" + row.color + "}" + row.skill.name + "{~}: " + row.skill.description;
            image->TextDraw(
                font,
                hintText.c_str(),
                boxX + 4,
                abilityY,
                textWidth,
                abilityHeight,
                eTextColor::REGULAR,
                eTextAlignment::TOP_LEFT);
            abilityY += abilityHeight;
            if (rowIndex + 1 < rows.size())
                abilityY += PANEL_ABILITY_GAP;
        }
        return image;
    }

    void RegisterSkillHints(H3CreatureInfoDlg* dialog)
    {
        if (!dialog)
            return;

		const auto existingPanel = g_panels.find(dialog);
		if (existingPanel != g_panels.end())
		{
			DestroyPanelData(existingPanel->second);
			g_panels.erase(existingPanel);
		}
        const std::vector<HintRow> rows = BuildHintRowsForCreature(dialog->creatureId);
        if (rows.empty())
            return;

        const int panelWidth = dialog->GetWidth();
        const int textWidth = panelWidth - 2 * PANEL_MARGIN - 8;
        const int panelHeight = CalculatePanelHeight(
            rows,
            textWidth,
            H3GameHeight::Get() - PANEL_SHADOW_SIZE);
        H3LoadedPcx16* image = CreatePanelImage(rows, panelWidth, panelHeight);
        if (!image)
            return;

		H3WindowManager* manager = H3WindowManager::Get();
		H3BaseDlg* heroScreen = *reinterpret_cast<H3BaseDlg**>(HERO_SCREEN_WINDOW_POINTER);
		H3BaseDlg* activeDialog = manager ? *reinterpret_cast<H3BaseDlg**>(reinterpret_cast<char*>(manager) + WINDOW_MANAGER_ACTIVE_DIALOG_OFFSET) : nullptr;
		const bool isHeroScreenActive = manager && heroScreen
			&& (manager->firstDlg == heroScreen || manager->lastDlg == heroScreen || activeDialog == heroScreen);
        // Hero quick-view windows keep using their original dimensions and item list. Resizing or appending an item corrupts their real-time message loop.
        g_panels.emplace(dialog, PanelData{ image, nullptr, 0, 0, panelWidth, panelHeight, isHeroScreenActive });
    }

    bool CopyPixels(
        H3LoadedPcx16* source,
        int sourceX,
        int sourceY,
        H3LoadedPcx16* destination,
        int destinationX,
        int destinationY,
        int width,
        int height)
    {
        if (!source || !destination || width <= 0 || height <= 0 ||
            sourceX < 0 || sourceY < 0 || destinationX < 0 || destinationY < 0 ||
            sourceX + width > source->width || sourceY + height > source->height ||
            destinationX + width > destination->width || destinationY + height > destination->height)
        {
            return false;
        }

        const int bytesPerPixel = H3BitMode::Get() == 4 ? 4 : 2;
        const size_t bytesPerRow = static_cast<size_t>(width) * bytesPerPixel;
        for (int row = 0; row < height; ++row)
        {
            std::memcpy(
                destination->GetRow(destinationY + row) + destinationX * bytesPerPixel,
                source->GetRow(sourceY + row) + sourceX * bytesPerPixel,
                bytesPerRow);
        }
        return true;
    }

    bool PreparePanel(H3BaseDlg* dialog, PanelData& panel)
    {
        H3WindowManager* manager = H3WindowManager::Get();
        H3LoadedPcx16* screen = manager ? manager->GetDrawBuffer() : nullptr;
        if (!screen || !panel.image)
            return false;

        const int screenWidth = H3GameWidth::Get();
        const int screenHeight = H3GameHeight::Get();
        const int gap = panel.isHeroQuickView ? PANEL_SHADOW_SIZE : PANEL_GAP;
        int dialogX = dialog->GetX();
        int dialogY = dialog->GetY();

        if (dialogX + dialog->GetWidth() + gap + panel.width + PANEL_SHADOW_SIZE <= screenWidth)
        {
            panel.x = dialogX + dialog->GetWidth() + gap;
        }
        else if (dialogX - gap - panel.width >= 0)
        {
            panel.x = dialogX - gap - panel.width;
        }
        else
        {
            const int combinedWidth = dialog->GetWidth() + gap + panel.width + PANEL_SHADOW_SIZE;
            dialogX = std::max(0, (screenWidth - combinedWidth) / 2);
            panel.x = dialogX + dialog->GetWidth() + gap;
        }

        const int requiredHeight = std::max(dialog->GetHeight(), panel.height) + PANEL_SHADOW_SIZE;
        dialogY = std::max(0, std::min(dialogY, screenHeight - requiredHeight));
        panel.y = dialogY;
		char* rawDialog = reinterpret_cast<char*>(dialog);
		*reinterpret_cast<int*>(rawDialog + 0x18) = dialogX;
		*reinterpret_cast<int*>(rawDialog + 0x1C) = dialogY;

        if (panel.savedBackground)
            panel.savedBackground->Destroy(TRUE);
        panel.savedBackground = H3LoadedPcx16::Create(
            panel.width + PANEL_SHADOW_SIZE,
            panel.height + PANEL_SHADOW_SIZE);
        if (!panel.savedBackground)
            return false;

        if (!CopyPixels(
                screen,
                panel.x,
                panel.y,
                panel.savedBackground,
                0,
                0,
                panel.width + PANEL_SHADOW_SIZE,
                panel.height + PANEL_SHADOW_SIZE))
        {
            panel.savedBackground->Destroy(TRUE);
            panel.savedBackground = nullptr;
            return false;
        }

        screen->DrawShadow(panel.x + panel.width, panel.y + 9, 7, panel.height - 9);
        screen->DrawShadow(panel.x + panel.width, panel.y + 8, 8, panel.height - 8);
        screen->DrawShadow(panel.x + 9, panel.y + panel.height, panel.width - 2, 7);
        screen->DrawShadow(panel.x + 8, panel.y + panel.height, panel.width, 8);
        return true;
    }

    void DrawPanel(PanelData& panel)
    {
        H3WindowManager* manager = H3WindowManager::Get();
        H3LoadedPcx16* screen = manager ? manager->GetDrawBuffer() : nullptr;
        if (!screen || !panel.image || !panel.savedBackground)
            return;

        panel.image->DrawToPcx16(panel.x, panel.y, FALSE, screen);
        manager->H3Redraw(
            panel.x,
            panel.y,
            panel.width + PANEL_SHADOW_SIZE,
            panel.height + PANEL_SHADOW_SIZE);
    }

    void RestorePanel(PanelData& panel, bool update)
    {
        H3WindowManager* manager = H3WindowManager::Get();
        H3LoadedPcx16* screen = manager ? manager->GetDrawBuffer() : nullptr;
        if (!screen || !panel.savedBackground)
            return;

        CopyPixels(
            panel.savedBackground,
            0,
            0,
            screen,
            panel.x,
            panel.y,
            panel.width + PANEL_SHADOW_SIZE,
            panel.height + PANEL_SHADOW_SIZE);
        if (update)
        {
            manager->H3Redraw(
                panel.x,
                panel.y,
                panel.width + PANEL_SHADOW_SIZE,
                panel.height + PANEL_SHADOW_SIZE);
        }
        panel.savedBackground->Destroy(TRUE);
        panel.savedBackground = nullptr;
    }

    int __stdcall HeroWindowOpen_005FF0A0(
        HiHook* hook,
        H3BaseDlg* dialog,
        int priority,
        bool update)
    {
		const bool isCreatureInfoDialog = dialog && *reinterpret_cast<int*>(dialog) == CREATURE_INFO_DIALOG_VTABLE;
		if (isCreatureInfoDialog)
            RegisterSkillHints(reinterpret_cast<H3CreatureInfoDlg*>(dialog));

        bool panelPrepared = false;
        auto found = g_panels.find(dialog);
        if (found != g_panels.end())
            panelPrepared = PreparePanel(dialog, found->second);

        const int result = THISCALL_3(
            int, hook->GetDefaultFunc(), dialog, priority, update);

        // The original Open call runs the hero quick-view message loop. Its Close hook may erase this entry before Open returns, so never retain that iterator.
        found = g_panels.find(dialog);
        if (found != g_panels.end())
        {
            if (panelPrepared)
            {
                if (result == 0)
                    DrawPanel(found->second);
                else
                    RestorePanel(found->second, update);
            }

            if (result != 0)
            {
                DestroyPanelData(found->second);
                g_panels.erase(found);
            }
        }
        return result;
    }

    void __stdcall HeroWindowClose_005FF220(
        HiHook* hook,
        H3BaseDlg* dialog,
        bool update)
    {
        THISCALL_2(VOID, hook->GetDefaultFunc(), dialog, update);

        const auto found = g_panels.find(dialog);
        if (found != g_panels.end())
        {
            RestorePanel(found->second, update);
            DestroyPanelData(found->second);
            g_panels.erase(found);
        }
    }

}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_patcher = GetPatcher();
        g_instance = g_patcher->CreateInstance("CombatHintDialog");
        Era::ConnectEra(module, "CombatHintDialog");

        g_instance->WriteHiHook(
            HERO_WINDOW_OPEN, SPLICE_, EXTENDED_, THISCALL_, HeroWindowOpen_005FF0A0);
        g_instance->WriteHiHook(
            HERO_WINDOW_CLOSE, SPLICE_, EXTENDED_, THISCALL_, HeroWindowClose_005FF220);
    }
    return TRUE;
}
