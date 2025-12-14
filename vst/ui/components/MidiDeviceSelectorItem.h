#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>
#include <juce_audio_devices/juce_audio_devices.h>

/**
 * A ComboBox that displays available MIDI input devices.
 * Stores the selected device name in the MagicGUIState property "midi_device_name".
 */
class MidiDeviceSelectorComponent : public juce::Component, private juce::Timer {
   public:
    MidiDeviceSelectorComponent();
    ~MidiDeviceSelectorComponent() override;

    void resized() override;

    /** Bind to a Value that stores the selected device name. */
    void bindToValue(juce::Value& value);

    /** Refresh the list of available MIDI devices. */
    void refreshDeviceList();

    /** Set caption text. */
    void setCaption(const juce::String& text);

   private:
    void timerCallback() override;
    void updateComboBoxFromValue();

    void enableMidiDeviceInStandalone(const juce::String& deviceName);

    juce::ComboBox comboBox;
    juce::Label captionLabel;
    juce::Value boundValue;
    juce::StringArray deviceNames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiDeviceSelectorComponent)
};

class MidiDeviceSelectorItem : public foleys::GuiItem {
   public:
    FOLEYS_DECLARE_GUI_FACTORY(MidiDeviceSelectorItem)

    MidiDeviceSelectorItem(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node);

    void update() override;
    juce::Component* getWrappedComponent() override;
    std::vector<foleys::SettableProperty> getSettableProperties() const override;

   private:
    MidiDeviceSelectorComponent selectorComponent;
    juce::Value midiDeviceNameValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiDeviceSelectorItem)
};