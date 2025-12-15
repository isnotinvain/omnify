#include "ChordQuality.h"

#include <stdexcept>

namespace {

// clang-format off
const std::vector<ChordQualityData> CHORD_QUALITY_TABLE = {
    {"MAJOR",     "Major",           "maj",   std::vector<int>{0, 4, 7},       std::vector<int>{0, 4, 7}},
    {"MINOR",     "Minor",           "m",     std::vector<int>{0, 3, 7},       std::vector<int>{0, 3, 7}},
    {"DOM_7",     "Dominant 7th",    "7",     std::vector<int>{0, 4, 7, 10},   std::vector<int>{0, 4, 10}},
    {"MAJOR_7",   "Major 7th",       "maj7",  std::vector<int>{0, 4, 7, 11},   std::vector<int>{0, 4, 11}},
    {"MINOR_7",   "Minor 7th",       "m7",    std::vector<int>{0, 3, 7, 10},   std::vector<int>{0, 3, 10}},
    {"DIM_7",     "Diminished 7th",  "dim7",  std::vector<int>{0, 3, 6, 9},    std::vector<int>{0, 3, 9}},
    {"AUGMENTED", "Augmented",       "aug",   std::vector<int>{0, 4, 8},       std::vector<int>{0, 4, 8}},
    {"SUS_4",     "Suspended 4th",   "sus4",  std::vector<int>{0, 5, 7},       std::vector<int>{0, 5, 7}},
    {"ADD_9",     "Add 9",           "add9",  std::vector<int>{0, 4, 7, 14},   std::vector<int>{0, 7, 14}},
};
// clang-format on

}  // namespace

const ChordQualityData& getChordQualityData(ChordQuality q) { return CHORD_QUALITY_TABLE[static_cast<size_t>(q)]; }

ChordQuality chordQualityFromName(std::string_view name) {
    for (size_t i = 0; i < ALL_CHORD_QUALITIES.size(); ++i) {
        if (CHORD_QUALITY_TABLE[i].name == name) {
            return ALL_CHORD_QUALITIES[i];
        }
    }
    throw std::runtime_error("Unknown ChordQuality: " + std::string(name));
}

void to_json(nlohmann::json& j, ChordQuality q) { j = getChordQualityData(q).name; }

void from_json(const nlohmann::json& j, ChordQuality& q) { q = chordQualityFromName(j.get<std::string_view>()); }