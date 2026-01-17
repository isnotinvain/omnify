#include "PianoKeyboardDisplay.h"

void PianoKeyboardDisplay::setActiveNotes(const std::array<bool, 128>& notes) {
    activeNotes = notes;
    repaint();
}

void PianoKeyboardDisplay::setKeyRange(int start, int end) {
    startNote = start;
    endNote = end;
    repaint();
}

bool PianoKeyboardDisplay::isBlackKey(int note) {
    int n = note % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

int PianoKeyboardDisplay::countWhiteKeys() const {
    int count = 0;
    for (int i = startNote; i < endNote; i++) {
        if (!isBlackKey(i)) {
            count++;
        }
    }
    return count;
}

void PianoKeyboardDisplay::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    int numWhiteKeys = countWhiteKeys();
    if (numWhiteKeys == 0) {
        return;
    }

    float whiteKeyWidth = bounds.getWidth() / static_cast<float>(numWhiteKeys);
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float blackKeyHeight = bounds.getHeight() * 0.6f;

    // Draw white keys first
    int whiteKeyIndex = 0;
    for (int note = startNote; note < endNote; note++) {
        if (!isBlackKey(note)) {
            float x = static_cast<float>(whiteKeyIndex) * whiteKeyWidth;
            auto keyBounds = juce::Rectangle<float>(x, 0, whiteKeyWidth, bounds.getHeight());

            // Fill
            if (activeNotes[static_cast<size_t>(note)]) {
                g.setColour(highlightColour);
            } else {
                g.setColour(whiteKeyColour);
            }
            g.fillRect(keyBounds);

            // Border
            g.setColour(juce::Colours::black);
            g.drawRect(keyBounds, 1.0F);

            whiteKeyIndex++;
        }
    }

    // Draw black keys on top
    whiteKeyIndex = 0;
    for (int note = startNote; note < endNote; note++) {
        if (!isBlackKey(note)) {
            // Check if next note is a black key
            if (note + 1 < endNote && isBlackKey(note + 1)) {
                float x = (static_cast<float>(whiteKeyIndex) + 1.0F) * whiteKeyWidth - blackKeyWidth / 2.0F;
                auto keyBounds = juce::Rectangle<float>(x, 1.0F, blackKeyWidth, blackKeyHeight);

                // Fill
                if (activeNotes[static_cast<size_t>(note + 1)]) {
                    g.setColour(highlightColour);
                } else {
                    g.setColour(blackKeyColour);
                }
                g.fillRect(keyBounds);
            }
            whiteKeyIndex++;
        }
    }
}
