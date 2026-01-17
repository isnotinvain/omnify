#pragma once

#include <json.hpp>
#include <map>

#include "../voicing_styles/Omni84.h"
#include "../voicing_styles/OmnichordChords.h"
#include "../voicing_styles/OmnichordStrum.h"
#include "../voicing_styles/PlainAscending.h"
#include "../voicing_styles/RootPosition.h"
#include "../voicing_styles/SmoothedFull.h"
#include "VoicingStyle.h"

enum class ChordVoicingType { Omnichord, RootPosition, Omni84, SmoothedFull };

enum class StrumVoicingType { Omnichord, PlainAscending };

inline const std::map<ChordVoicingType, const VoicingStyle<VoicingFor::Chord>*>& chordVoicings() {
    static OmnichordChords omnichord;
    static RootPosition rootPosition;
    static Omni84 omni84;
    static SmoothedFull smoothedFull;

    static const std::map<ChordVoicingType, const VoicingStyle<VoicingFor::Chord>*> map = {
        {ChordVoicingType::Omnichord, &omnichord},
        {ChordVoicingType::RootPosition, &rootPosition},
        {ChordVoicingType::Omni84, &omni84},
        {ChordVoicingType::SmoothedFull, &smoothedFull},
    };
    return map;
}

inline const std::map<StrumVoicingType, const VoicingStyle<VoicingFor::Strum>*>& strumVoicings() {
    static OmnichordStrum omnichord;
    static PlainAscending plainAscending;

    static const std::map<StrumVoicingType, const VoicingStyle<VoicingFor::Strum>*> map = {
        {StrumVoicingType::Omnichord, &omnichord},
        {StrumVoicingType::PlainAscending, &plainAscending},
    };
    return map;
}

inline ChordVoicingType chordVoicingTypeFor(const VoicingStyle<VoicingFor::Chord>* style) {
    for (const auto& [type, ptr] : chordVoicings()) {
        if (ptr == style) return type;
    }
    return ChordVoicingType::Omnichord;
}

inline StrumVoicingType strumVoicingTypeFor(const VoicingStyle<VoicingFor::Strum>* style) {
    for (const auto& [type, ptr] : strumVoicings()) {
        if (ptr == style) return type;
    }
    return StrumVoicingType::Omnichord;
}

NLOHMANN_JSON_SERIALIZE_ENUM(ChordVoicingType, {
    {ChordVoicingType::Omnichord, "Omnichord"},
    {ChordVoicingType::RootPosition, "RootPosition"},
    {ChordVoicingType::Omni84, "Omni84"},
    {ChordVoicingType::SmoothedFull, "SmoothedFull"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(StrumVoicingType, {
    {StrumVoicingType::Omnichord, "Omnichord"},
    {StrumVoicingType::PlainAscending, "PlainAscending"},
})
