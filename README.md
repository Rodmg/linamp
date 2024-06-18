# Linamp player

Music player app for Linamp - Your favorite music player of the 90s, but in real life.

## Requirements

- Qt 6 with Qt Multimedia
- taglib 1.11.1 or newer
- libasound2-dev
- libpipewire-0.3-dev
- vlc
- libiso9660-dev
- libcdio-dev
- libcdio-utils
- swig
- python3-pip
- python3-full
- python3-dev
- libdiscid0

## Development

### Setup

The exact steps you need to take to setup your dev environment will depend on your OS. Currently I'm using Debian Bookworm as my dev environment as it's very close to the target device environment (Raspberry Pi with DietPi, which is based on Debian Bookworm).

The instructions here are for Debian Bookworm:

```bash
# Install Qt, Qt Multimedia and Qt Creator
sudo apt-get install build-essential qt6-base-dev qt6-base-dev-tools qt6-multimedia-dev qtcreator -y

# Install third party library dependencies
sudo apt-get install libtag1-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev -y

# Install dependencies for Python (for CD player functionality)
sudo apt-get install vlc libiso9660-dev libcdio-dev libcdio-utils swig python3-pip python3-full python3-dev libdiscid0 -y

# Create Python venv and install Python dependencies (for CD player functionality)
## IMPORTANT: Make sure you are on the folder where you cloned this repo before running the following commands:
python3 -m venv venv
source venv/bin/activate
pip install -r python/requirements.txt
```

### Build and run

1. Open this project with Qt Creator (open player.pro in Qt Creator)
2. Qt Creator should guide you setting up your paths and settings for building the proyect in your machine
3. You should be able to click the green "Play" icon in Qt Creator to build and run the app

**Tip:** If you want to see the app in a window instead of full screen, comment out the following line in `main.cpp`: `//window.setWindowState(Qt::WindowFullScreen);`

### Building a Debian package

Install [sbuild](https://wiki.debian.org/sbuild) using option 3 (change `unstable` to your distro, eg `bookworm` and create the `.sbuildrc` manually instead of `tee`). 

Then simply run `sbuild --no-run-piuparts --lintian-opt="--suppress-tags=bad-distribution-in-changes-file"` in the cloned repository directory. The `.deb` packages will be in your home directory afterwards if everything went well. 

### Known issues

- File picker and playlist view doesn't correctly work with mouse input, clicks are not detected (touch works fine). There was a bug with touch input which got fixed by disabling certain mouse events, but this had the side effect that if you don't have a touch screen, you cannot quite use it with a mouse. WORKAROUND: Whenever you want to click something inside the file browser or playlist, click and hold for about one second, this will trigger the click event correctly.
- In order for the Python integration (and thus, the CD player functionality) to work and not crash, you need to run the app inside the Python venv, you can use the "start.sh" script as a helper to set everything up for you and run it. However, because Qt Creator by default will directly run the player executable outside of the venv, the cd player will crash. I'm yet to find a better way of running it inside Qt Creator, other than telling it to run the "start.sh" script instead of the executable.

### Debugging memory leaks:

- Install valgrind
- In Qt Creator, in the menu bar, click Analyze -> Valgrind Memory Analizer
- Wait for the app to start (it will be slow), use it and close it, you will get the results then.

### Links that I've found useful and/or got inspiration from:

- https://taglib.org/
- https://github.com/Znurre/QtMixer/blob/b222c49c8f202981e5104bd65c8bf49e73b229c1/QAudioDecoderStream.cpp#L153
- https://github.com/audacious-media-player/audacious/blob/master/src/libaudcore/visualization.cc#L110
- https://github.com/audacious-media-player/audacious/blob/master/src/libaudcore/fft.cc
- https://github.com/qt/qtmultimedia/blob/4fafcd6d2c164472ce63d5f09614b7e073c74bea/src/spatialaudio/qaudioengine.cpp
- https://github.com/audacious-media-player/audacious-plugins/blob/master/src/qt-spectrum/qt-spectrum.cc
- https://github.com/captbaritone/webamp/blob/master/packages/webamp/js/components/Visualizer.tsx
