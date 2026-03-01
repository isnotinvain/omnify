---
layout: default
title: Home
---

<p class="tagline">Transform any MIDI instrument into an omnichord-inspired autoharp</p>

<div class="download-section">
  <a href="https://github.com/isnotinvain/omnify/releases/latest" class="lcars-button">Download Latest Release</a>

  <div class="platforms">
    <span class="platform-badge">macOS</span>
    <span class="platform-badge">Windows</span>
    <span class="platform-badge">Linux</span>
  </div>
</div>

<div class="hero">
  <img src="{{ '/assets/images/omnify-screenshot.png' | relative_url }}" alt="Omnify plugin screenshot" class="hero-image">
</div>

Omnify is a VST3/AU plugin (or standalone app) that turns any MIDI controller into an omnichord / autoharp style instrument. Select a chord quality (such as Major) using a pad (or any midi note or cc), then press the root of the chord on your keyboard and omnify will send a fully formed midi chord to whatever destination instrument you wish. If your controller has a touchplate you can use it to strum, just like an omnichord, and if not you can use any midi cc (such as your modwheel or joystick) to strum instead.

## Features
- **9 chord qualities** — Major, Minor, Dominant 7, Major 7, Minor 7, Diminished 7, Augmented, Sus 4, Add 9
- **4 chord voicing styles** - Accurate omnichord voicings, full root position, full root position smooth voiced, and a compatibility mode that allows you to properly control the [Omni-84 sample plugin](https://store.dehlimusikk.no/l/omni-84)
- **Flexible MIDI input** — configure any MIDI note or CC as quality selector or strum signal
- **Separate MIDI channel output** for chord and strum output, so you can use a different instrument for each
- **Works standalone or as a plugin** in any DAW that supports VST3 or AU

## Bugs and Feature Requests
- **Something not working?** - Please create an [issue on github](https://github.com/isnotinvain/omnify/issues)
- **Have a request?** - Please create an [issue on github](https://github.com/isnotinvain/omnify/issues) and label it with the "enhancement" label
- **Pull requests welcome** - want to contribute? Send me a pull request on github!
