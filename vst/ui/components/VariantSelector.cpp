#include "VariantSelector.h"

VariantSelector::VariantSelector() {
    addAndMakeVisible(comboBox);

    comboBox.onChange = [this]() {
        int index = comboBox.getSelectedItemIndex();

        // Update bound value if we have one
        if (boundValue.getValue().isVoid() == false) {
            boundValue.setValue(index);
        }

        updateVisibility();

        if (onSelectionChanged) {
            onSelectionChanged(index);
        }
    };
}

VariantSelector::~VariantSelector() = default;

void VariantSelector::addVariant(const juce::String& caption, juce::Component* component) {
    ownedVariants.add(component);
    addVariantNotOwned(caption, component);
}

void VariantSelector::addVariantNotOwned(const juce::String& caption, juce::Component* component) {
    int itemId = static_cast<int>(variants.size()) + 1;
    comboBox.addItem(caption, itemId);
    variants.push_back(component);
    addChildComponent(component);  // Initially hidden

    // If this is the first variant, select it
    if (variants.size() == 1) {
        comboBox.setSelectedItemIndex(0, juce::dontSendNotification);
        updateVisibility();
    }
}

int VariantSelector::getSelectedIndex() const {
    return comboBox.getSelectedItemIndex();
}

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
    if (savedIndex >= 0 && savedIndex < static_cast<int>(variants.size())) {
        comboBox.setSelectedItemIndex(savedIndex, juce::dontSendNotification);
        updateVisibility();
    }
}

void VariantSelector::valueChanged(juce::Value& value) {
    if (&value == &boundValue) {
        int index = static_cast<int>(value.getValue());
        if (index >= 0 && index < static_cast<int>(variants.size())) {
            comboBox.setSelectedItemIndex(index, juce::dontSendNotification);
            updateVisibility();
        }
    }
}

void VariantSelector::resized() {
    auto bounds = getLocalBounds();

    // ComboBox at top
    comboBox.setBounds(bounds.removeFromTop(30));

    // Remaining space for the active variant
    for (auto* variant : variants) {
        variant->setBounds(bounds);
    }
}

void VariantSelector::updateVisibility() {
    int selectedIndex = comboBox.getSelectedItemIndex();

    for (int i = 0; i < static_cast<int>(variants.size()); ++i) {
        variants[i]->setVisible(i == selectedIndex);
    }
}
