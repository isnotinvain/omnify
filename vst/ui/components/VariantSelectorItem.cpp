#include "../GuiItems.h"

VariantSelectorItem::VariantSelectorItem(foleys::MagicGUIBuilder& builder,
                                         const juce::ValueTree& node)
    : foleys::Container(builder, node) {
    // createSubComponents() is called by our DECLARE_CONTAINER_FACTORY
}

void VariantSelectorItem::createSubComponents() {
    foleys::Container::createSubComponents();

    // Find the first direct child that is a ComboBox
    comboBoxItem = nullptr;
    for (int i = 0; i < configNode.getNumChildren(); ++i) {
        auto childNode = configNode.getChild(i);
        if (childNode.getType() == foleys::IDs::comboBox) {
            comboBoxItem = findGuiItem(childNode);
            break;
        }
    }

    auto* comboBox = getComboBox();
    if (comboBox == nullptr) {
        DBG("VariantSelectorItem: No ComboBox found among direct children!");
        return;
    }

    // Populate ComboBox from variant children (non-ComboBox children)
    comboBox->clear();
    int itemId = 1;
    for (auto it = begin(); it != end(); ++it) {
        if (it->get() == comboBoxItem) continue;
        auto caption = (*it)->getTabCaption("Option " + juce::String(itemId));
        comboBox->addItem(caption, itemId++);
    }

    if (comboBox->getNumItems() > 0) {
        comboBox->setSelectedItemIndex(0, juce::dontSendNotification);
    }

    // Capture initial max sizes from XML before we modify anything
    for (auto it = begin(); it != end(); ++it) {
        auto* item = it->get();
        if (item == comboBoxItem) continue;
        auto& flexItem = item->getFlexItem();
        originalMaxSizes[item] = {flexItem.maxWidth, flexItem.maxHeight};
    }

    comboBox->onChange = [this]() {
        updateChildVisibility();
        if (onSelectionChanged) {
            int index = getComboBox()->getSelectedItemIndex();
            juce::Component* visibleChild = nullptr;
            if (auto* item = getVariantChildAtIndex(index)) {
                visibleChild = item->getWrappedComponent();
            }
            onSelectionChanged(index, visibleChild);
        }
    };

    updateChildVisibility();
}

void VariantSelectorItem::update() {
    foleys::Container::update();
    updateChildVisibility();
}

juce::ComboBox* VariantSelectorItem::getComboBox() const {
    if (comboBoxItem == nullptr) return nullptr;
    return dynamic_cast<juce::ComboBox*>(comboBoxItem->getWrappedComponent());
}

int VariantSelectorItem::getSelectedIndex() const {
    auto* comboBox = getComboBox();
    return comboBox ? comboBox->getSelectedItemIndex() : -1;
}

void VariantSelectorItem::setSelectedIndex(int index) {
    if (auto* comboBox = getComboBox()) {
        comboBox->setSelectedItemIndex(index, juce::sendNotificationSync);
    }
}

void VariantSelectorItem::updateChildVisibility() {
    auto* comboBox = getComboBox();
    if (comboBox == nullptr) return;

    int selectedIndex = comboBox->getSelectedItemIndex();

    // Find and hide the currently visible item, saving its values
    for (auto it = begin(); it != end(); ++it) {
        auto* item = it->get();
        if (item == comboBoxItem) continue;

        if (item->isVisible()) {
            auto& flexItem = item->getFlexItem();
            originalMaxSizes[item] = {flexItem.maxWidth, flexItem.maxHeight};
            flexItem.maxWidth = 0;
            flexItem.maxHeight = 0;
            item->setVisible(false);
        }
    }

    // Show the selected item
    if (auto* selectedItem = getVariantChildAtIndex(selectedIndex)) {
        auto savedIt = originalMaxSizes.find(selectedItem);
        if (savedIt != originalMaxSizes.end()) {
            auto& flexItem = selectedItem->getFlexItem();
            flexItem.maxWidth = savedIt->second.first;
            flexItem.maxHeight = savedIt->second.second;
        }
        selectedItem->setVisible(true);
    }

    updateLayout();
}

foleys::GuiItem* VariantSelectorItem::getVariantChildAtIndex(int index) {
    int variantIndex = 0;
    for (auto it = begin(); it != end(); ++it) {
        if (it->get() == comboBoxItem) continue;
        if (variantIndex == index) {
            return it->get();
        }
        ++variantIndex;
    }
    return nullptr;
}