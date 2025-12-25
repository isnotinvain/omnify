#pragma once

#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "LcarsColors.h"

class LcarsLookAndFeel : public juce::LookAndFeel_V4 {
   public:
    // Standard font sizes - use these for consistency across the UI
    static constexpr float fontSizeTiny = 16.0F;
    static constexpr float fontSizeSmall = 22.0F;
    static constexpr float fontSizeMedium = 26.0F;
    static constexpr float fontSizeLarge = 34.0F;
    static constexpr float fontSizeTitle = 56.0F;

    // Standard border radius for boxes/panels
    static constexpr float borderRadius = 4.0F;

    // Standard combo box row height
    static constexpr int comboBoxRowHeight = 50;

    // Property ID for custom combo box font size
    static inline const juce::Identifier comboBoxFontSizeId{"LcarsFontSize"};

    static void setComboBoxFontSize(juce::ComboBox& box, float fontSize) { box.getProperties().set(comboBoxFontSizeId, fontSize); }

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
    static constexpr float comboboxArrowSize = 6.0F;
    static constexpr float comboboxArrowPadding = 8.0F;
    static constexpr float popupMenuBorderSize = 1.0F;
    static constexpr float buttonBorderThickness = 1.0F;

    juce::Font getLabelFont(juce::Label& label) override { return getOrbitronFont(label.getFont().getHeight()); }

    juce::Font getComboBoxFont(juce::ComboBox& box) override {
        auto fontSize = static_cast<float>(box.getProperties().getWithDefault(comboBoxFontSizeId, fontSizeSmall));
        return getOrbitronFont(fontSize);
    }

    juce::Font getPopupMenuFont() override { return getOrbitronFont(fontSizeSmall); }

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth,
                                              int& idealHeight, const juce::PopupMenu::Options& options) override {
        if (isSeparator) {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        } else {
            idealHeight = comboBoxRowHeight;
            float fontSize = fontSizeSmall;

            if (auto* targetComp = options.getTargetComponent()) {
                idealHeight = targetComp->getHeight();
                fontSize = static_cast<float>(targetComp->getProperties().getWithDefault(comboBoxFontSizeId, fontSizeSmall));
            }

            auto font = getOrbitronFont(fontSize);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0, 0);
            idealWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + idealHeight * 2;
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height);

        // Draw background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), borderRadius);

        // Draw outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.0F), borderRadius, 1.0F);

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

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
        auto bounds = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));

        g.fillAll(juce::Colours::black);

        g.setColour(findColour(juce::PopupMenu::backgroundColourId));
        g.fillRoundedRectangle(bounds, borderRadius);

        g.setColour(LcarsColors::orange);
        g.drawRoundedRectangle(bounds.reduced(0.5F), borderRadius, 1.0F);
    }

    void drawPopupMenuItemWithOptions(juce::Graphics& g, const juce::Rectangle<int>& area, bool isHighlighted, const juce::PopupMenu::Item& item,
                                      const juce::PopupMenu::Options& options) override {
        if (item.isSeparator) {
            auto r = area.reduced(5, 0).toFloat();
            r.removeFromTop(juce::roundToInt((r.getHeight() * 0.5F) - 0.5F));
            g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.3F));
            g.fillRect(r.removeFromTop(1.0F));
            return;
        }

        auto textColour = findColour(isHighlighted ? juce::PopupMenu::highlightedTextColourId : juce::PopupMenu::textColourId);

        if (isHighlighted && item.isEnabled) {
            g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
            g.fillRoundedRectangle(area.toFloat(), borderRadius);
        }

        float fontSize = fontSizeSmall;
        if (auto* targetComp = options.getTargetComponent()) {
            fontSize = static_cast<float>(targetComp->getProperties().getWithDefault(comboBoxFontSizeId, fontSizeSmall));
        }

        g.setColour(textColour);
        g.setFont(getOrbitronFont(fontSize));

        auto textArea = area.reduced(12, 0);
        g.drawText(item.text, textArea, juce::Justification::centredLeft, true);
    }

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
        g.strokePath(line, juce::PathStrokeType(1.0F, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override {
        if (style != juce::Slider::LinearBar && style != juce::Slider::LinearBarVertical) {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
            return;
        }

        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
        const float radius = bounds.getHeight() * 0.5F;
        const float borderThickness = 1.0F;

        // Background (red fill for empty space)
        g.setColour(LcarsColors::orange);
        g.fillRoundedRectangle(bounds, radius);

        // Fill representing value - use clipping to sweep a rectangular mask across the capsule
        float fillWidth = sliderPos - static_cast<float>(x);
        if (fillWidth > 0) {
            juce::Graphics::ScopedSaveState saveState(g);
            g.reduceClipRegion(static_cast<int>(bounds.getX()), static_cast<int>(bounds.getY()), static_cast<int>(fillWidth),
                               static_cast<int>(bounds.getHeight()));
            g.setColour(LcarsColors::red);
            g.fillRoundedRectangle(bounds.reduced(borderThickness), radius);
        }

        // Border (full extent)
        g.setColour(LcarsColors::orange);
        g.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius, borderThickness);

        // Value text centered inside
        g.setColour(juce::Colours::black);
        g.setFont(getOrbitronFont(fontSizeSmall));
        g.drawText(juce::String(juce::roundToInt(slider.getValue())), bounds.toNearestInt(), juce::Justification::centred, false);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool /*shouldDrawButtonAsHighlighted*/,
                          bool /*shouldDrawButtonAsDown*/) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0F);
        const float borderThickness = 1.0F;
        const float radius = bounds.getHeight() * 0.5F;

        // Background
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius);

        // Border
        g.setColour(button.findColour(juce::ToggleButton::tickColourId));
        g.drawRoundedRectangle(bounds, radius, borderThickness);

        // Text - show on/off text based on state (customizable via properties)
        auto onText = button.getProperties().getWithDefault("onText", "On").toString();
        auto offText = button.getProperties().getWithDefault("offText", "Off").toString();
        g.setColour(button.findColour(juce::ToggleButton::tickColourId));
        g.setFont(getOrbitronFont(fontSizeSmall));
        g.drawText(button.getToggleState() ? onText : offText, bounds.toNearestInt(), juce::Justification::centred);
    }
};
