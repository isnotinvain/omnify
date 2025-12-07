#pragma once

#include <BinaryData.h>
#include <foleys_gui_magic/foleys_gui_magic.h>

#include "LcarsColors.h"

class LcarsLookAndFeel : public foleys::LookAndFeel {
   public:
    LcarsLookAndFeel() {
        orbitronTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::OrbitronRegular_ttf, BinaryData::OrbitronRegular_ttfSize);

        setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::black);
        setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::black);

        // Default popup menu colors (can be overridden via XML)
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF1A1A2E));
        setColour(juce::PopupMenu::textColourId, LcarsColors::sunflower);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, LcarsColors::orange.withAlpha(0.3f));
        setColour(juce::PopupMenu::highlightedTextColourId, LcarsColors::spaceWhite);
    }

   private:
    juce::Typeface::Ptr orbitronTypeface;

    juce::Font getOrbitronFont(float height) const {
        return juce::Font(orbitronTypeface).withHeight(height);
    }

    juce::Font getLabelFont(juce::Label& label) override {
        return getOrbitronFont(label.getFont().getHeight());
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override {
        return getOrbitronFont(juce::jmin(15.0f, (float)box.getHeight() * 0.85f));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override {
        return getOrbitronFont(juce::jmin(15.0f, buttonHeight * 0.6f));
    }

    juce::Font getPopupMenuFont() override { return getOrbitronFont(15.0f); }

    juce::Font getMenuBarFont(juce::MenuBarComponent&, int, const juce::String&) override {
        return getOrbitronFont(15.0f);
    }

    juce::Font getAlertWindowTitleFont() override { return getOrbitronFont(18.0f); }

    juce::Font getAlertWindowMessageFont() override { return getOrbitronFont(15.0f); }

    juce::Font getAlertWindowFont() override { return getOrbitronFont(14.0f); }

    juce::Font getSliderPopupFont(juce::Slider&) override { return getOrbitronFont(14.0f); }

    juce::Font getTabButtonFont(juce::TabBarButton&, float height) override {
        return getOrbitronFont(height * 0.6f);
    }

    juce::Font getSidePanelTitleFont(juce::SidePanel&) override { return getOrbitronFont(18.0f); }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX,
                      int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height);

        // Draw background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

        // Draw outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), 4.0f, 2.0f);

        // Draw a single filled downward-pointing triangle
        const float arrowSize = 6.0f;
        const float arrowX = (float)width - arrowSize - 8.0f;  // 8px padding from right edge
        const float arrowY = (float)height * 0.5f;

        juce::Path path;
        path.addTriangle(arrowX - arrowSize, arrowY - arrowSize * 0.5f,  // top left
                         arrowX + arrowSize, arrowY - arrowSize * 0.5f,  // top right
                         arrowX, arrowY + arrowSize * 0.5f);             // bottom center

        g.setColour(
            box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
        g.fillPath(path);
    }

    int getPopupMenuBorderSize() override { return 2; }

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
        g.setFont(getOrbitronFont(14.0f));
        g.drawText(button.getButtonText(), activeArea, juce::Justification::centred);
    }
};
