from daemomnify import chords


def reify_chord_voicing_style(style):
    """
    Splat out a ChordVoicingStyle into a dict.
    Might be better for performance?
    """
    res = {}
    for quality in chords.ChordQuality.all:
        res[quality] = {}
        for note in range(0, 128):
            res[quality][note] = style.construct_chord(quality, note)

    return res


# print(reify_chord_voicing_style(chords.RootPositionStyle(None)))
