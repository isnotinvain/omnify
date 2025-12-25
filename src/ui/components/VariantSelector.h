#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// A container that displays one child at a time, controlled by a ComboBox.
// Child components are added via addVariant() and shown/hidden based on selection.
class VariantSelector : public juce::Component, private juce::Value::Listener {
   public:
    VariantSelector();
    ~VariantSelector() override;

    // Add a variant with a caption and optional description. The component will be owned by this selector.
    void addVariant(const juce::String& caption, juce::Component* component, const juce::String& description = {});

    // Add a variant with a caption and optional description. Caller retains ownership.
    void addVariantNotOwned(const juce::String& caption, juce::Component* component, const juce::String& description = {});

    int getSelectedIndex() const;
    void setSelectedIndex(int index, juce::NotificationType notification = juce::sendNotification);

    // Bind to a ValueTree property for persistence
    void bindToValue(juce::Value& value);

    // Callback when selection changes
    std::function<void(int)> onSelectionChanged;

    void resized() override;
    void paint(juce::Graphics& g) override;

   private:
    juce::ComboBox comboBox;
    juce::Rectangle<int> descriptionBounds;
    juce::String currentDescription;
    juce::OwnedArray<juce::Component> ownedVariants;
    std::vector<juce::Component*> variants;  // All variants (owned or not)
    std::vector<juce::String> descriptions;
    juce::Value boundValue;

    void updateVisibility();
    void valueChanged(juce::Value& value) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariantSelector)
};
