#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include <functional>

// A reusable container that shows a ComboBox and displays one child at a time.
// Child Views are declared in XML with captions that populate the ComboBox.
// Usage in XML:
//   <VariantSelector>
//     <View caption="Option A">...</View>
//     <View caption="Option B">...</View>
//   </VariantSelector>
class VariantSelectorItem : public foleys::Container {
   public:
    DECLARE_CONTAINER_FACTORY(VariantSelectorItem)

    static const juce::Identifier pComboBoxHeight;

    VariantSelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void createSubComponents() override;
    void update() override;
    void resized() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

    int getSelectedIndex() const;
    void setSelectedIndex(int index);

    // Called when user changes selection: (newIndex, visibleChildComponent)
    std::function<void(int, juce::Component*)> onSelectionChanged;

   private:
    juce::ComboBox comboBox;
    int comboBoxHeight = 30;

    void updateChildVisibility();
    foleys::GuiItem* getChildAtIndex(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariantSelectorItem)
};