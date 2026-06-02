#pragma once

#include <stdint.h>

namespace lamp::config {

// ═══════════════════════════════════════════════════════════════════════════
//  Физическая раскладка LED-панелей
// ═══════════════════════════════════════════════════════════════════════════
//
//  Две панели 16×16, рядом друг с другом, скручены в цилиндр.
//  Панель 0 → первая на линии данных, панель 1 → вторая.
//
//  Логическая система координат (после XY-swap):
//    X = 0..15   — высота цилиндра (вдоль оси), НЕ зациклена
//    Y = 0..31   — окружность цилиндра, ЗАЦИКЛЕНА (y=0 сосед y=31)
//
//  Чтобы изменить раскладку — правь MatrixLayout.cpp (PanelLayout).
//  Чтобы поменять оси местами — правь kLogicalWidth/Height и
//  renderSpritePixel() в Executor.cpp.

static constexpr uint8_t kPanelWidth = 16;
static constexpr uint8_t kPanelHeight = 16;
static constexpr uint8_t kPanelCount = 2;
static constexpr uint8_t kLogicalWidth = kPanelWidth;              // X = высота (16)
static constexpr uint8_t kLogicalHeight = kPanelHeight * kPanelCount;  // Y = окружность (32)
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
static constexpr char kNtpPrimaryServer[] = "0.ru.pool.ntp.org";
static constexpr char kNtpSecondaryServer[] = "1.ru.pool.ntp.org";

static constexpr uint32_t kWiFiConnectTimeoutMs = 10000;
static constexpr uint16_t kWiFiPollIntervalMs = 250;
static constexpr uint32_t kTimeRefreshIntervalMs = 30000;
static constexpr uint32_t kSensorRefreshIntervalMs = 5000;
static constexpr uint8_t kSensorStaleReadLimit = 12;

}  // namespace lamp::config
