# Linamp

Music player for embedded project.

## Requirements

- taglib 1.11.1 or newer
- libasound2-dev

```
sudo apt install libtag1-dev libasound2-dev libpulse-dev
```

## Development

### Debugging memory:

- Install valgrind
- In Qt Creator, in the menu bar, click Analyze -> Valgrind Memory Analizer
- Wait for the app to start (it will be slow), use it and close it, you will get the results then.

### Useful links:

- https://taglib.org/
- https://github.com/Znurre/QtMixer/blob/b222c49c8f202981e5104bd65c8bf49e73b229c1/QAudioDecoderStream.cpp#L153
- https://github.com/audacious-media-player/audacious/blob/master/src/libaudcore/visualization.cc#L110
- https://github.com/audacious-media-player/audacious/blob/master/src/libaudcore/fft.cc
- https://github.com/qt/qtmultimedia/blob/4fafcd6d2c164472ce63d5f09614b7e073c74bea/src/spatialaudio/qaudioengine.cpp
- https://github.com/audacious-media-player/audacious-plugins/blob/master/src/qt-spectrum/qt-spectrum.cc
- https://github.com/captbaritone/webamp/blob/master/packages/webamp/js/components/Visualizer.tsx
