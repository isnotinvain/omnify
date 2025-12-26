#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../LcarsColors.h"
#include "../LcarsLookAndFeel.h"
#include "../components/ChordQualitySelector.h"
#include "../components/MidiLearnComponent.h"
#include "../components/VariantSelector.h"

// Forward declaration
class OmnifyAudioProcessor;

class ChordQualityPanel : public juce::Component {
   public:
    explicit ChordQualityPanel(OmnifyAudioProcessor& processor);
    ~ChordQualityPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refreshFromSettings();

   private:
    void setupCallbacks();

    OmnifyAudioProcessor& processor;

    // Title
    juce::Label titleLabel{"", "Quality Selection"};

    // Selection style selector
    VariantSelector styleSelector;

    // Variant 0: Button per chord quality (grid)
    ChordQualitySelector qualityGrid;

    // Variant 1: One CC for all - custom container that lays out children in resized()
    class SingleCcContainer : public juce::Component {
       public:
        juce::Label label{"", "CC"};
        MidiLearnComponent midiLearn;

        SingleCcContainer() {
            addAndMakeVisible(label);
            addAndMakeVisible(midiLearn);
        }

        void resized() override {
            auto bounds = getLocalBounds();
            midiLearn.setBounds(bounds.removeFromRight(LcarsLookAndFeel::capsuleWidth));
            label.setBounds(bounds);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleCcContainer)
    };
    SingleCcContainer singleCcContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordQualityPanel)
};
