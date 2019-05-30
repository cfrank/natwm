#### Installing XCB on OSX:

First you have to install `XQuartz` from [https://www.xquartz.org/](https://www.xquartz.org/)

The following are required to successfully compile xcb from source

- `make`
- `automake`
- `libtool`
- `pkg-conf`

Then you need to follow the developers guide for `xcb` from here: [https://xcb.freedesktop.org/DevelopersGuide/](https://xcb.freedesktop.org/DevelopersGuide/)

Make sure to read the note about explicitly setting ACLOCAL before running autogen

Here is the order which worked for me:

- `xorg/util/macros`
- `xorg/proto/xorgproto`
- `xorg/lib/libXau`
- `xcb/pthread-stubs`
- `xcb/proto`
- `xcb/libxcb`
- `xcb/util`
- `xcb/util-keysyms`

If you need X11 libs as well install the following:
- `xorg/lib/libxtrans`
- `xorg/lib/libX11`

If you don't have `xcb-xrm` follow all the steps above then clone it and install:

- `git clone --recursive https://github.com/Airblader/xcb-util-xrm.git`
