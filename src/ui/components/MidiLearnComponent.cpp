#include "MidiLearnComponent.h"

#include "../LcarsColors.h"
#include "../LcarsLookAndFeel.h"

MidiLearnComponent::MidiLearnComponent() { setWantsKeyboardFocus(true); }

MidiLearnComponent::~MidiLearnComponent() {
    if (currentlyLearning.load() == this) {
        currentlyLearning.store(nullptr);
    }
}

void MidiLearnComponent::broadcastMidi(const juce::MidiMessage& message) {
    auto* active = currentlyLearning.load();
    if (active != nullptr) {
        active->processMessage(message);
    }
}

void MidiLearnComponent::setLearnedValue(MidiLearnedValue val) {
    learnedType.store(val.type);
    learnedValue.store(val.value);
    repaint();
}

MidiLearnedValue MidiLearnComponent::getLearnedValue() const { return {.type = learnedType.load(), .value = learnedValue.load()}; }

void MidiLearnComponent::setAcceptMode(MidiAcceptMode mode) { acceptMode = mode; }

void MidiLearnComponent::setAspectRatio(float ratio) { aspectRatio = ratio; }

void MidiLearnComponent::processMessage(const juce::MidiMessage& msg) {
    if (!isLearning.load()) {
        return;
    }

    bool acceptNotes = acceptMode == MidiAcceptMode::NotesOnly || acceptMode == MidiAcceptMode::Both;
    bool acceptCCs = acceptMode == MidiAcceptMode::CCsOnly || acceptMode == MidiAcceptMode::Both;

    if (acceptNotes && msg.isNoteOn() && msg.getVelocity() > 0) {
        learnedType.store(MidiLearnedType::Note);
        learnedValue.store(msg.getNoteNumber());
        isLearning.store(false);
        if (currentlyLearning.load() == this) {
            currentlyLearning.store(nullptr);
        }
        triggerAsyncUpdate();
        if (onValueChanged) {
            onValueChanged({.type = MidiLearnedType::Note, .value = msg.getNoteNumber()});
        }
        return;
    }
    if (acceptCCs && msg.isController()) {
        learnedType.store(MidiLearnedType::CC);
        learnedValue.store(msg.getControllerNumber());
        isLearning.store(false);
        if (currentlyLearning.load() == this) {
            currentlyLearning.store(nullptr);
        }
        triggerAsyncUpdate();
        if (onValueChanged) {
            onValueChanged({.type = MidiLearnedType::CC, .value = msg.getControllerNumber()});
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
    }
    return "CC " + juce::String(value);
}

void MidiLearnComponent::paint(juce::Graphics& g) {
    auto localBounds = getLocalBounds().reduced(2);

    // Apply aspect ratio constraint if set - fit largest possible rectangle maintaining ratio
    if (aspectRatio > 0.0F) {
        float currentRatio = static_cast<float>(localBounds.getWidth()) / localBounds.getHeight();

        if (currentRatio > aspectRatio) {
            // Too wide - constrain width based on height
            int targetWidth = static_cast<int>(localBounds.getHeight() * aspectRatio);
            int xOffset = (localBounds.getWidth() - targetWidth) / 2;
            localBounds = localBounds.withWidth(targetWidth).withX(localBounds.getX() + xOffset);
        } else if (currentRatio < aspectRatio) {
            // Too tall - constrain height based on width
            int targetHeight = static_cast<int>(localBounds.getWidth() / aspectRatio);
            int yOffset = (localBounds.getHeight() - targetHeight) / 2;
            localBounds = localBounds.withHeight(targetHeight).withY(localBounds.getY() + yOffset);
        }
    }

    boxBounds = localBounds;
    auto bounds = boxBounds.toFloat();
    const float borderThickness = 1.0F;
    const float radius = bounds.getHeight() * 0.5F;

    // Background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius);

    // Border
    g.setColour(isLearning.load() ? LcarsColors::africanViolet : LcarsColors::orange);
    g.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5F), radius, borderThickness);

    // Text
    if (auto* laf = dynamic_cast<LcarsLookAndFeel*>(&getLookAndFeel())) {
        g.setFont(laf->getOrbitronFont(LcarsLookAndFeel::fontSizeSmall));
    }
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

    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (noteNumber / 12) - 1;
    int noteIndex = noteNumber % 12;

    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}