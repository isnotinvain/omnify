#pragma once

#include <json.hpp>

enum class VoicingModifier { FIXED, NONE, SMOOTH, DYNAMIC };

NLOHMANN_JSON_SERIALIZE_ENUM(VoicingModifier, {
    {VoicingModifier::FIXED, "FIXED"},
    {VoicingModifier::NONE, "NONE"},
    {VoicingModifier::SMOOTH, "SMOOTH"},
    {VoicingModifier::DYNAMIC, "DYNAMIC"},
})
