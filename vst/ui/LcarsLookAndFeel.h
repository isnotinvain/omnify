#pragma once

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LcarsColors.h"

class LcarsLookAndFeel : public juce::LookAndFeel_V4 {
   public:
    // Standard font sizes - use these for consistency across the UI
    static constexpr float fontSizeSmall = 14.0F;
    static constexpr float fontSizeMedium = 18.0F;
    static constexpr float fontSizeLarge = 24.0F;

    LcarsLookAndFeel() {
        orbitronTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::OrbitronRegular_ttf, BinaryData::OrbitronRegular_ttfSize);

        // Tab colors
        setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::black);
        setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::black);

        // ComboBox colors
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
        setColour(juce::ComboBox::outlineColourId, LcarsColors::orange);
        setColour(juce::ComboBox::arrowColourId, LcarsColors::orange);
        setColour(juce::ComboBox::textColourId, LcarsColors::orange);

        // PopupMenu colors (for ComboBox dropdowns)
        setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::textColourId, LcarsColors::orange);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, LcarsColors::africanViolet);
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    }

    // Public font getter for components that need to set fonts directly
    juce::Font getOrbitronFont(float height) const { return juce::Font(juce::FontOptions(orbitronTypeface).withHeight(height)); }

   private:
    juce::Typeface::Ptr orbitronTypeface;

    // Drawing constants
    static constexpr float comboboxBorderRadius = 4.0F;
    static constexpr float comboboxArrowSize = 6.0F;
    static constexpr float comboboxArrowPadding = 8.0F;
    static constexpr float popupMenuBorderSize = 2.0F;
    static constexpr float buttonBorderThickness = 2.0F;

    juce::Font getLabelFont(juce::Label& label) override { return getOrbitronFont(label.getFont().getHeight()); }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return getOrbitronFont(fontSizeSmall); }

    juce::Font getPopupMenuFont() override { return getOrbitronFont(fontSizeSmall); }

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth,
                                              int& idealHeight, const juce::PopupMenu::Options&) override {
        if (isSeparator) {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        } else {
            idealHeight = static_cast<int>(fontSizeMedium);
            auto font = getOrbitronFont(fontSizeSmall);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0, 0);
            idealWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + idealHeight * 2;
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height);

        // Draw background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), comboboxBorderRadius);

        // Draw outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.0F), comboboxBorderRadius, 2.0F);

        // Draw a single filled downward-pointing triangle
        const float arrowX = static_cast<float>(width) - comboboxArrowSize - comboboxArrowPadding;
        const float arrowY = static_cast<float>(height) * 0.5F;

        juce::Path path;
        path.addTriangle(arrowX - comboboxArrowSize, arrowY - comboboxArrowSize * 0.5F,  // top left
                         arrowX + comboboxArrowSize,
                         arrowY - comboboxArrowSize * 0.5F,           // top right
                         arrowX, arrowY + comboboxArrowSize * 0.5F);  // bottom center

        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9F : 0.2F));
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
        g.setFont(getOrbitronFont(fontSizeMedium));
        g.drawText(button.getButtonText(), activeArea, juce::Justification::centred);
    }

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override {
        auto font = getOrbitronFont(fontSizeMedium);
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, button.getButtonText().trim(), 0, 0);
        int width = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + tabDepth;

        if (auto* extraComponent = button.getExtraComponent()) {
            width += button.getTabbedButtonBar().isVertical() ? extraComponent->getHeight() : extraComponent->getWidth();
        }

        return juce::jmax(tabDepth * 2, width);
    }

    void drawTabAreaBehindFrontButton(juce::TabbedButtonBar&, juce::Graphics& g, int w, int h) override {
        g.setColour(LcarsColors::africanViolet);
        juce::Path line;
        line.startNewSubPath(1.0F, static_cast<float>(h) - 1.0F);
        line.lineTo(static_cast<float>(w) - 1.0F, static_cast<float>(h) - 1.0F);
        g.strokePath(line, juce::PathStrokeType(2.0F, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();

        // Capsule radius is half the height for full pill shape
        const float radius = bounds.getHeight() * 0.5F;

        // Determine background color based on state
        juce::Colour bgColour;
        if (shouldDrawButtonAsDown) {
            bgColour = LcarsColors::africanViolet;
        } else if (shouldDrawButtonAsHighlighted) {
            bgColour = LcarsColors::moonlitViolet;
        } else {
            bgColour = button.findColour(button.getToggleState() ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
        }

        juce::Colour borderColour = LcarsColors::orange;

        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds.reduced(buttonBorderThickness * 0.5F), radius);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.reduced(buttonBorderThickness * 0.5F), radius, buttonBorderThickness);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override {
        g.setFont(getOrbitronFont(fontSizeSmall));

        juce::Colour textColour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);
        g.setColour(textColour);

        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool /*shouldDrawButtonAsHighlighted*/,
                          bool /*shouldDrawButtonAsDown*/) override {
        auto bounds = button.getLocalBounds().toFloat();
        auto tickBounds = bounds.removeFromLeft(bounds.getHeight()).reduced(4);

        // Draw checkbox background
        g.setColour(button.findColour(juce::ToggleButton::tickDisabledColourId));
        g.fillRoundedRectangle(tickBounds, 3.0F);

        // Draw checkbox border
        g.setColour(button.findColour(juce::ToggleButton::tickColourId));
        g.drawRoundedRectangle(tickBounds, 3.0F, 2.0F);

        // Draw checkmark if toggled
        if (button.getToggleState()) {
            g.setColour(button.findColour(juce::ToggleButton::tickColourId));
            auto checkBounds = tickBounds.reduced(4);
            g.fillRoundedRectangle(checkBounds, 2.0F);
        }

        // Draw text
        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(getOrbitronFont(fontSizeSmall));
        g.drawText(button.getButtonText(), bounds.toNearestInt(), juce::Justification::centredLeft);
    }
};
