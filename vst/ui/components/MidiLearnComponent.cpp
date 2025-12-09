#include "MidiLearnComponent.h"

#include "../LcarsColors.h"

MidiLearnComponent::MidiLearnComponent() { setWantsKeyboardFocus(true); }

MidiLearnComponent::~MidiLearnComponent() {
    if (currentlyLearning.load() == this) {
        currentlyLearning.store(nullptr);
    }
}

void MidiLearnComponent::broadcastMidi(const juce::MidiBuffer& buffer) {
    auto* active = currentlyLearning.load();
    if (active != nullptr) {
        active->processNextMidiBuffer(buffer);
    }
}

void MidiLearnComponent::setLearnedValue(MidiLearnedValue val) {
    learnedType.store(val.type);
    learnedValue.store(val.value);
    repaint();
}

MidiLearnedValue MidiLearnComponent::getLearnedValue() const {
    return {learnedType.load(), learnedValue.load()};
}

void MidiLearnComponent::setAcceptMode(MidiAcceptMode mode) { acceptMode = mode; }

void MidiLearnComponent::processNextMidiBuffer(const juce::MidiBuffer& buffer) {
    if (!isLearning.load()) {
        return;
    }

    bool acceptNotes =
        acceptMode == MidiAcceptMode::NotesOnly || acceptMode == MidiAcceptMode::Both;
    bool acceptCCs = acceptMode == MidiAcceptMode::CCsOnly || acceptMode == MidiAcceptMode::Both;

    for (const auto& metadata : buffer) {
        auto msg = metadata.getMessage();
        if (acceptNotes && msg.isNoteOn() && msg.getVelocity() > 0) {
            learnedType.store(MidiLearnedType::Note);
            learnedValue.store(msg.getNoteNumber());
            isLearning.store(false);
            if (currentlyLearning.load() == this) {
                currentlyLearning.store(nullptr);
            }
            triggerAsyncUpdate();
            if (onValueChanged) {
                onValueChanged({MidiLearnedType::Note, msg.getNoteNumber()});
            }
            return;
        } else if (acceptCCs && msg.isController()) {
            learnedType.store(MidiLearnedType::CC);
            learnedValue.store(msg.getControllerNumber());
            isLearning.store(false);
            if (currentlyLearning.load() == this) {
                currentlyLearning.store(nullptr);
            }
            triggerAsyncUpdate();
            if (onValueChanged) {
                onValueChanged({MidiLearnedType::CC, msg.getControllerNumber()});
            }
            return;
        }
    }
}

juce::String MidiLearnComponent::getDisplayText() const {
    if (isLearning.load()) {
        return "...";
    }

    auto type = learnedType.load();
    auto value = learnedValue.load();

    if (type == MidiLearnedType::None || value < 0) {
        return "";
    }

    if (type == MidiLearnedType::Note) {
        return noteNumberToName(value);
    } else {
        return "CC" + juce::String(value);
    }
}

void MidiLearnComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().reduced(2);
    int boxWidth = static_cast<int>(bounds.getHeight() * 2.5f);
    boxBounds = bounds.removeFromRight(boxWidth);

    if (isLearning.load()) {
        g.setColour(juce::Colours::black);
    } else {
        g.setColour(juce::Colours::black);
    }
    g.fillRect(boxBounds);

    g.setColour(isLearning.load() ? LcarsColors::africanViolet : LcarsColors::orange);
    g.drawRect(boxBounds, 2);

    g.setColour(LcarsColors::orange);
    g.drawText(getDisplayText(), boxBounds, juce::Justification::centred);
}

void MidiLearnComponent::resized() {}

void MidiLearnComponent::mouseDown(const juce::MouseEvent& event) {
    if (boxBounds.contains(event.getPosition())) {
        startLearning();
    }
}

void MidiLearnComponent::handleAsyncUpdate() { repaint(); }

void MidiLearnComponent::startLearning() {
    auto* prev = currentlyLearning.load();
    if (prev != nullptr && prev != this) {
        prev->stopLearning();
    }
    currentlyLearning.store(this);
    isLearning.store(true);
    grabKeyboardFocus();
    repaint();
}

bool MidiLearnComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey && isLearning.load()) {
        stopLearning();
        return true;
    }
    return false;
}

void MidiLearnComponent::stopLearning() {
    if (currentlyLearning.load() == this) {
        currentlyLearning.store(nullptr);
    }
    isLearning.store(false);
    repaint();
}

juce::String MidiLearnComponent::noteNumberToName(int noteNumber) {
    if (noteNumber < 0 || noteNumber > 127) {
        return {};
    }

    static const char* noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                      "F#", "G",  "G#", "A",  "A#", "B"};
    int octave = (noteNumber / 12) - 1;
    int noteIndex = noteNumber % 12;

    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}