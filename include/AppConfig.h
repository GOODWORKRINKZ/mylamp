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
static constexpr uint16_t kI2cTimeoutMs = 100;

static constexpr uint8_t kDefaultBrightness = 32;
static constexpr uint8_t kMaxBrightness = 96;

static constexpr char kAccessPointPrefix[] = "MYLAMP";
static constexpr char kAccessPointPassword[] = "12345678";
static constexpr char kTimeZone[] = "UTC0";
static constexpr char kNtpPrimaryServer[] = "pool.ntp.org";
static constexpr char kNtpSecondaryServer[] = "time.nist.gov";

static constexpr uint32_t kWiFiConnectTimeoutMs = 10000;
static constexpr uint16_t kWiFiPollIntervalMs = 250;
static constexpr uint32_t kTimeRefreshIntervalMs = 30000;
static constexpr uint32_t kSensorRefreshIntervalMs = 5000;
static constexpr uint8_t kSensorStaleReadLimit = 12;

}  // namespace lamp::config
