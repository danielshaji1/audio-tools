# Open Sound Meter Modded for macOS

Open Sound Meter Modded extends [Open Sound Meter](https://github.com/psmokotnin/osm/tree/1e08de2322f5e8849e1ba39e1d3993187a7cc75c) by Pavel Smokotnin and contributors. This independent macOS derivative uses the upstream C++ measurement engine, Qt Quick workspace, Core Audio routing and OpenGL renderer. Original notices are preserved. See LICENSE for the GNU GPL and the notices in fonts/ for font licensing.

## Measure

Select a measurement in the right sidebar to choose the input device, microphone and reference channels. A common clock for both channels is needed for transfer measurements. Import the microphone response file in measurement properties. Use the 94 dB control with a stable 94 dB acoustic calibrator before interpreting readings as absolute SPL. Recalibrate after changing the microphone, preamp gain or channel.

The default workspace stacks magnitude, phase and numerical levels. Each chart has a selector. Choose Spectrum for live RTA and open chart properties to change resolution, scale or display style. Select Generator for output routing, waveform, level and sine frequency. Click a measurement color swatch to choose its trace color.

Enable Target Trace and click its name to edit the native curve. Import additional comparison curves from File → Import using frequency in Hz and level in dB in the first two columns of CSV or tab-separated text. Imported curves appear as named sources. File → Save stores source data and routing in an OSM workspace. File → Recent projects reopens it. Display preferences and the native target editor use application settings.

## Sunday service

Use the top-left red **Record service** control from any plot. Select the measurement and service name in that same toolbar. File → Record service or ⌘⇧R toggles recording; **Service Timeline** displays the graph and archive. Switching plots keeps the recorder running. **Finish service**, input deactivation, and normal application quit finalize the recording. The archive contains one-second Fast and Slow A-weighted readings, RTA snapshots, and a PDF plotting both levels against elapsed minutes with the service date, input, duration, sample count, and highest sampled levels. **Reports** opens current PDFs or the folder for an older recording.

Use **Reporting Preferences…** (⌘,) to choose **Local archive**, **Shared folder**, or **Apple Mail** and establish a 6–12 digit leader PIN. For delivery without Apple Mail or an Apple ID, select an existing mounted SMB share or a folder synchronized to another device. Every stop saves the PDF locally and copies it to the configured folder. An unavailable destination is retried while the app is open. For Apple Mail, enter the leader’s email. Subsequent changes to the recipient or automatic delivery require that PIN. Preferences are saved in the Mac login Keychain. A previously saved email is offered during initial setup. Apple Mail needs a configured sending account and macOS permission to automate Mail. Failed submissions are queued on disk and retried while the app is open. A successful Mail submission means the message was handed to the local sending account.

Choose **RT60** in a plot selector and **Measure RT60** to graph a selected impulse response’s broadband decay. The app fits −5 to −25 dB and extrapolates to 60 dB. Short, noisy or poorly fitted decays show a diagnostic.

Click a **Numeric** meter to edit its yellow and red thresholds in the bottom properties strip. Green covers values below yellow, yellow starts at the first threshold, and red starts at the second. Use OSM’s built-in Numeric Leq mode for equivalent level measurements.

## Build on macOS

Install Apple Command Line Tools and Qt 5.15 with Qt Quick Controls 1 and 2, qmake, and macdeployqt on PATH. Run `scripts/package-mac.sh`. The resulting app bundle and zip are in dist/. This build targets the local CPU architecture and uses ad-hoc signing for local testing. Public release signing, notarization, and testing on a calibrated audio interface are pending.
