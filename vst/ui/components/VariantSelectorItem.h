#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include <functional>
#include <map>

// A container that displays one child at a time, controlled by a ComboBox.
// The first child must be a ComboBox, subsequent children are the variants.
// The ComboBox is auto-populated from the captions of the variant children.
// Usage in XML:
//   <VariantSelector>
//     <ComboBox/>
//     <View caption="Option A">...</View>
//     <View caption="Option B">...</View>
//   </VariantSelector>
class VariantSelectorItem : public foleys::Container {
   public:
    DECLARE_CONTAINER_FACTORY(VariantSelectorItem)

    VariantSelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void createSubComponents() override;
    void update() override;

    int getSelectedIndex() const;
    void setSelectedIndex(int index);

    // Called when user changes selection: (newIndex, visibleChildComponent)
    std::function<void(int, juce::Component*)> onSelectionChanged;

   private:
    foleys::GuiItem* comboBoxItem = nullptr;
    std::map<foleys::GuiItem*, std::pair<float, float>> originalMaxSizes;  // maxWidth, maxHeight

    juce::ComboBox* getComboBox() const;
    void updateChildVisibility();
    foleys::GuiItem* getVariantChildAtIndex(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariantSelectorItem)
};