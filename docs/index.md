---
layout: default
title: Home
---

<div class="hero">
  <h1>OMNIFY</h1>
  <p class="tagline">Transform any MIDI instrument into an omnichord-inspired autoharp</p>

  <div class="hero-image-placeholder">
    Demo screenshot coming soon
  </div>
</div>

Omnify is a VST3/AU plugin that turns any MIDI controller into an omnichord / autoharp style instrument. Strum on a mod wheel or touch plate while selecting chord roots and qualities from pads and keys — Omnify handles the voicing and outputs chords and strums on separate MIDI channels.

## Features

- **Chord voicing engine** with multiple voicing styles including authentic Omnichord voicings, root position, ascending arpeggios, and custom voicings from JSON files
- **9 chord qualities** — Major, Minor, Dom 7, Major 7, Minor 7, Dim 7, Augmented, Sus 4, Add 9
- **Flexible MIDI input** — configure any MIDI note or CC as a chord root, quality selector, or strum trigger
- **MIDI Learn** — click any control and wiggle a knob to assign it
- **Latch mode** — hold chords without keeping keys pressed
- **Sample-accurate strum timing** with configurable gate time and cooldown
- **Separate MIDI channels** for chord and strum output
- **Works standalone or as a plugin** in any DAW that supports VST3 or AU

## Download

<div class="download-section">
  <a href="https://github.com/isnotinvain/omnify/releases/latest" class="lcars-button">Download Latest Release</a>

  <div class="platforms">
    <span class="platform-badge">macOS</span>
    <span class="platform-badge">Windows</span>
    <span class="platform-badge">Linux</span>
  </div>
</div>

## How It Works

1. **Connect** a MIDI controller (keyboard, pad controller, or anything with MIDI output)
2. **Map** keyboard notes to chord roots and pads to chord qualities
3. **Strum** using a mod wheel, touch strip, or any CC — Omnify generates the notes
4. **Route** the output MIDI to your favorite synths and samplers

## License

<div class="license">
  Omnify is licensed under <a href="https://creativecommons.org/licenses/by-nc-sa/4.0/">CC BY-NC-SA 4.0</a>. Free for non-commercial use.
</div>
