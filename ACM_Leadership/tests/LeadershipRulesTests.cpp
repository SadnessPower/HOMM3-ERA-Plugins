#include "../LeadershipRules.h"
#include <cassert>
#include <iostream>

int main()
{
    unsigned checks = 0;
    for (int width : { 0, 128, 256, 384, 576, 578, 590, 640, 768, 900 })
    for (int explanationWidth : { -1, 0, 128, 320, 450, 526, 577, 578, 800, 1000 })
    {
        const int expected = (std::max)(width,
            (std::max)(0, (std::min)(explanationWidth, acm::kNativeDialogMaximumTextWidth)));
        assert(acm::MoralePopupTextWidth(width, explanationWidth) == expected);
        assert(acm::MoralePopupTextWidth(expected, explanationWidth) == expected);
        assert(expected >= width);
        ++checks;
    }
    // Arbitrary fixtures exercise markup handling without embedding game text.
    assert(acm::WithoutFontColorMarkup("{~g}token-A{~}: token-B") == "token-A: token-B");
    assert(acm::WithoutFontColorMarkup("{~gold}token-C{~}\ntoken-D") == "token-C\ntoken-D");
    assert(acm::WithoutFontColorMarkup("token-E") == "token-E");
    assert(acm::WithoutFontColorMarkup("token-F {~tag") == "token-F {~tag");
    checks += 4;
    for (int native = 0; native <= 3; ++native)
        for (int master = 0; master <= 1; ++master)
            for (int grandmaster = 0; grandmaster <= 1; ++grandmaster)
            {
                const int level = acm::LeadershipLevel(native, master != 0, grandmaster != 0);
                assert(level == (native < 3 ? native : grandmaster ? 5 : master ? 4 : 3));
                ++checks;
                for (int count = 0; count <= 7; ++count)
                {
                    const int reduction = level >= 3 ? level - 2 : 0;
                    const int expected = count == 0 ? 0 : (std::max)(1, count - reduction);
                    assert(acm::AdjustedAlignments(count, level) == expected);
                    assert(count - expected >= 0 && count - expected <= reduction);
                    ++checks;
                }
            }
    // Every row is supplied by the caller. No fixture depends on a real language,
    // mastery label, modifier caption or wording stored in external properties.
    for (const auto& intro : { "intro-A", "intro-B", "intro-C" })
        for (int variant = 0; variant < 3; ++variant)
        {
            const std::string explanation = "{~g}token-" + std::to_string(variant) + "{~}";
            const std::string prefix = "{title}\n\n" + std::string(intro);
            const std::string modifiers = "\n\nblock-A\nrow-A\nrow-B\nrow-C";
            std::string text = prefix + modifiers + "\n" + explanation;
            assert(acm::MoveExplanation(text, intro, explanation));
            assert(text == prefix + "\n" + explanation + modifiers);
            assert(!acm::MoveExplanation(text, intro, explanation));
            ++checks;

            // Empty lines before the explanation are removed; other spacing stays.
            text = prefix + "\n\n\n" + explanation + modifiers;
            assert(acm::MoveExplanation(text, intro, explanation));
            assert(text == prefix + "\n" + explanation + modifiers);
            assert(!acm::MoveExplanation(text, intro, explanation));
            ++checks;
        }
    std::string noMarker = "intro-X\n\nblock-X\nrow-X";
    const auto unchanged = noMarker;
    assert(!acm::MoveExplanation(noMarker, "intro-X", "missing-X"));
    assert(noMarker == unchanged);
    ++checks;
    assert(!acm::MoveExplanation(noMarker, "missing-Y", "row-X"));
    assert(noMarker == unchanged);
    ++checks;
    assert(!acm::MoveExplanation(noMarker, "", "row-X"));
    assert(noMarker == unchanged);
    ++checks;
    assert(!acm::MoveExplanation(noMarker, "intro-X", ""));
    assert(noMarker == unchanged);
    ++checks;
    std::string reversed = "\nrow-X\nintro-X";
    const auto reversedUnchanged = reversed;
    assert(!acm::MoveExplanation(reversed, "intro-X", "row-X"));
    assert(reversed == reversedUnchanged);
    ++checks;
    std::cout << checks << " rule/popup cases passed\n";
}
