#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "GeneratedParams.h"

//==============================================================================
class OmnifyAudioProcessor : public foleys::MagicProcessor {
   public:
    OmnifyAudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

   private:
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
