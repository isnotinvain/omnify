#pragma once

#include <foleys_gui_magic/foleys_gui_magic.h>

#include "GeneratedParams.h"
#include "ui/components/MidiLearnComponent.h"

//==============================================================================
class OmnifyAudioProcessor : public foleys::MagicProcessor {
   public:
    OmnifyAudioProcessor();

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void initialiseBuilder(foleys::MagicGUIBuilder& builder) override;

   private:
    GeneratedParams::Params params;
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OmnifyAudioProcessor)
};
