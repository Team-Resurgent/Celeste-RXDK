# Celeste-RXDK

[![CMake](https://github.com/ngagesdk/ccleste/actions/workflows/cmake.yml/badge.svg)](https://github.com/ngagesdk/ccleste/actions/workflows/cmake.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/5216ba1db09c4dccb561018156e1e4b1)](https://www.codacy.com/gh/ngagesdk/ccleste/dashboard?utm_source=github.com&utm_medium=referral&utm_content=ngagesdk/ccleste&utm_campaign=Badge_Grade)

[![Celeste 1](https://raw.githubusercontent.com/ngagesdk/ccleste/master/media/screenshot-01-tn.png)](https://raw.githubusercontent.com/ngagesdk/ccleste/master/media/screenshot-01.png?raw=true "Celeste 1")
[![Celeste 2](https://raw.githubusercontent.com/ngagesdk/ccleste/master/media/screenshot-02-tn.png)](https://raw.githubusercontent.com/ngagesdk/ccleste/master/media/screenshot-02.png?raw=true "Celeste 2")

# About the project

- A port of Celeste Classic C source to Original Xbox using RXDK
- This port was made by user HCF over on the Xbox-Scene Discord & EmuXtras Forums 

## Original Xbox Controls

- D-pad or Left Analog Stick - Movement
- A Button - Jump
- B Button - Dash
- Start Button - Pause
- Left Shoulder Button- Save State
- Right Shoulder Button - Load State
- Start + Back Button - Exit Game


## Build Original Xbox Binaries / xISO

This project has been **ported using RXDK**. Please follow the updated setup steps below to build the Original Xbox binaries or xISO image.

---

### 1. Install RXDK

You must first set up **RXDK** (Resurgent Xbox Development Kit).  
Follow the installation guide on the official repository:

🔗 [Team Resurgent RXDK](https://github.com/Team-Resurgent/RXDK)

---

### 2. RXDK SDL2x Port

This project also uses the **RXDK Port of SDL2x**.  These have been included in this repo

🔗 [RXDK-SDL2x](https://github.com/Team-Resurgent/RXDK-SDL2x)

---


# Original description

This C source port of the [original
Celeste](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8 was
originally developed by
[lemon32767](https://github.com/lemon32767/ccleste).

In the original version, attention was paid to portability, but there
was still some need for improvement. Especially since I really wanted
to get the game to run on the **Nokia N-Gage** and was stuck with a very
old compiler.

This fork aims to be more robust and portable than the original and
should, at least in theory, be compilable from all platforms officially
supported by SDL 2.

It has been confirmed to work on the following platforms:

- Linux
- Nintendo 3DS
- Nokia N-Gage
- Windows
- Playstation Portable

## Compiling

### Windows

The easiest way to get ccleste up and running is Visual Studio 2022 with
[C++ CMake tools for
Windows](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)
installed. Simply open the cloned repository via `File -> Open ->
Folder`. Everything else is set up automatically and all required
dependencies are fetched at compile time.

### Nokia N-Gage

The procedure for the Nokia N-Gage is the same as for Windows. You only
need to install and set up the [N-Gage
SDK.](https://github.com/ngagesdk/ngage-toolchain) in advance and select
the respective build configuration in Visual Studio.

Please note: In order for the SDL executable to be executed properly by
the launcher, the path in the file
[ngage_appui.cpp](src/ngage_appui.cpp#L35) file must be adjusted
accordingly. Unfortunately, I have not yet found a way to configure
this dynamically.

In addition to the project configuration, the application launcher also
needs to be customised. The associated files are located in the
sub-folder [res](res/).

### Linux

ccleste can also be compiled on Linux with the included CMake
configuration.

```bash
mkdir build
cd build
cmake ..
make
```

### Nintendo 3DS

You can find the homebrew toolchain [here](https://devkitpro.org/).

Currently there are no binary packages of SDL2 and SDL2_mixer, as such
you need to compile both from source.

Note that you need to use the `TREMOR` backend for SDL2_mixer, as using
the `STB` backend will cause a crash about 1/3rd into the game. There
currently are issues with linking the `TREMOR` backend which will
require you to manually link `libogg` to the right of `libvorbisidec`
(SDL2_mixer 2.6.2).

You can then compile a `.3dsx` executable with the following commands

```bash
cmake -S. -Bbuild -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCELESTE_P8_ENABLE_AUDIO=ON
cmake --build build
```

### Playstation Portable

ccleste can also be compiled for the PSP with the included CMake
configuration and the [PSPDEV](https://pspdev.github.io/) toolchain.

```bash
mkdir build
cd build
psp-cmake ..
make
```

It can also be build with audio support by adding the `-DCELESTE_P8_ENABLE_AUDIO=ON` to the psp-cmake command, but the game runs at slightly above half speed with it.

## Controls

|   PC    |     N-Gage      |             Action |
| :-----: | :-------------: | -----------------: |
|  LEFT   |      LEFT       |          Move left |
|  RIGHT  |      RIGHT      |         Move right |
|  DOWN   |      DOWN       |          Look down |
|   UP    |       UP        |            Look up |
|   Z/C   |        7        |               Jump |
|   X/V   |        5        |               Dash |
| ESCAPE  | Softkey (right) |              Pause |
|    E    |        3        | Toggle screenshake |
| SHIFT+D |        2        |         Load state |
| SHIFT+S |        1        |         Save state |
| Hold F9 |     Hold C      |              Reset |
|   F11   |                 |         Fullscreen |
| Delete  | Softkey (left)  |               Quit |

## Credits

Sound wave files are taken from the
[Celeste-Classic-GBA](https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data)
project, music _.ogg files were obtained by converting the _.wav dumps
from PICO-8, which I did using audacity & ffmpeg.

The frame for the N-Gage version is based on [Mountain at Dusk
Background](https://opengameart.org/content/mountain-at-dusk-background)
by ansimuz.

All credit for the original game goes to the original developers (Maddy
Thorson & Noel Berry).