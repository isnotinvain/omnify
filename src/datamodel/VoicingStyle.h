#pragma once

#include <string>
#include <vector>

#include "ChordQuality.h"

enum class VoicingFor { Chord, Strum };

// Abstract base class for voicing styles.
template <VoicingFor T>
class VoicingStyle {
   public:
    virtual ~VoicingStyle() = default;
    virtual std::string displayName() const = 0;
    virtual std::string description() const = 0;
    virtual std::vector<int> constructChord(ChordQuality quality, int root) const = 0;
};
