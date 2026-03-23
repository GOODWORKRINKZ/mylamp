#include <Arduino.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "effects/EffectContext.h"
#include "effects/SolidColorEffect.h"

namespace {

lamp::MatrixLayout g_layout;
lamp::FrameBuffer g_frameBuffer(g_layout);
lamp::effects::SolidColorEffect g_bootEffect(lamp::Rgb{0, 0, 24});
unsigned long g_lastHeartbeatMs = 0;

void printBootBanner() {
  Serial.println();
  Serial.println("mylamp bootstrap");
  Serial.print("board: ");
  Serial.println(lamp::BuildInfo::board);
  Serial.print("version: ");
  Serial.println(lamp::BuildInfo::version);
  Serial.print("channel: ");
  Serial.println(lamp::BuildInfo::channel);
  Serial.print("logical canvas: ");
  Serial.print(lamp::config::kLogicalWidth);
  Serial.print("x");
  Serial.println(lamp::config::kLogicalHeight);
  Serial.print("sample pixel map x=0 y=0 -> ");
  Serial.println(g_layout.toLinearIndex(0, 0));
  Serial.print("sample pixel map x=31 y=15 -> ");
  Serial.println(g_layout.toLinearIndex(31, 15));
  const lamp::Rgb bootPixel = g_frameBuffer.pixelAtIndex(g_layout.toLinearIndex(0, 0));
  Serial.print("boot pixel r=");
  Serial.print(bootPixel.r);
  Serial.print(" g=");
  Serial.print(bootPixel.g);
  Serial.print(" b=");
  Serial.println(bootPixel.b);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  lamp::effects::EffectContext context{0, g_frameBuffer};
  g_bootEffect.render(context);
  printBootBanner();
}

void loop() {
  const unsigned long now = millis();
  if (now - g_lastHeartbeatMs >= 5000UL) {
    g_lastHeartbeatMs = now;
    lamp::effects::EffectContext context{now, g_frameBuffer};
    g_bootEffect.render(context);
    Serial.print("heartbeat uptime_ms=");
    Serial.println(now);
  }
}
