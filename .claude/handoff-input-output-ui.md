# Handoff: Input/Output UI Implementation

## Goal
Add UI controls for selecting MIDI input and output routing. The backend already supports a 2x2 matrix (DAW or Device for both input and output), but the UI only has partial support for input selection.

## Final Design (Agreed)

### Layout
Keep input/output in the existing top-right panel, split into two halves:

```
┌─────────────────────────────────────────────────────────┐
│   Input [DAW]              Output [DAW]                 │
│   [  ComboBox  ▼]          [  TextBox  ]                │
└─────────────────────────────────────────────────────────┘
```

- **Input side**: "Input" label + [DAW] toggle on same row, device ComboBox below
- **Output side**: "Output" label + [DAW] toggle on same row, port name TextBox below
- Top row height can increase if needed
- Use same font sizes and row heights as other panels

### Behavior

**Input:**
- [DAW] toggle ON (default): ComboBox disabled, MIDI comes from DAW
- [DAW] toggle OFF: ComboBox enabled, user selects hardware MIDI device

**Output:**
- [DAW] toggle ON (default): TextBox disabled, MIDI goes to DAW
- [DAW] toggle OFF: TextBox enabled, creates virtual port with entered name
- Default port name: "Omnify"

### Styling
- DAW toggle buttons: Capsule style like existing buttons, same orange color when active

### Text Box Debouncing
To avoid opening/closing ports on every keystroke, apply settings changes on:
- Enter key pressed, OR
- Focus lost (user clicks away or tabs out)

JUCE's `TextEditor` has both `onReturnKey` and `onFocusLost` callbacks for this.

---

## Current State

### Data Model (already complete)
- `src/datamodel/DawOrDevice.h` - `std::variant<Daw, Device>` type
- `OmnifySettings` has `input` and `output` fields, both `DawOrDevice`, defaulting to `Daw{}`
- JSON serialization already works

### Backend (already complete)
- `PluginProcessor::processBlock()` routes based on `isDevice(settings->input)` and `isDevice(settings->output)`
- `reconcileDevices()` opens/closes hardware MIDI ports when settings change
- Settings changes via `modifySettings()` trigger `AsyncUpdater` for device reconciliation

### Existing Input UI
- `MidiDeviceSelectorComponent` (`src/ui/components/MidiDeviceSelectorItem.h/.cpp`)
  - ComboBox listing available MIDI input devices
  - Timer-based refresh of device list
  - Callback: `onDeviceSelected(const juce::String& deviceName)`
- Used in `PluginEditor.cpp` - sets `settings->input = Device{deviceName}`

## Implementation Plan

### 1. Create new unified I/O panel component
Create `MidiIOPanel` component that contains:
- Input label + DAW toggle + device ComboBox
- Output label + DAW toggle + port name TextBox
- Callbacks: `onInputChanged(bool useDaw, juce::String deviceName)`, `onOutputChanged(bool useDaw, juce::String portName)`

### 2. Input DAW toggle behavior
- When DAW toggled ON: disable ComboBox, call `onInputChanged(true, "")`
- When DAW toggled OFF: enable ComboBox, call `onInputChanged(false, selectedDevice)`
- ComboBox selection change: call `onInputChanged(false, selectedDevice)`

### 3. Output DAW toggle behavior
- When DAW toggled ON: disable TextBox, call `onOutputChanged(true, "")`
- When DAW toggled OFF: enable TextBox, call `onOutputChanged(false, textBoxValue)`
- TextBox Enter/focus lost: call `onOutputChanged(false, textBoxValue)`

### 4. PluginEditor integration
```cpp
midiIOPanel.onInputChanged = [this](bool useDaw, const juce::String& deviceName) {
    omnifyProcessor.modifySettings([useDaw, deviceName](OmnifySettings& s) {
        s.input = useDaw ? DawOrDevice{Daw{}} : DawOrDevice{Device{deviceName.toStdString()}};
    });
};

midiIOPanel.onOutputChanged = [this](bool useDaw, const juce::String& portName) {
    omnifyProcessor.modifySettings([useDaw, portName](OmnifySettings& s) {
        s.output = useDaw ? DawOrDevice{Daw{}} : DawOrDevice{Device{portName.toStdString()}};
    });
};
```

### 5. refreshFromSettings()
- Set input DAW toggle based on `isDaw(settings->input)`
- Set input ComboBox selection to `getDeviceName(settings->input)` if device
- Set output DAW toggle based on `isDaw(settings->output)`
- Set output TextBox to `getDeviceName(settings->output)` if device, else "Omnify"

## Files to Modify/Create
1. Create `src/ui/components/MidiIOPanel.h/.cpp` - new unified component
2. `src/PluginEditor.cpp` - replace `midiDeviceSelector` with `midiIOPanel`
3. `src/PluginEditor.h` - update member
4. Can remove or keep `MidiDeviceSelectorItem` (may reuse ComboBox logic)

## Testing
1. Input DAW toggle ON - verify MIDI from DAW is processed
2. Input DAW toggle OFF + select device - verify MIDI from device is processed
3. Output DAW toggle ON - verify MIDI appears on DAW track output
4. Output DAW toggle OFF - verify virtual port "Omnify" appears in other apps
5. Change port name, hit Enter - verify new port name appears
6. Change port name, click away - verify new port name appears
7. Save/load plugin state - verify input/output settings persist
8. Toggle states and selections persist across save/load

## Reference: How Settings Flow
```
UI Component
    -> onInputChanged / onOutputChanged callback
    -> omnifyProcessor.modifySettings([](OmnifySettings& s) { ... })
        -> creates new OmnifySettings copy
        -> applies mutation
        -> atomic_store to omnifySettings
        -> triggers AsyncUpdater
    -> handleAsyncUpdate()
        -> reconcileDevices()
        -> opens/closes MIDI ports as needed
```