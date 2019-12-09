# natwm [![Build Status](https://travis-ci.com/cfrank/natwm.svg?branch=dev)](https://travis-ci.com/cfrank/natwm) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/cfrank/natwm.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/cfrank/natwm/context:cpp)
The purpose of this is to create a tiling window manager which one developer would be able to wrap their head around.

## Installation
Since this project is in constant development the only way to install is by building from source. This process should not be too difficult since this is a small project.

The first step is to clone the project including submodules used for logging and testing.

```
git clone --recursive git@github.com:cfrank/natwm.git && cd natwm
```

From there we need to build the project

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
./bin/natwm
```

The default configuration file location is `~/.config/natmw/config` - A base configuration and information on setting up natwm will be coming as the project progresses. For now an empty file is enough.

### Additional installation instructions

You may want to build the project with debug symbols for easier contributing. This can be done by running `cmake` like so:

`cmake -DCMAKE_BUILD_TYPE=Debug ../`

Also if you want to build `natwm` with it's unit tests you can by doing:

`cmake -DENABLE_TESTING=On ../`

### Troubleshooting

#### No such file or direction
If you recieve any errors that resemble the following:

`fatal error: xcb/xcb_ewmh.h: No such file or directory`

This is letting you know that you are missing a xcb libary. You can usually find the resolution to this error with a quick online search.

For example the solution to the above error (On Arch Linux) is:

`sudo pacman -S xcb-util-wm`

#### Failed to find configuration file at...
If you receive the following error:

`[NATWM:ERROR] - Failed to find configuration file at /home/<username>/.config/natwm/config`

You need to create the following file: `/home/<YOUR_USERNAME>/.config/natwm/config`

*This requirement will most likely be removed in the future. But for now it's required*


## Project Goals

The goals of the project are:

- Use up to date XCB
- Correctly implement Extended Window Manager Hints (EWMH)
- Provide a good starting configuration
- Use minimial system resources
- Be understandable to anyone who has a understanding of the C programming language
- Provide complete documentation
- More...
