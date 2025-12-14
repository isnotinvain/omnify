#pragma once

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LcarsColors.h"

class LcarsLookAndFeel : public juce::LookAndFeel_V4 {
   public:
    LcarsLookAndFeel() {
        orbitronTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::OrbitronRegular_ttf, BinaryData::OrbitronRegular_ttfSize);

        setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::black);
        setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::black);
    }

   private:
    juce::Typeface::Ptr orbitronTypeface;

    // Hard-coded settings (values from magic.xml LcarsSettings)
    static constexpr float tabFontSize = 18.0f;
    static constexpr float comboboxLabelFontMultiplier = 0.9f;
    static constexpr float comboboxFontMultiplier = 0.85f;
    static constexpr float textButtonFontMultiplier = 0.6f;
    static constexpr float tabButtonFontMultiplier = 0.6f;
    static constexpr float popupMenuFontSize = 15.0f;
    static constexpr float popupMenuItemFontMultiplier = 1.0f;
    static constexpr float popupMenuItemHeightMultiplier = 1.3f;
    static constexpr float comboboxBorderRadius = 4.0f;
    static constexpr float comboboxArrowSize = 6.0f;
    static constexpr float comboboxArrowPadding = 8.0f;
    static constexpr float popupMenuBorderSize = 2.0f;
    static constexpr float buttonBorderThickness = 2.0f;

    juce::Font getOrbitronFont(float height) const {
        return juce::Font(juce::FontOptions(orbitronTypeface).withHeight(height));
    }

    juce::Font getLabelFont(juce::Label& label) override {
        // For combo box labels, scale based on height
        if (dynamic_cast<juce::ComboBox*>(label.getParentComponent()) != nullptr) {
            return getOrbitronFont(label.getHeight() * comboboxLabelFontMultiplier);
        }
        return getOrbitronFont(label.getFont().getHeight());
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override {
        return getOrbitronFont(
            juce::jmin(popupMenuFontSize, static_cast<float>(box.getHeight()) * comboboxFontMultiplier));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override {
        return getOrbitronFont(static_cast<float>(buttonHeight) * textButtonFontMultiplier);
    }

    juce::Font getPopupMenuFont() override { return getOrbitronFont(popupMenuFontSize); }

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& text, bool isSeparator,
                                              int standardMenuItemHeight, int& idealWidth,
                                              int& idealHeight,
                                              const juce::PopupMenu::Options&) override {
        if (isSeparator) {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        } else {
            float fontSize = popupMenuFontSize;
            idealHeight = static_cast<int>(fontSize * popupMenuItemHeightMultiplier);
            auto font = getOrbitronFont(fontSize);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0, 0);
            idealWidth =
                static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + idealHeight * 2;
        }
    }

    juce::Font getMenuBarFont(juce::MenuBarComponent&, int, const juce::String&) override {
        return getOrbitronFont(15.0f);
    }

    juce::Font getAlertWindowTitleFont() override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    juce::Font getAlertWindowMessageFont() override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    juce::Font getAlertWindowFont() override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    juce::Font getSliderPopupFont(juce::Slider&) override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    juce::Font getTabButtonFont(juce::TabBarButton&, float height) override {
        return getOrbitronFont(height * tabButtonFontMultiplier);
    }

    juce::Font getSidePanelTitleFont(juce::SidePanel&) override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int,
                      juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height);

        // Draw background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), comboboxBorderRadius);

        // Draw outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.0f), comboboxBorderRadius, 2.0f);

        // Draw a single filled downward-pointing triangle
        const float arrowX = static_cast<float>(width) - comboboxArrowSize - comboboxArrowPadding;
        const float arrowY = static_cast<float>(height) * 0.5f;

        juce::Path path;
        path.addTriangle(arrowX - comboboxArrowSize, arrowY - comboboxArrowSize * 0.5f,  // top left
                         arrowX + comboboxArrowSize, arrowY - comboboxArrowSize * 0.5f,  // top right
                         arrowX, arrowY + comboboxArrowSize * 0.5f);                     // bottom center

        g.setColour(
            box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
        g.fillPath(path);
    }

    int getPopupMenuBorderSize() override { return static_cast<int>(popupMenuBorderSize); }

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool, bool) override {
        const auto activeArea = button.getActiveArea();

        if (button.getToggleState()) {
            g.setColour(LcarsColors::moonlitViolet);
        } else {
            g.setColour(LcarsColors::africanViolet);
        }
        g.fillRect(activeArea);

        g.setColour(juce::Colours::black);
        g.setFont(getOrbitronFont(tabFontSize));
        g.drawText(button.getButtonText(), activeArea, juce::Justification::centred);
    }

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override {
        auto font = getOrbitronFont(tabFontSize);
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, button.getButtonText().trim(), 0, 0);
        int width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + tabDepth;

        if (auto* extraComponent = button.getExtraComponent()) {
            width += button.getTabbedButtonBar().isVertical() ? extraComponent->getHeight()
                                                              : extraComponent->getWidth();
        }

        return juce::jmax(tabDepth * 2, width);
    }

    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar&, juce::Graphics& g, int w,
                                      int h) override {
        g.setColour(LcarsColors::africanViolet);
        juce::Path line;
        line.startNewSubPath(1.0f, static_cast<float>(h) - 1.0f);
        line.lineTo(static_cast<float>(w) - 1.0f, static_cast<float>(h) - 1.0f);
        g.strokePath(line, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();

        // Capsule radius is half the height for full pill shape
        const float radius = bounds.getHeight() * 0.5f;

        // Determine background color based on state
        juce::Colour bgColour;
        if (shouldDrawButtonAsDown) {
            bgColour = LcarsColors::africanViolet;
        } else if (shouldDrawButtonAsHighlighted) {
            bgColour = LcarsColors::moonlitViolet;
        } else {
            bgColour = button.findColour(button.getToggleState() ? juce::TextButton::buttonOnColourId
                                                                 : juce::TextButton::buttonColourId);
        }

        juce::Colour borderColour = LcarsColors::orange;

        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.reduced(buttonBorderThickness * 0.5f), radius);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(buttonBorderThickness * 0.5f), radius,
                               buttonBorderThickness);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override {
        auto font = getTextButtonFont(button, button.getHeight());

        const int textPadding = static_cast<int>(button.getHeight() * 0.3f);
        auto textBounds = button.getLocalBounds().reduced(textPadding, 0);

        // Shrink font if text doesn't fit
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, button.getButtonText(), 0, 0);
        float textWidth = glyphs.getBoundingBox(0, -1, false).getWidth();
        if (textWidth > textBounds.getWidth()) {
            float scale = static_cast<float>(textBounds.getWidth()) / textWidth;
            font = font.withHeight(font.getHeight() * scale);
        }

        g.setFont(font);

        juce::Colour textColour = button.findColour(button.getToggleState()
                                                        ? juce::TextButton::textColourOnId
                                                        : juce::TextButton::textColourOffId);
        g.setColour(textColour);

        g.drawText(button.getButtonText(), textBounds, juce::Justification::centred, false);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool /*shouldDrawButtonAsHighlighted*/,
                          bool /*shouldDrawButtonAsDown*/) override {
        auto bounds = button.getLocalBounds().toFloat();
        auto tickBounds = bounds.removeFromLeft(bounds.getHeight()).reduced(4);

        // Draw checkbox background
        g.setColour(button.findColour(juce::ToggleButton::tickDisabledColourId));
        g.fillRoundedRectangle(tickBounds, 3.0f);

        // Draw checkbox border
        g.setColour(button.findColour(juce::ToggleButton::tickColourId));
        g.drawRoundedRectangle(tickBounds, 3.0f, 2.0f);

        // Draw checkmark if toggled
        if (button.getToggleState()) {
            g.setColour(button.findColour(juce::ToggleButton::tickColourId));
            auto checkBounds = tickBounds.reduced(4);
            g.fillRoundedRectangle(checkBounds, 2.0f);
        }

        // Draw text
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(getOrbitronFont(bounds.getHeight() * 0.6f));
        g.drawText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centredLeft);
    }
};
