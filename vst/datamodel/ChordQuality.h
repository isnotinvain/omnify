#pragma once

#include <array>
#include <json.hpp>
#include <string>
#include <vector>

enum class ChordQuality { MAJOR, MINOR, DOM_7, MAJOR_7, MINOR_7, DIM_7, AUGMENTED, SUS_4, ADD_9 };

struct ChordQualityData {
    std::string name;       // "MAJOR" - for JSON serialization
    std::string nice_name;  // "Major" - for UI display
    std::string suffix;     // "maj" - for chord notation like "Cmaj"
    std::vector<int> offsets;
    std::vector<int> triad_offsets;
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