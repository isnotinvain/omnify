#include "../GuiItems.h"

const juce::Identifier VariantSelectorItem::pComboBoxHeight{"combobox-height"};

VariantSelectorItem::VariantSelectorItem(foleys::MagicGUIBuilder& builder,
                                         const juce::ValueTree& node)
    : foleys::Container(builder, node) {
    addAndMakeVisible(comboBox);

    comboBox.onChange = [this]() {
        updateChildVisibility();
        if (onSelectionChanged) {
            int index = comboBox.getSelectedItemIndex();
            juce::Component* visibleChild = nullptr;
            if (auto* item = getChildAtIndex(index)) {
                visibleChild = item->getWrappedComponent();
            }
            onSelectionChanged(index, visibleChild);
        }
    };
    // createSubComponents() is called by our DECLARE_CONTAINER_FACTORY
}

void VariantSelectorItem::createSubComponents() {
    // Let parent build children from XML
    foleys::Container::createSubComponents();

    // Populate ComboBox from child captions
    comboBox.clear();
    int itemId = 1;
    for (auto it = begin(); it != end(); ++it) {
        auto caption = (*it)->getTabCaption("Option " + juce::String(itemId));
        comboBox.addItem(caption, itemId++);
    }

    if (comboBox.getNumItems() > 0) {
        comboBox.setSelectedItemIndex(0, juce::dontSendNotification);
    }

    updateChildVisibility();
}

void VariantSelectorItem::update() {
    auto heightVar = magicBuilder.getStyleProperty(pComboBoxHeight, configNode);
    if (!heightVar.isVoid()) {
        comboBoxHeight = static_cast<int>(heightVar);
    }

    foleys::Container::update();
}

void VariantSelectorItem::resized() {
    auto bounds = getLocalBounds();

    // ComboBox takes top portion
    comboBox.setBounds(bounds.removeFromTop(comboBoxHeight));

    // Let Container handle the rest for the visible child
    // We need to manually position children since we're overriding resized
    for (auto it = begin(); it != end(); ++it) {
        if ((*it)->isVisible()) {
            (*it)->setBounds(bounds);
        }
    }
}

std::vector<foleys::SettableProperty> VariantSelectorItem::getSettableProperties() const {
    auto props = foleys::Container::getSettableProperties();
    props.push_back({configNode, pComboBoxHeight, foleys::SettableProperty::Number, 30, {}});
    return props;
}

int VariantSelectorItem::getSelectedIndex() const { return comboBox.getSelectedItemIndex(); }

void VariantSelectorItem::setSelectedIndex(int index) {
    comboBox.setSelectedItemIndex(index, juce::sendNotificationSync);
}

void VariantSelectorItem::updateChildVisibility() {
    int selectedIndex = comboBox.getSelectedItemIndex();
    int childIndex = 0;
    for (auto it = begin(); it != end(); ++it) {
        (*it)->setVisible(childIndex == selectedIndex);
        ++childIndex;
    }
    resized();
}

foleys::GuiItem* VariantSelectorItem::getChildAtIndex(int index) {
    int childIndex = 0;
    for (auto it = begin(); it != end(); ++it) {
        if (childIndex == index) {
            return it->get();
        }
        ++childIndex;
    }
    return nullptr;
}