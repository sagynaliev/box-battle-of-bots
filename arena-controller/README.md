# ArenaController

Open `ArenaController/ArenaController.ino` in Arduino IDE and select your classic ESP32 board. The generic target is **ESP32 Dev Module**. Keep the generated `VoiceAssets.h` in the same folder.

The original sketch was unavailable. This standalone sketch implements the agreed effect types with the timing below. Compare the light behavior and DAC wiring with your existing setup before replacing its sketch.

## Wiring and input polarity

| Signal | ESP32 GPIO |
|---|---:|
| Red, green, blue, white (R/G/B/W) | 32, 26, 27, 14 |
| YK04 A (top left), B (top right), C (bottom left), D (bottom right) | 21, 18, 22, 19 |
| Mono DAC, channel 0 | 25 |

The sketch assumes active-high light drivers and sequential momentary outputs on the YK04. Wire each light through its existing driver and feed GPIO25 into the amplifier input using the appropriate coupling for that amplifier. GPIO26 is reserved for green.

1. With the receiver outputs disconnected from the ESP32, measure each output against receiver ground at idle, while its button is held and after release. Use a scope or logic analyzer if the output is too brief for a meter. Check which output corresponds to each button.
2. Use a common ground. If any receiver output is 5 V or can exceed 3.3 V, use suitable level shifting before connecting it. Use driven logic levels, or add the appropriate external pull resistor for open-drain or floating outputs. The sketch configures plain inputs without internal pulls.
3. Set each entry of `inputActiveLevel` to `HIGH` for a low-to-high press or `LOW` for a high-to-low press. Confirm that each output returns to idle on release. Select momentary mode if the receiver offers a choice and its outputs are latching.
4. To confirm the mapping on the ESP32, set `inputDiagnostics = true` and open Serial Monitor at 115200 baud. It reports startup levels and debounced transitions with light effects and audio disabled. Set it back to `false` for normal use. If your light drivers are active-low, set `lightsActiveHigh = false` before powering the lights.

## Remote controls

Release each button before pressing the next, including repeated presses of the same button. The receiver signal must remain active for at least 10 ms and idle for at least 15 ms between repeated presses. Start a repeated press within 800 ms or the next button of a pair within 900 ms of the previous press. Follow the order shown for each pair. Team introductions accept either order.

| # | Input | Result |
|---|---|---|
| 1 | A | Toggle the white channel independently of the red or blue effect |
| 2 | B | Steady blue |
| 3 | C | Steady red |
| 4 | D | “Three, two, one, FIGHT!” |
| 5 | A × 2 | All four light channels at 100% |
| 6 | B × 2 | Blue pulse |
| 7 | C × 2 | Red pulse |
| 8 | D → A | “Stop the battle!” |
| 9 | B × 3 | Fast blue blink |
| 10 | C × 3 | Fast red blink |
| 11 | Hold D for at least 650 ms | “Round finished!” |
| 12 | D → B or B → D | Blue pulse and “Introducing the Blue Team!” |
| 13 | D → C or C → D | Red pulse and “Introducing the Red Team!” |
| 14 | B → C | Alternating red and blue |
| 15 | A → D after Round finished | Four-channel chase and selected final announcement |

A single press waits up to 900 ms for a possible pair. B/C double presses wait up to 800 ms unless a new button closes the sequence sooner. Valid pairs, A × 2, and B/C triple presses take effect as soon as recognized. Holding D for at least 650 ms plays the round announcement once. Repeated D taps no longer select the stop or round commands. Lighting modes persist until a new light command. FIGHT and Round finished end the active effect and restore the white background. Hold D to finish the round before using A → D. The winner announcement is enabled once per round-end command. FIGHT resets it. A toggles only the white channel at 128/255 PWM. A × 2 sets all channels to 100%; the next team effect returns the other RGB channels to independent operation. A single press during the all-on effect exits it and turns off the white channel. Team effects leave the white channel at its set level. Pulses cycle over 1.2 seconds, fast blinks switch every 100 ms, red/blue alternation switches every 300 ms, lighting one of the two channels at a time while retaining the white background. The finale advances every 180 ms, setting the leading channel to full brightness and the others to a low level. Both effects stay visible at a 100% background setting.

## Voice and audio

