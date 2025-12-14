#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * A ComboBox that displays available MIDI input devices.
 * Stores the selected device name in a bound juce::Value.
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
