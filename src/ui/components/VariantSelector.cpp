#include "VariantSelector.h"

#include <utility>

#include "../LcarsLookAndFeel.h"

VariantSelector::VariantSelector() {
    addAndMakeVisible(comboBox);

    comboBox.onChange = [this]() {
        int index = comboBox.getSelectedItemIndex();

        // Update bound value if we have one
        if (!boundValue.getValue().isVoid()) {
            boundValue.setValue(index);
        }

        updateVisibility();

        if (onSelectionChanged) {
            onSelectionChanged(index);
        }
    };
}

VariantSelector::~VariantSelector() = default;

void VariantSelector::addVariant(const juce::String& caption, juce::Component* component, const juce::String& description) {
    ownedVariants.add(component);
    addVariantNotOwned(caption, component, description);
}

void VariantSelector::addVariantNotOwned(const juce::String& caption, juce::Component* component, const juce::String& description) {
    int itemId = static_cast<int>(variants.size()) + 1;
    comboBox.addItem(caption, itemId);
    variants.push_back(component);
    descriptions.push_back(description);
    addChildComponent(component);  // Initially hidden

    // If this is the first variant, select it
    if (variants.size() == 1) {
        comboBox.setSelectedItemIndex(0, juce::dontSendNotification);
        updateVisibility();
    }
}

int VariantSelector::getSelectedIndex() const { return comboBox.getSelectedItemIndex(); }

void VariantSelector::setSelectedIndex(int index, juce::NotificationType notification) {
    comboBox.setSelectedItemIndex(index, notification);
    if (notification == juce::dontSendNotification) {
        updateVisibility();
    }
}

void VariantSelector::bindToValue(juce::Value& value) {
    boundValue.referTo(value);
    boundValue.addListener(this);

    // Set initial selection from value
    int savedIndex = static_cast<int>(value.getValue());
    if (savedIndex >= 0 && std::cmp_less(savedIndex, variants.size())) {
        comboBox.setSelectedItemIndex(savedIndex, juce::dontSendNotification);
        updateVisibility();
    }
}

void VariantSelector::valueChanged(juce::Value& value) {
    if (&value == &boundValue) {
        int index = static_cast<int>(value.getValue());
        if (index >= 0 && std::cmp_less(index, variants.size())) {
            comboBox.setSelectedItemIndex(index, juce::dontSendNotification);
            updateVisibility();
        }
    }
}

void VariantSelector::resized() {
    auto bounds = getLocalBounds();

    // ComboBox at top
    comboBox.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(8);  // Padding between combo box and description

    // Calculate description height
    int descHeight = 0;
    if (currentDescription.isNotEmpty()) {
        if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
            auto font = laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny);

            juce::AttributedString attrStr;
            attrStr.append(currentDescription, font, LcarsColors::red);
            attrStr.setWordWrap(juce::AttributedString::WordWrap::byWord);

            juce::TextLayout layout;
            layout.createLayout(attrStr, static_cast<float>(bounds.getWidth()));
            descHeight = static_cast<int>(std::ceil(layout.getHeight())) + 4;
        } else {
            descHeight = 40;
        }
    }
    descriptionBounds = bounds.removeFromTop(descHeight);

    // Variant component - use preferred height if set, otherwise use all remaining space
    auto selectedIndex = static_cast<size_t>(comboBox.getSelectedItemIndex());
    juce::Rectangle<int> variantBounds = bounds;
    if (selectedIndex < variants.size()) {
        auto* activeVariant = variants[selectedIndex];
        if (activeVariant->getProperties().contains("preferredHeight")) {
            int variantHeight = static_cast<int>(activeVariant->getProperties()["preferredHeight"]);
            bool topAlign = static_cast<bool>(activeVariant->getProperties().getWithDefault("topAlign", false));
            variantBounds = topAlign ? bounds.removeFromTop(variantHeight) : bounds.removeFromBottom(variantHeight);
        }
    }
    for (auto* variant : variants) {
        variant->setBounds(variantBounds);
    }
}

void VariantSelector::paint(juce::Graphics& g) {
    if (currentDescription.isEmpty()) return;

    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        auto font = laf->getOrbitronFont(LcarsLookAndFeel::fontSizeTiny);

        juce::AttributedString attrStr;
        attrStr.append(currentDescription, font, LcarsColors::red);
        attrStr.setWordWrap(juce::AttributedString::WordWrap::byWord);

        juce::TextLayout layout;
        layout.createLayout(attrStr, static_cast<float>(descriptionBounds.getWidth()));
        layout.draw(g, descriptionBounds.toFloat());
    }
}

void VariantSelector::updateVisibility() {
    auto selectedIndex = static_cast<size_t>(comboBox.getSelectedItemIndex());

    for (size_t i = 0; i < variants.size(); ++i) {
        variants[i]->setVisible(i == selectedIndex);
    }

    if (selectedIndex < descriptions.size()) {
        currentDescription = descriptions[selectedIndex];
        resized();
        repaint();
    }
}
