#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "LcarsColors.h"

class LcarsLookAndFeel : public foleys::LookAndFeel {
   public:
    LcarsLookAndFeel() {
        setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::black);
        setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::black);
    }

   private:
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool isMouseOver,
                       bool isMouseDown) override {
        const auto activeArea = button.getActiveArea();

        if (button.getToggleState()) {
            g.setColour(LcarsColors::orange);
        } else {
            g.setColour(LcarsColors::red);
        }
        g.fillRect(activeArea);

        g.setColour(juce::Colours::black);
        g.setFont(14.0f);
        g.drawText(button.getButtonText(), activeArea, juce::Justification::centred);
    }
};
