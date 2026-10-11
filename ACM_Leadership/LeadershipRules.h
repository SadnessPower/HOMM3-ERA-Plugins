#pragma once
#include <algorithm>
#include <string>

namespace acm
{
    constexpr int kNativeDialogMaximumTextWidth = 578;

    inline int MoralePopupTextWidth(int vanillaWidth, int explanationWidth)
    {
        // Fit the translated sentence when possible, retaining native wrapping limits
        // and any larger width already chosen by vanilla or another plugin.
        return (std::max)(vanillaWidth,
            (std::max)(0, (std::min)(explanationWidth, kNativeDialogMaximumTextWidth)));
    }

    inline std::string WithoutFontColorMarkup(std::string text)
    {
        size_t color = 0;
        while ((color = text.find("{~", color)) != std::string::npos)
        {
            const size_t colorEnd = text.find('}', color + 2);
            if (colorEnd == std::string::npos)
                break;
            text.erase(color, colorEnd - color + 1);
        }
        return text;
    }

    inline int LeadershipLevel(int nativeLevel, bool master, bool grandmaster)
    {
        if (nativeLevel < 3)
            return (std::max)(0, nativeLevel);
        return grandmaster ? 5 : master ? 4 : 3;
    }

    inline int AlignmentReduction(int level)
    {
        return (std::max)(0, level - 2);
    }

    inline int AdjustedAlignments(int count, int level)
    {
        return count <= 0 ? 0 : (std::max)(1, count - AlignmentReduction(level));
    }

    // Place our explanation directly below the native introduction, without an empty line.
    inline bool MoveExplanation(std::string& text, const std::string& introduction,
        const std::string& explanation)
    {
        if (introduction.empty() || explanation.empty())
            return false;
        const std::string line = "\n" + explanation;
        const size_t linePosition = text.find(line);
        const size_t introPosition = text.find(introduction);
        if (linePosition == std::string::npos || introPosition == std::string::npos ||
            linePosition < introPosition + introduction.size())
            return false;
        const size_t introEnd = introPosition + introduction.size();
        if (linePosition == introEnd)
            return false;
        // Also normalize text where an older formatter has already moved our line,
        // but left one or more empty lines between it and the introduction.
        const size_t firstNonNewline = text.find_first_not_of("\r\n", introEnd);
        if (firstNonNewline == linePosition + 1)
            text.erase(introEnd, linePosition + line.size() - introEnd);
        else
            text.erase(linePosition, line.size());
        text.insert(introEnd, line);
        return true;
    }
}
