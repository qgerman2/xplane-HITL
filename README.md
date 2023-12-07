# X-Plane HITL Plug-in

This plug-in for X-Plane enables 'Hardware-in-the-Loop' testing with an autopilot running ArduPilot with a custom firmware.

### Features

- [x] Send plane state as telemetry from sensors.
- [x] Calibrate a plane sensors just as it's done with a real plane model.
- [x] Use any feature from Mission Planner as if it was working with a real plane model.
- [ ] Simulate sensor accuracy errors and interruptions.

It has been tested on X-Plane 9 and X-Plane 11, both 32 and 64 bit versions are provided as binaries.

## Setup

### 1. Download

Grab the [latest release](https://github.com/qgerman2/xplane-HITL/releases) and install it in the ```resources/plugins``` folder of X-Plane.

### 2. Configure an autopilot

Follow the instructions [here](https://github.com/qgerman2/ardupilot-HITL/blob/HITL/README.md) to flash an autopilot with the custom firmware required for the plug-in to work.

## Building the plug-in

The plug-in has been written in Visual Studio Code and compiled with the latest MSVC compiler, the tasks.json file contains the compiler parameters necessary and all dependencies are already included in the repository.

## Images

![RC Plane in X-Plane 9](setup-instructions/xplane-rc.png)
![Desk setup with RC controller](setup-instructions/setup.jpeg)
