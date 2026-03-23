# Architecture

## Device target

- MCU: ESP32-C3 Super Mini
- LEDs: 2x WS2812B 16x16
- Topology: cylinder assembled from two logical halves
- Sensors: AHT30 over I2C
- Connectivity: WiFi only

## Rendering model

The lamp uses a logical 32x16 canvas. Effects render against this logical space and do not need to know about physical panel boundaries.

Responsibilities by layer:
- `MatrixLayout`: maps logical coordinates to physical LED indices
- future `FrameBuffer`: stores logical pixel state
- future `EffectEngine`: renders animations into the frame buffer
- future `OverlayRenderer`: draws time and sensor data on top of the active effect

## Current mapping assumptions

- panel 0 occupies logical X range `0..15`
- panel 1 occupies logical X range `16..31`
- both panels currently assume serpentine rows
- `wrapX()` makes cylinder-friendly horizontal addressing possible

These assumptions are intentionally centralized in `MatrixLayout`, so hardware inversion or panel rotation can be corrected without rewriting effects.

## Planned runtime services

- WiFi manager with AP fallback
- settings storage backed by Preferences/NVS
- web control surface for effect parameters and telemetry
- NTP time synchronization for clock overlays
- sensor polling service for AHT30
- OTA updater with stable/dev channels

## Release engineering direction

This project will mirror the generic firmware delivery approach used in microbbox:
- GitHub Releases as OTA source
- separate `stable` and `dev` channels
- draft release first, publish only after assets are attached
- version/channel embedded into firmware via build flags
