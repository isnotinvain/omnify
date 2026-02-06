---
layout: default
title: Manual
---

# Omnify Manual

## Overview

<div class="manual-section">

Omnify transforms any MIDI instrument into an omnichord / autoharp style instrument. It receives MIDI input, interprets it as chord selections and strum gestures, and outputs the resulting notes on configurable MIDI channels.

<p class="placeholder-note">Detailed getting started guide to be added.</p>
</div>

## MIDI Setup

<div class="manual-section">

### Input Device

Configure which MIDI device Omnify listens to. In standalone mode, select a hardware MIDI device directly. As a plugin, Omnify receives MIDI from your DAW's track routing.

### Output Device

In standalone mode, select a hardware MIDI output device. As a plugin, MIDI output is routed back through your DAW.

### Channels

Omnify uses separate MIDI channels for chord and strum output, allowing you to route them to different instruments in your DAW.

<p class="placeholder-note">Channel configuration details to be added.</p>
</div>

## Chord Selection

<div class="manual-section">

### Root Notes

Keyboard notes set the root of the current chord. By default, each note on the keyboard corresponds to a chromatic root (C, C#, D, etc.).

### Chord Qualities

Omnify supports 9 chord qualities:

- **Major**
- **Minor**
- **Dom 7**
- **Major 7**
- **Minor 7**
- **Dim 7**
- **Augmented**
- **Sus 4**
- **Add 9**

### Selection Styles

There are two ways to select chord qualities:

- **Button Per Chord Quality** — assign a separate MIDI button to each quality
- **CC Range Per Chord Quality** — map a single CC to cycle through qualities based on its value

<p class="placeholder-note">Detailed configuration instructions to be added.</p>
</div>

## Strum / Mod Wheel

<div class="manual-section">

The strum input triggers note playback of the current chord. Typically mapped to a mod wheel or touch plate. Omnify provides sample-accurate timing for strum events.

### Strum Gate Time

Controls how long each strummed note is held. Adjustable in milliseconds.

### Strum Cooldown

Minimum time between strum triggers to prevent retriggering. Adjustable in milliseconds.

<p class="placeholder-note">Strum configuration details to be added.</p>
</div>

## Voicing Styles

<div class="manual-section">

Voicing styles determine how a chord root + quality is translated into actual MIDI notes.

### Omnichord Chords / Strum

Authentic Omnichord voicings with relative F# octave positioning.

### Root Position

Simple root position triads and seventh chords.

### Plain Ascending

Ascending arpeggio pattern.

### From File

Load custom voicings from a JSON file. Supports both offset-based intervals and absolute MIDI note numbers.

<p class="placeholder-note">Voicing style details and examples to be added.</p>
</div>

## Latch Mode

<div class="manual-section">

When latch mode is enabled, chords sustain after you release the keys. The chord remains active until you select a new chord or disable latch mode.

<p class="placeholder-note">Latch mode details to be added.</p>
</div>

## MIDI Learn

<div class="manual-section">

MIDI Learn lets you assign any MIDI control to any Omnify parameter by clicking the parameter and then moving the desired control on your MIDI device.

<p class="placeholder-note">MIDI Learn walkthrough to be added.</p>
</div>

## Settings

<div class="manual-section">

Omnify stores its configuration as JSON. Settings include MIDI device selection, channel assignments, button mappings, voicing style selections, and strum parameters.

Settings persist across sessions and can be exported/imported as JSON files.

<p class="placeholder-note">Settings reference to be added.</p>
</div>
