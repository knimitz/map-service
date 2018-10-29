# Information

## This is a sample application for homescreen-2017 and windowmanager-2017.

## How to compile and install

- $ mkdir build
- $ cd build
- $ cmake ..
- $ make
- $ make widget

- Copy package/simple-egl.wgt to rootfs.

## How to use

- afm-util install simple-egl.wgt
- afm-util start simple-egl@0.1

## Depends

- homescreen-2017
- agl-service-homescreen-2017
- agl-service-windowmanger-2017
- libhomescreen
- libwindowmanager
- wayland-ivi-extension
