#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

// A container that wraps a TextButton for file picking.
// The first child must be a TextButton which will trigger the file dialog.
// Usage in XML:
//   <FilePicker id="my_file" file-filter="*.json">
//     <TextButton text="Browse..."/>
//   </FilePicker>
class FilePickerItem : public foleys::Container {
   public:
    DECLARE_CONTAINER_FACTORY(FilePickerItem)

    static const juce::Identifier pFileFilter;

    FilePickerItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);
    ~FilePickerItem() override = default;

    void createSubComponents() override;
    void update() override;

   private:
    void openFileChooser();
    void updateButtonText();

    juce::TextButton* getButton() const;

    foleys::GuiItem* buttonItem = nullptr;
    juce::String selectedPath;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilePickerItem)
};