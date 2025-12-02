# just range, with inclusive 2nd arg
def irange(x, y):
    return range(x, y + 1)


def clamp_note(note):
    if note > 127:
        return (note % 12) + 108  # Clamp to highest octave
    if note < 0:
        return abs(note) % 12
    return note


