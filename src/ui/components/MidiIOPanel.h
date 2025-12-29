#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class MidiIOPanel : public juce::Component, private juce::Timer {
   public:
    MidiIOPanel();
    ~MidiIOPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(bool useDaw, const juce::String& deviceName)> onInputChanged;
    std::function<void(bool useDaw, const juce::String& portName)> onOutputChanged;

    void setInputDaw(bool useDaw);
    void setInputDevice(const juce::String& deviceName);
    void setOutputDaw(bool useDaw);
    void setOutputPortName(const juce::String& portName);

   private:
    void timerCallback() override;
    void refreshDeviceList();
    void notifyInputChanged();
    void notifyOutputChanged();

    // Input side
    juce::Label inputLabel{"", "Input"};
    juce::ToggleButton inputDawToggle;
    juce::ComboBox inputDeviceCombo;
    juce::StringArray deviceNames;
    juce::String currentDeviceName;

    // Output side
    juce::Label outputLabel{"", "Output"};
    juce::ToggleButton outputDawToggle;
    juce::TextEditor outputPortNameEditor;
    juce::String lastCommittedPortName{"Omnify"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiIOPanel)
};
