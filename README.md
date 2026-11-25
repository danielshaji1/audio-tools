# Audio Tools for macOS

Audio Tools extends [Open Sound Meter](https://github.com/psmokotnin/osm/tree/1e08de2322f5e8849e1ba39e1d3993187a7cc75c) by Pavel Smokotnin and contributors. This independent macOS derivative uses the upstream C++ measurement engine, Qt Quick workspace, Core Audio routing and OpenGL renderer. Original notices are preserved. See LICENSE for the GNU GPL and the notices in fonts/ for font licensing.

## Measure

Select a measurement in the right sidebar to choose the input device, microphone and reference channels. A common clock for both channels is needed for transfer measurements. Import the microphone response file in measurement properties. Use the 94 dB control with a stable 94 dB acoustic calibrator before interpreting readings as absolute SPL. Recalibrate after changing the microphone, preamp gain or channel.

The default workspace stacks magnitude, phase and numerical levels. Each chart has a selector. Choose Spectrum for live RTA and open chart properties to change resolution, scale or display style. Select Generator for output routing, waveform, level and sine frequency. Click a measurement color swatch to choose its trace color.

Enable Target Trace and click its name to edit the native curve. Import additional comparison curves from File → Import using frequency in Hz and level in dB in the first two columns of CSV or tab-separated text. Imported curves appear as named sources. File → Save stores source data and routing in an OSM workspace. File → Recent projects reopens it. Display preferences and the native target editor use application settings.

## Sunday service

Open **Service & room…**, choose an active measurement, enter a name and set the rolling interval and alert levels. Green is below yellow. Recording continues when the service window is hidden. Finish the service before disconnecting the interface.

The app appends timestamped fast and slow A-weighted levels, rolling Leq, and frequency/level pairs to a JSON Lines file once per second. Service and rolling Leq are energy averages of sampled one-second slow readings. Spectrum values are relative digital levels. The summary includes the highest sampled fast reading. After finishing, select an archived service to compose its report in your mail app or reveal the data in Finder. You control sending.

For room decay, obtain a clean transfer-function impulse response with measurement and reference channels. **Estimate RT60** uses a broadband, noise-corrected backward energy integral and extrapolates a −5 to −25 dB fit. It rejects inadequate impulse-to-noise ratio and weak linear fits. The impulse capture must be long enough to include the decay and noise floor.

## Build on macOS

Install Apple Command Line Tools and Qt 5.15 with Qt Quick Controls 1 and 2, qmake, and macdeployqt on PATH. Run `scripts/package-mac.sh`. The resulting app bundle and zip are in dist/. This build targets the local CPU architecture and uses ad-hoc signing for local testing. Public release signing, notarization, and testing on a calibrated audio interface are pending.
