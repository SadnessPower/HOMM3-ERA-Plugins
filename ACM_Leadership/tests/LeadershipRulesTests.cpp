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
    assert(acm::WithoutFontColorMarkup("{~g}Grandmaster Leadership{~} decreases alignments by 3.") ==
        "Grandmaster Leadership decreases alignments by 3.");
    assert(acm::WithoutFontColorMarkup("{~gold}Translated title{~}\nSecond line.") ==
        "Translated title\nSecond line.");
    assert(acm::WithoutFontColorMarkup("Unchanged sentence.") == "Unchanged sentence.");
    assert(acm::WithoutFontColorMarkup("Malformed {~tag") == "Malformed {~tag");
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
    for (const auto& intro : {"Good morale may give your armies extra attack in combat.",
        "Your armies have neutral morale.", "Bad morale may cause your armies to freeze in combat."})
        for (int level = 3; level <= 5; ++level)
        {
            const std::string explanation = "{~g}Leadership{~} decreases your group alignments by " +
                std::to_string(level - 2) + ".";
            const std::string prefix = "{Morale}\n\n" + std::string(intro);
            const std::string modifiers = "\n\nCurrent Morale Modifiers\nLeadership +" +
                std::to_string(level) + "\nAll troops of the same alignment +1\nUndead in group -1";
            std::string text = prefix + modifiers + "\n" + explanation;
            assert(acm::MoveExplanation(text, intro, explanation));
            assert(text == prefix + "\n" + explanation + modifiers);
            assert(!acm::MoveExplanation(text, intro, explanation));
            ++checks;

            // The screenshot case: our explanation is already above the modifiers,
            // with an empty line before it and an obsolete explanation after it.
            for (const auto& legacy : {
                "Master Leadership makes your army group share the same alignment.",
                "Grandmaster Leadership makes your army group share the same alignment.",
                "{~g}Master Leadership{~} makes your army group share the same alignment.",
                "{~g}Grandmaster Leadership{~} makes your army group share the same alignment." })
            {
                text = prefix + "\n\n" + explanation + "\n\n" + legacy + modifiers;
                assert(acm::RemoveLegacyAlignmentExplanation(text));
                assert(acm::MoveExplanation(text, intro, explanation));
                assert(text == prefix + "\n" + explanation + modifiers);
                assert(!acm::RemoveLegacyAlignmentExplanation(text));
                assert(!acm::MoveExplanation(text, intro, explanation));
                ++checks;
            }

            // Extra old whitespace before our line is removed; other native spacing stays.
            text = prefix + "\n\n\n" + explanation + modifiers;
            assert(acm::MoveExplanation(text, intro, explanation));
            assert(text == prefix + "\n" + explanation + modifiers);
            ++checks;
        }
    std::string noMarker = "Good morale\n\nCurrent Morale Modifiers\nExpert Leadership +3";
    const auto unchanged = noMarker;
    assert(!acm::MoveExplanation(noMarker, "Good morale", "Missing explanation."));
    assert(noMarker == unchanged);
    assert(!acm::RemoveLegacyAlignmentExplanation(noMarker));
    assert(noMarker == unchanged);
    std::string legacyOnly = "Grandmaster Leadership makes your army group share the same alignment.";
    assert(acm::RemoveLegacyAlignmentExplanation(legacyOnly));
    assert(legacyOnly.empty());
    std::string anotherExplanation = "A different feature makes your army group share the same alignment.";
    assert(!acm::RemoveLegacyAlignmentExplanation(anotherExplanation));
    std::cout << checks + 1 << " Leadership rule/popup cases passed\n";
}
