#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// A container that displays one child at a time, controlled by a ComboBox.
// Child components are added via addVariant() and shown/hidden based on selection.
class VariantSelector : public juce::Component, private juce::Value::Listener {
   public:
    VariantSelector();
    ~VariantSelector() override;

    // Add a variant with a caption. The component will be owned by this selector.
    void addVariant(const juce::String& caption, juce::Component* component);

    // Add a variant with a caption. Caller retains ownership.
    void addVariantNotOwned(const juce::String& caption, juce::Component* component);

    int getSelectedIndex() const;
    void setSelectedIndex(int index, juce::NotificationType notification = juce::sendNotification);

    // Bind to a ValueTree property for persistence
    void bindToValue(juce::Value& value);

    // Callback when selection changes
    std::function<void(int)> onSelectionChanged;

    void resized() override;

   private:
    juce::ComboBox comboBox;
    juce::OwnedArray<juce::Component> ownedVariants;
    std::vector<juce::Component*> variants;  // All variants (owned or not)
    juce::Value boundValue;

    void updateVisibility();
    void valueChanged(juce::Value& value) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariantSelector)
};
