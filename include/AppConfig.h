#pragma once

#include <stdint.h>

namespace lamp::config {

static constexpr uint8_t kPanelWidth = 16;
static constexpr uint8_t kPanelHeight = 16;
static constexpr uint8_t kPanelCount = 2;
static constexpr uint8_t kLogicalWidth = kPanelWidth * kPanelCount;
static constexpr uint8_t kLogicalHeight = kPanelHeight;
static constexpr uint16_t kPixelCount = kLogicalWidth * kLogicalHeight;

static constexpr uint8_t kLedDataPin = 2;
static constexpr uint8_t kI2cSdaPin = 8;
static constexpr uint8_t kI2cSclPin = 9;

static constexpr uint8_t kDefaultBrightness = 32;
static constexpr uint8_t kMaxBrightness = 96;

static constexpr char kAccessPointPrefix[] = "MYLAMP";
static constexpr char kTimeZone[] = "UTC0";

}  // namespace lamp::config