The bundled header contains eight English recordings generated with Microsoft Guy Neural. Set `finaleVoice` in the sketch to `Voice::Final` for a generic announcement, `Voice::BlueWins` or `Voice::RedWins` for a named winner. This choice is made before compiling. Only the selected final clip and the five other announcements are included in the firmware. A newer announcement preempts playback after the already-buffered samples drain. The winner announcement waits for Round finished to play through.

A separate task feeds the continuous DAC DMA, clocked by the ESP32 audio PLL, with 16 kHz mono audio and midpoint samples during silence. Playback gain is explicitly **80%**, applied around unsigned PCM midpoint 128. Light updates and remote input run independently of DAC writes.

To generate optional macOS test audio, open Terminal and change into the extracted folder containing `generate-voices.sh`. With `ffmpeg` and `python3` available, run:

```bash
say -v '?'
bash generate-voices.sh Samantha
```

Substitute any installed macOS voice name. Edit the phrases in `generate-voices.sh` to change the wording. The script uses `say` to make AIFF clips, converts them to mono unsigned 8-bit PCM at 16 kHz with FFmpeg, and writes the Arduino arrays to `ArenaController/VoiceAssets.h`.

To compile with Arduino CLI and the installed ESP32 core 4.0.0:

```bash
arduino-cli core list
arduino-cli compile --fqbn esp32:esp32:esp32 ArenaController
```

## Import professional voice recordings

Use the filenames in `Voiceover-brief.md`. The importer accepts WAV or MP3 for each line, preferring WAV when both exist. With `ffmpeg` and `python3` installed, run from the extracted package:

```bash
bash import-voices.sh /path/to/recordings
```

The importer replaces `VoiceAssets.h` after all eight clips convert successfully. Recompile and upload the sketch to use the new recordings. The bundled Guy Neural MP3s are in the recordings folder.

## Phone control

Set a unique Wi-Fi password in `wifiPassword` before uploading. The public sketch contains the placeholder `REPLACE-BEFORE-USE`. The firmware creates a password-protected Wi-Fi network. Connect the phone to **BoxBot-Arena** using the password set in the sketch, then open **http://192.168.4.1/** in the phone browser. If the phone reports that this network has no internet, stay connected to it.

Choose **Қазақша**, **Русский**, or **English** at the top of the page. The language choice applies to the screen. The arena announcements use the bundled English recordings. The page uses descriptive command names and is served directly by the ESP32. In Lighting, Turn off all lights clears both the white background and the active RGB effect without interrupting audio.

The phone and the physical remote share the same controller state. The winner command becomes available after finishing the round with either control. Starting another round clears that eligibility. To use a different Wi-Fi password, set `wifiPassword` in the sketch before uploading.

### Extra sounds

In **Extra sounds**, choose an MP3 or WAV, enter a display name if desired, and select **Add sound**. Save multiple sounds and select **Play**, **Stop**, or **Delete** for each entry. Files stay in the browser on the phone that added them. Another phone has its own list. A clip can be up to 12 MB and 60 seconds.

The browser converts a selected clip to 16 kHz mono and streams it to the ESP32 as needed. Arena audio still uses 80% gain. Official announcements can interrupt extra sounds. Reloading the page or closing the browser does not erase saved entries, but clearing browser data does.

### Push-to-talk microphone

The direct HTTP page still supports lighting and saved-file playback. To use the microphone, put both the Mac and the phone on **BoxBot-Arena** and run this in the extracted folder on the Mac:

```bash
python3 arena-https.py
```

The script prints its HTTPS address and the path to its local certificate, `arena-ca.cer`. Transfer only that certificate to the operator's phone and install it as a trusted certificate on that phone. On iPhone, follow [Apple’s certificate trust steps](https://support.apple.com/en-us/102390). The certificate’s private key stays on the Mac. Never share or install its private key.

Keep Terminal open while operating the microphone. Open the printed **HTTPS** address on the phone, grant microphone access, then hold **Hold to talk**. Release to stop. If the first press opens a permission prompt, grant permission and press again. HTTPS and HTTP use separate browser storage, so sounds saved on the HTTP page need to be added again on the HTTPS page. The Mac, phone and ESP32 can run on the arena Wi-Fi without internet access.
