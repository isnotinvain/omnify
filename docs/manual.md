---
layout: default
title: Manual
---

# Omnify Manual + User Guide

## Overview

Omnify transforms any MIDI instrument into an omnichord / autoharp style instrument. The way you interact with it is by selecting a chord quality (Major / Minor / etc) and then playing just the root note of the chord. For example, to play an A minor chord, you select Minor, then play an A note on your keyboard. Omnify will intercept these midi inputs, and output instead a fully formed chord. The number of notes in the chord and which inversions are used are determined by the chord voicing styles. Additionally, omnify allows you to strum using any cc signal on your midi controller. Ideally you would use a touchpad, but you can use a modwheel or joystick if that's all your controller has.

## Setup: Midi Routing

Unfortunately many (most? all?) DAWs don't really support a pure midi -> midi transformation plugin. So Omnify presents itself as an instrument plugin. That means you'll have to put it on its own track, like any other instrument plugin - you can't drop it onto the track of the instrument you want to control like an effect.

### Input
For midi input, you have two choices:
1. From Device: Select your midi controller from the drop down. This bypasses your DAW and just reads midi directly off your midi controller.
2. From DAW: This mode uses whatever midi input your DAW sends into the plugin's track. You use your normal DAW midi routing settings / configuration to determine how that works. NOTE: It's not a good idea to have your DAW send midi from all sources to omnify, you will end up with a feedback loop as Omnify's outputs will become its inputs.

### Output
For midi output, you have two choices:
1. To Port: Choose a port name from the drop down. Omnify will create a virtual midi device with that name and send its outputs there. Your DAW will see this port as an additional midi controller that you can configure any track to accept as its input.
2. To DAW: This mode will send Omnify's output midi to your DAW as the plugin's output, which means you can route it however you would route any midi plugin.

Also note that the "Chords" and "Strum" panels each have a midi channel drop down which you can use to determine which channel chords and strum notes are sent to.

### On the Nature of Ableton and Windows
There are two important complications to keep in mind when working with either Ableton, Windows, or both:
1. When using the "To DAW" output mode in Ableton, Ableton "squashes" all midi data into channel 1. That means your chord notes and strum notes all end up on channel 1 regardless of the channels you've picked. For this reason, it really only makes sense to use "To Port" mode when using Ableton.
2.Which leads us to the next complication. Windows does not support dynamically creating virtual midi devices, so "To Port" mode can't automatically create a port for you on Windows. If you're not using Ableton, that's fine - just use "To DAW" mode. But if you are using Ableton on Windows (or just prefer "To Port"), you'll need third-party software to create the virtual port yourself, for example [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) or [LoopBe1](https://nerds.de/en/loopbe1.html). Make sure the virtual port's name matches the one you pick from Omnify's dropdown (eg. "Omnify" or "Omnify: BMO").

## Setup: MIDI Mapping the Controls
You need a way to pick which chord quality you want to play (Major, Minor, etc). This is done via the "Quality Selection" panel. There are two options:
1. **One Button Each** - For each chord quality, click the capsule and either press a note or pad on your controller to use for selecting that chord quality.
2. **One CC for All** - Pick a single midi CC (eg one knob) by clicking on the capsule and wiggling the knob / wheel you want to use. This mode uses the knob's position to pick the chord quality.

## Chords
The Chords Panel is where you choose how your chords are formed.

### Voicing Styles

The following voicing styles are available:
1. **Omnichord** - Uses the same algorithm as the real Omnichord. All chords consist of 3 notes only. Which notes in 4-note chords get dropped and which inversions are used are [explained here](https://github.com/isnotinvain/omnify/blob/main/Omnichord%20Facts/OM%20108%20Chord%20Voicings.md)
2. **Root Position** - Nothing fancy here, just the full chord in root position, using 3 or 4 notes as needed.
3. **Smoothed Full** - Just like Root Position, but constrained to the root's octave. Add9 keeps the 9 up one octave to preserve the nature of an Add9
4. **Omni-84** - This voicing is only useful when the instrument you are controlling is the [Omni-84 sample plugin](https://store.dehlimusikk.no/l/omni-84). That plugin uses 1 root note to play a chord, but it determines the chord quality by which octave you place the root note in. So this mode outputs only the root note, but shifted into the octave Omni-84 expects for your selected chord quality.

### Octave Modifiers
There are also 3 modifiers to choose from that determine what happens as you play root notes in different octaves.

1. **None** - The default modifier. In this mode, the octave you play your root note in determines the octave of all the notes in your chord. Eg if you go up one octave, you'll get the same exact notes + inversions, but each note will be up one octave.
2. **Fixed** - The octave you play the root note in is completely ignored. You get the same output for root C2, C3, C4, etc.
3. **Smooth** - This mode attempts to make the jump from one octave to another less jarring or a "smaller" step. For example, with modifier "None" if you play C3 Major with "Smoothed Full" voicing style, you will get the chord [C3, E3, G3]. If you play B3 Major you get [D#3, F#3, B3], which feels like a "higher" pitched chord than [C3, E3, G3]. But lets say you wanted to transition from C Major -> B Major with a downward feeling motion - eg you wanted the B Major to feel "lower" pitched than the C Major. With modifier "None" if you play B2 Major instead of B3 Major, you will get [D#2, F#2, B2]. That's a pretty big jump from [C3, E3, G3]. The "Smooth" modifier aims to solve this by having each octave further away from middle C use lower and lower (and higher and higher) inversions instead of shifting the entire chord by an octave. So in our previous example, both C3 Major and B3 Major are unchanged [C3, E3, G3] -> [D#3, F#3, B3]. But B2 Major with the smooth modifer becomes [B2, D#3, F#3]. This is done by moving only the highest note of the chord down 1 octave. If you play B1, the two highest notes both move down an octave, and so on.

### Latch + Stop All
There are two midi-learnable buttons here:

* **Latch** - When latch is on, chords keep playing even after you let go of the root note.
* **Stop All** - Stops the currently playing chord (without changing the latch mode).

The "latch mode" selector determines how the latch button behaves:

* **Momentary** (most common) - Each press of the button toggles latch on or off. Use this for controllers that send a single cc signal per press.
* **Toggle** - Use this for controllers that send a continuous stream of high cc values while toggled on and low values while toggled off. This mode can be nice because your midi controller will often change the pad's color while it's toggled on.

## Strum
When you strum on your controller, omnify will output notes from the currently playing chord across a few octaves.
Here you pick one midi CC to be treated as the strum plate. Ideally you use a touchpad for this, but a modwheel or joystick (or even a knob) can be used. You could also use a series of 13 pads that each send different ascending values on the same cc channel. If you want to do that, the cc values should be [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120]

Settings:
* **Gate** - How many milliseconds a strum note plays for (technically, how long to wait before sending a note-off event).
* **Cooldown** - This is used to prevent re-triggering the same note as you slide your finger up the touchplate (or rotate the modwheel etc). You probably don't need to adjust it but a smaller value will allow you to play the same strum note back to back with less delay between them, and a higher value will ignore when you play the same note too close together.

### Voicing Styles
There are two voicing styles for determining the order the notes play from the lowest strum position to the highest. They are:
1. **Omnichord** - Behaves exactly like the omnichord, which does not play the notes in ascending order, but rather uses its strange F#->F octave wrapping rules. It uses only the 3 "most important" notes of the chord repeated across each octave.
2. **Ascending** - Also uses the three "most important" notes but ordered low to high.