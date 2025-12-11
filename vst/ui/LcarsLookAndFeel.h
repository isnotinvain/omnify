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
    }

    void setBuilder(foleys::MagicGUIBuilder* b) {
        builder = b;
        refreshSettingsCache();
    }

    void refreshSettingsCache() {
        if (builder) {
            auto rootView = builder->getConfigTree().getChildWithName("View");
            settingsCache = rootView.getChildWithName("LcarsSettings");
        }
    }

    float getSetting(const juce::String& name, float defaultVal) const {
        if (settingsCache.isValid() && settingsCache.hasProperty(name)) {
            return static_cast<float>(settingsCache.getProperty(name));
        }
        return defaultVal;
    }

    juce::Colour getSettingColour(const juce::String& name, juce::Colour defaultVal) const {
        if (settingsCache.isValid() && settingsCache.hasProperty(name)) {
            auto colourStr = settingsCache.getProperty(name).toString();
            // Support palette references like "$lcars-orange"
            if (colourStr.startsWith("$") && builder) {
                return builder->getStylesheet().getColour(colourStr);
            }
            // Otherwise parse as hex color
            return juce::Colour::fromString(colourStr);
        }
        return defaultVal;
    }

   private:
    foleys::MagicGUIBuilder* builder = nullptr;
    juce::ValueTree settingsCache;
    juce::Typeface::Ptr orbitronTypeface;

    juce::Font getOrbitronFont(float height) const {
        return juce::Font(juce::FontOptions(orbitronTypeface).withHeight(height));
    }

    juce::Font getLabelFont(juce::Label& label) override {
        if (auto* comboBox = dynamic_cast<juce::ComboBox*>(label.getParentComponent())) {
            if (auto* guiItem = dynamic_cast<foleys::GuiItem*>(comboBox->getParentComponent())) {
                auto captionSize =
                    static_cast<float>(guiItem->getProperty(foleys::IDs::captionSize));
                if (captionSize > 0) {
                    return getOrbitronFont(captionSize *
                                           getSetting("combobox-label-font-multiplier", 0.8F));
                }
            }
            return getOrbitronFont(comboBox->getHeight() *
                                   getSetting("combobox-font-multiplier", 0.85F));
        }
        return getOrbitronFont(label.getFont().getHeight());
    }

    juce::Font getComboBoxFont(juce::ComboBox& box) override {
        return getOrbitronFont(
            juce::jmin(getSetting("popup-menu-font-size", 15.0F),
                       (float)box.getHeight() * getSetting("combobox-font-multiplier", 0.85F)));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override {
        return getOrbitronFont(
            juce::jmin(getSetting("popup-menu-font-size", 15.0F),
                       buttonHeight * getSetting("text-button-font-multiplier", 0.6F)));
    }

    juce::Font getPopupMenuFont() override {
        return getOrbitronFont(getSetting("popup-menu-font-size", 15.0F));
    }

    void getIdealPopupMenuItemSizeWithOptions(const juce::String& text, bool isSeparator,
                                              int standardMenuItemHeight, int& idealWidth,
                                              int& idealHeight,
                                              const juce::PopupMenu::Options& options) override {
        if (isSeparator) {
            idealWidth = 50;
            idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 10 : 10;
        } else {
            float fontSize = getSetting("popup-menu-font-size", 15.0F);
            if (auto* target = options.getTargetComponent()) {
                if (auto* guiItem = dynamic_cast<foleys::GuiItem*>(target->getParentComponent())) {
                    auto captionSize =
                        static_cast<float>(guiItem->getProperty(foleys::IDs::captionSize));
                    if (captionSize > 0) {
                        fontSize =
                            captionSize * getSetting("popup-menu-item-font-multiplier", 0.8F);
                    }
                }
            }
            idealHeight =
                static_cast<int>(fontSize * getSetting("popup-menu-item-height-multiplier", 1.3F));
            auto font = getOrbitronFont(fontSize);
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, text, 0, 0);
            idealWidth = static_cast<int>(glyphs.getBoundingBox(0, -1, false).getWidth()) + idealHeight * 2;
        }
    }

    juce::Font getMenuBarFont(juce::MenuBarComponent&, int, const juce::String&) override {
        return getOrbitronFont(getSetting("menu-bar-font-size", 15.0F));
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
        return getOrbitronFont(height * getSetting("tab-button-font-multiplier", 0.6F));
    }

    juce::Font getSidePanelTitleFont(juce::SidePanel&) override {
        return juce::Font(juce::FontOptions(orbitronTypeface));
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int,
                      juce::ComboBox& box) override {
        auto bounds = juce::Rectangle<int>(0, 0, width, height);
        const float borderRadius = getSetting("combobox-border-radius", 4.0F);

        // Draw background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds.toFloat(), borderRadius);

        // Draw outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.0F), borderRadius, 2.0F);

        // Draw a single filled downward-pointing triangle
        const float arrowSize = getSetting("combobox-arrow-size", 6.0F);
        const float arrowPadding = getSetting("combobox-arrow-padding", 8.0F);
        const float arrowX = (float)width - arrowSize - arrowPadding;
        const float arrowY = (float)height * 0.5F;

        juce::Path path;
        path.addTriangle(arrowX - arrowSize, arrowY - arrowSize * 0.5F,  // top left
                         arrowX + arrowSize, arrowY - arrowSize * 0.5F,  // top right
                         arrowX, arrowY + arrowSize * 0.5F);             // bottom center

        g.setColour(
            box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9F : 0.2F));
        g.fillPath(path);
    }

    int getPopupMenuBorderSize() override {
        return static_cast<int>(getSetting("popup-menu-border-size", 2.0F));
    }

    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g, bool, bool) override {
        const auto activeArea = button.getActiveArea();

        if (button.getToggleState()) {
            g.setColour(getSettingColour("tab-active-color", LcarsColors::orange));
        } else {
            g.setColour(getSettingColour("tab-inactive-color", LcarsColors::red));
        }
        g.fillRect(activeArea);

        g.setColour(getSettingColour("tab-text-color", juce::Colours::black));
        g.setFont(getOrbitronFont(getSetting("tab-font-size", 14.0F)));
        g.drawText(button.getButtonText(), activeArea, juce::Justification::centred);
    }

    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override {
        auto font = getOrbitronFont(getSetting("tab-font-size", 14.0F));
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
        g.setColour(getSettingColour("tab-underline-color", LcarsColors::orange));
        juce::Path line;
        line.startNewSubPath(1.0F, h - 1.0F);
        line.lineTo(w - 1.0F, h - 1.0F);
        g.strokePath(line, juce::PathStrokeType(2.0F, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    }
};
