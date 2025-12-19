#pragma once

#include <array>
#include <json.hpp>
#include <string>
#include <vector>

enum class ChordQuality { MAJOR, MINOR, DOM_7, MAJOR_7, MINOR_7, DIM_7, AUGMENTED, SUS_4, ADD_9 };

struct ChordQualityData {
    std::string name;      // matches ChordQuality enum names
    std::string niceName;  // for UI, eg "Diminished 7th"
    std::string suffix;    // "maj" - for chord notation like "Cmaj"

    // delta from the root note of the chord in it's full form (sometimes more than 3 notes)
    std::vector<int> offsets;

    // The omnichord can only play 3 notes at a time. So for larger chords they choose
    // which notes to drop. It's always the 5th except add9 drops the 3rd instead.
    // That's represented here for convenience, triadOffsets is the 3 offsets from offsets
    // above that are used when making a 3 note version of this chord.
    std::vector<int> triadOffsets;
};

inline constexpr std::array<ChordQuality, 9> ALL_CHORD_QUALITIES = {ChordQuality::MAJOR,     ChordQuality::MINOR,   ChordQuality::DOM_7,
                                                                    ChordQuality::MAJOR_7,   ChordQuality::MINOR_7, ChordQuality::DIM_7,
                                                                    ChordQuality::AUGMENTED, ChordQuality::SUS_4,   ChordQuality::ADD_9};

// Get data for a specific quality
const ChordQualityData& getChordQualityData(ChordQuality q);

// Lookup by name (for JSON deserialization), throws if not found
ChordQuality chordQualityFromName(std::string_view name);

// JSON serialization - serializes to/from the enum name string
void to_json(nlohmann::json& j, ChordQuality q);
void from_json(const nlohmann::json& j, ChordQuality& q);

struct Chord {
    ChordQuality quality;
    int root;
};