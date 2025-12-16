#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/**
 * A ComboBox that displays available MIDI input devices.
 */
class MidiDeviceSelectorComponent : public juce::Component, private juce::Timer {
   public:
    MidiDeviceSelectorComponent();
    ~MidiDeviceSelectorComponent() override;

    void resized() override;

    /** Callback when a device is selected. */
    std::function<void(const juce::String& deviceName)> onDeviceSelected;

    /** Set the displayed device name. */
    void setSelectedDevice(const juce::String& deviceName);

    /** Refresh the list of available MIDI devices. */
    void refreshDeviceList();

    /** Set caption text. */
    void setCaption(const juce::String& text);

   private:
    void timerCallback() override;

    static void enableMidiDeviceInStandalone(const juce::String& deviceName);

    juce::ComboBox comboBox;
    juce::Label captionLabel;
    juce::StringArray deviceNames;
    juce::String currentDeviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiDeviceSelectorComponent)
};
