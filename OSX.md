### Installing XCB on OSX:

First you have to install `XQuartz` from [https://www.xquartz.org/](https://www.xquartz.org/)

The following are required to successfully compile xcb from source

- `make`
- `automake`
- `libtool`
- `pkg-conf`

Then you need to follow the developers guide for `xcb` from here: [https://xcb.freedesktop.org/DevelopersGuide/](https://xcb.freedesktop.org/DevelopersGuide/)

Make sure to read the note about explicitly setting ACLOCAL before running autogen

#### Note for users of newer versions of OSX

In a recent update to OSX Apple has made it apparent that they no longer wish to support `/usr/include`. They have decided to move entirely to "SDKs". I do not spend much time developing native applications on OSX so I don't know the reasoning but it will require updating the autotools configuration command to:

`ACLOCAL="aclocal -I $(xcrun --show-sdk-path)/usr/share/aclocal" ./autogen.sh --prefix=$(xcrun --show-sdk-path)/usr`

This will install the dependencies in the correct location for your system

Here is the order which worked for me:

- `xorg/util/macros`
- `xorg/proto/xorgproto`
- `xorg/lib/libXau`
- `xcb/pthread-stubs`
- `xcb/proto`
- `xcb/libxcb`
- `xcb/util`
- `xcb/util-keysyms`
- `xcb/util-wm`

If you need X11 libs as well install the following:
- `xorg/lib/libxtrans`
- `xorg/lib/libX11`
