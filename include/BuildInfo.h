#pragma once

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-local"
#endif

#ifndef APP_CHANNEL
#define APP_CHANNEL "local"
#endif

#ifndef APP_BOARD
#define APP_BOARD "unknown-board"
#endif

#ifndef APP_HARDWARE_TYPE
#define APP_HARDWARE_TYPE "unknown-hardware"
#endif

namespace lamp {

struct BuildInfo {
  static constexpr const char* projectName = "mylamp";
  static constexpr const char* version = APP_VERSION;
  static constexpr const char* channel = APP_CHANNEL;
  static constexpr const char* board = APP_BOARD;
  static constexpr const char* hardwareType = APP_HARDWARE_TYPE;
};

}  // namespace lamp
