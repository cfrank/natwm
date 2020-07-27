<h1 align="center">natwm</h1>
<h4 align="center">Not a Tiling Window Manager</h4>

<div align="center">

[![Build Status](https://travis-ci.com/cfrank/natwm.svg?branch=dev)](https://travis-ci.com/cfrank/natwm) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/cfrank/natwm.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/cfrank/natwm/context:cpp)

</div>

<p align="center">
  <a href="#about">About</a> •
  <a href="#features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#installation">Configuration</a> •
  <a href="#keybinds">Keybinds</a> •
  <a href="#thanks">Thanks</a> •
  <a href="#license">License</a>
</p>

---

## About

natwm is a simple non-reparenting floating window manager for the X Window System build on top of XCB. I set out to create something which would fulfill my needs for a window manager, while also being an example for others who wish to pursue the same. The code should be easy to follow for anyone familiar with C99 and serves as an example of modern XCB practices.

During development I leaned on countless resources to compensate for the lack of complete XCB/X11 documentation. This codebase is a culmination of all those resources (many of which are listed [below](#thanks))

While the aim was to create something simple, minimalism wasn't a goal, and there are numerous features and quality of life additions which make this more usable as a daily driver.

### Screenshot

<img align="center" src="https://user-images.githubusercontent.com/2308484/88531138-3bf38000-cfb7-11ea-9bdf-086b8bc034ae.png" />

## Features
<div align="center">

|Feature                          | Status |
|---------------------------------|--------|
|Runtime Configuration File       | ✔️ |
|Correct handling of modifier keys| ✔️ |
|Workspaces                       | ✔️ |
|Move+resize w/ mouse             | ✔️ |
|Subset of EWMH                   | ✔️ |
|Multi monitor support            | ✔️ |
|Randr support                    | ✔️ |
|Xinerama support                 | ✔️ |
|Window decorations               | ✔️ |
|And more!                        |   |

</div>

## Installation

_Packages coming soon_

Currently building from source is the main method for installation, and can be completed quite easily on a Linux based system. I have also used OSX for development, but the installation on that platform is much less [straightforward](https://github.com/cfrank/natwm/blob/master/OSX.md)

### Required for building:
- `cmake`
- `xcb`
- `xcb-ewmh`
- `xcb-icccm`
- `xcb-keysyms`
- `xcb-randr`
- `xcb-xinerama`
- `xcb-util`

### Retrieving the sources

The repo contains submodules which are required for building and must be cloned as well:

`git clone --recursive git@github.com:cfrank/natwm.git && cd natwm`

### Installing

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
sudo make install
natwm
```

### Troubleshooting

#### No such file or direction

If you receive any errors that resemble the following:

`fatal error: xcb/xcb_ewmh.h: No such file or directory`

This is letting you know that you are missing an xcb libary. You can usually find the resolution to this error with a quick online search.

For example the solution to the above error (On Arch Linux) is:

`sudo pacman -S xcb-util-wm`

#### Failed to find configuration file at...

If you receive the following error:

`[NATWM:ERROR] - Failed to find configuration file at /home/<username>/.config/natwm/config`

You need to create the following file: `/home/<YOUR_USERNAME>/.config/natwm/config`

*This requirement will most likely be removed in the future. But for now it's required*

## Configuration

The default configuration file location is `$HOME/.config/natwm/natwm.config` but this can be changed by passing the configuration path to the binary like so `natwm -c <path>`

There are a number of different configuration options available to change the look and feel of the window manager.

```
# The configuration file supports variables that contain any of the supported
# data types (boolean, string, number, array)
#
# Variables are denoted by a '$'
$UNFOCUSED_BORDER_COLOR = "#23232a"
$FOCUSED_BORDER_COLOR = "#383851

# Workspaces are the user provided names for each workspace. natwm provides 
# 10 workspaces and by default each workspace is given it's numeric name.
# Using this configuration option the user can specify custom names for
# their workspaces. Each item in the array corresponds to a workspace
#
# Max values: 10
workspaces = [
  "browser",
  "code",
  "terminals",
]

# Monitors can be given offsets which will prevent the window manager from
# taking up space reserved for other programs (for example a status bar)'
#
# The 'monitor.offsets' configuration item allows the user to specify a    
# number of arrays representing offset values for each of the monitors
# connected to the computer
#
# Top		[0]
# Right		[1]
# Bottom	[2]
# Left		[3]
monitor.offsets = [
  [35, 10, 10, 10],
  [10, 10, 10, 10],
]

# During resize a dummy window will be mapped to the page which will
# represent the new size of the window after the resize event is
# complete. Using these configuration options you can customize
# the background color and border color of this dummy window
resize.background_color = "#000000"
resize.border_color = $UNFOCUSED_BORDER_COLOR

# Windows controlled by the window manager can have a border which
# surrounds them. The width of this border can be configured for
# each of the supported window "states"
#
# Unfocused		[0]
# Focused		[1]
# Urgent		[2]*
# Sticky		[3]*
#
# *Currently in development
window.border_width = [
  1,
  2,
  1,
  1,
]

# You can also control the color of the border for each of these
# window "states"
#
# Unfocused		[0]
# Focused		[1]
# Urgent		[2]*
# Sticky		[3]*
#
# *Currently in development
window.border_color = [
  $UNFOCUSED_BORDER_COLOR,
  #FOCUSED_BORDER_COLOR,
  "#cc0000",
  "#ffffff",
]
```

## Keybinds

natwm does not include any native support for keybinds as that is out of the scope of a window manager in my opinion. All keybinds are supported through a combinaiton of [`sxhkd`](https://github.com/baskerville/sxhkd) and [`wmctrl`](https://linux.die.net/man/1/wmctrl)

Here is an example of how I manage my workspaces using the above combination

```
# Workspace switching
alt + {0-9}
    wmctrl -s {0-9}

# Workspace teleporting
alt + shift + {0-9}
    wmctrl -r :ACTIVE: -t {0-9}
```

## Thanks

This project would not have been possible without the numerous existing projects which acted as documentation and examples of best practices. There are many great projects, but these are the ones which I relied on heavily. Sorted alphabetically:

- [2bwm](https://github.com/venam/2bwm)
- [bspwm](https://github.com/baskerville/bspwm)
- [herbstluftwm](https://github.com/herbstluftwm/herbstluftwm)
- [ratpoison](https://www.nongnu.org/ratpoison/)
- [spectrwm](https://github.com/conformal/spectrwm)

## License

![GitHub](https://img.shields.io/github/license/cfrank/natwm)

- Copyright © [cfrank](https://github.com/cfrank)
