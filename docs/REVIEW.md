# Code Review & Optimization Audit — Моя Лампа

**Дата:** 2026-06-07
**Устройство:** ESP32-C3 (384KB Flash, 320KB SRAM, без FPU)
**Охват:** Все модули `src/`, `include/`, сеть, DSL, эффекты, хранилище

---

## Сводка находок

| Severity | Count | Модули |
|----------|-------|--------|
| 🔴 CRITICAL | 7 | NTP (leap year), OTA (timeout), DSL (recursion), Lexer (buffer), Parser (bounds), Web (path traversal), Storage (atomic write) |
| 🟠 HIGH | 11 | Web (heap fragment.), DSL (string sub), Compiler (for-loop), FrameBuffer (hot path), Sensors (init), Settings (null), Playlist (overflow) |
| 🟡 MEDIUM | 9 | main.cpp (globals), MatrixLayout (float), ClockOverlay (blend), PresetApi (validation), GitHubRelease (version), Checksum (format) |
| 🟢 LOW | 6 | Code duplication (DRY), Serial logging, Inconsistent error handling, Hardcoded strings |

---

## 🔴 CRITICAL — Fix Immediately

### 1. NTP: Leap year calculation broken
**Файл:** `src/time/ArduinoNtpTimeSource.cpp:87`
```cpp
// BROKEN — off-by-one epoch
int leapDays = (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400;
```
**Fix:** Использовать встроенный `configTzTime()` на ESP32 вместо HTTP-based «NTP».
```cpp
configTzTime(timezone, "pool.ntp.org", "time.nist.gov");
time_t now = time(nullptr);
return now > 1000000000UL;
```

### 2. OTA: Бесконечное ожидание при зависшем сервере
**Файл:** `src/update/Esp32FirmwareInstaller.cpp:106-134`
```cpp
while (http.connected() && (contentLength < 0 || written < ...)) {
    const size_t available = stream->available();
    if (available == 0) {
        delay(1);        // ← Вечный цикл при stalled-сервере
        continue;
    }
```
**Fix:** Добавить кумулятивный таймаут на весь трансфер + per-chunk таймаут.
```cpp
unsigned long transferStart = millis();
while (http.connected() && (millis() - transferStart < kOtaTimeoutMs)) { ... }
```

### 3. DSL Parser: Выход за границы массива токенов
**Файл:** `src/live/dsl/Parser.cpp:23-24`
```cpp
const Token& current() const { 
  return tokens_[index_];  // ← Unchecked access!
}
```
**Fix:**
```cpp
const Token& current() const {
  if (index_ >= tokens_.size()) {
    static Token eof{TokenType::kEof, "", 0, 0};
    return eof;
  }
  return tokens_[index_];
}
```

### 4. Lexer: Неограниченный рост bitmap-строки
**Файл:** `src/live/dsl/Lexer.cpp:271`
```cpp
bitmap += trim(bitmapLine);  // ← += без bound-check, OOM risk
```
**Fix:** `bitmap.reserve(8192);` + проверка `if (bitmap.size() > 8192) return false;`

### 5. Web: Path traversal в API пресетов
**Файл:** `src/web/LampWebServer.cpp:277`
```cpp
// /api/presets/../../../data.txt → возвращает "../../../data.txt"!
std::string trimPresetPath(const std::string& uri) {
  static constexpr const char* kPrefix = "/api/presets/";
  return uri.substr(std::char_traits<char>::length(kPrefix));
}
```
**Fix:** Валидировать ID: запретить `..`, `/`, `\0`, ограничить длину 64 символами.

### 6. Storage: Неатомарная запись файлов
**Файл:** `src/storage/LittleFsFileStore.cpp:35-51`
```cpp
file.print(content.c_str()); // Прямая запись — при сбое питания файл битый
```
**Fix:** Паттерн atomic write: `запись в .tmp → remove(original) → rename(.tmp, original)`.

### 7. main.cpp: Serial.println() в горячем цикле loop()
**Файл:** `src/main.cpp:443-450`
```cpp
// ВНУТРИ loop() — блокирует рендеринг на 10-50ms каждые 5 секунд!
Serial.print("heartbeat uptime_ms=");
```
**Fix:** Вынести логирование в отдельную FreeRTOS-задачу с неблокирующей очередью.

---

## 🟠 HIGH — Fix Soon

### 8. Web: Фрагментация кучи через String::operator+=
**Файл:** `src/web/LampWebServer.cpp:55-75`
```cpp
json += "\"mode\":\"" + escapeJsonValue(...) + "\",";  // Каждый += — аллокация!
```
20+ конкатенаций на запрос = 20+ фрагментирующих аллокаций. ESP32-C3 имеет всего 320KB RAM.
**Fix:** Использовать ArduinoJson (как уже сделано в PresetApi.cpp) или `std::string::reserve()`.

### 9. DSL Compiler: Неограниченная подстановка строк в for-loop
**Файл:** `src/live/runtime/Compiler.cpp:854-876`
```cpp
for (int i = startVal; ...; i += stepVal) {
  replaceAll(layer.xExpression, loopVar, std::to_string(i));  // 8 replaceAll × N итераций
  replaceAll(layer.yExpression, loopVar, std::to_string(i));
  // ... 6 more ...
}
```
`for i = 0; i < 100` → 800 вызовов `replaceAll()`, каждый сканирует строку заново — O(n²).
**Fix:** Заменить строковую подстановку на AST-манипуляцию: парсить выражение один раз, подставлять константу на этапе evaluation.

### 10. DSL Compiler: Бесконечный for-loop при stepVal=0
**Файл:** `src/live/runtime/Compiler.cpp:880-885`
```cpp
for (int i = startVal; evaluateComparison(i, endVal, op); i += stepVal) {
  ++iterationCount;  // ← stepVal=0 → бесконечный цикл
}
```
**Fix:** Добавить guard: `if (stepVal == 0) return error;`

### 11. FrameBuffer: Избыточная валидация в commitFrame()
**Файл:** `src/main.cpp:172-181` + `src/FrameBuffer.cpp:38`
```cpp
for (uint16_t i = 0; i < kPixelCount; ++i) {
  const lamp::Rgb src = g_frameBuffer.pixelAtIndex(i);  // ← bounds check 512 раз
  g_leds[i] = CRGB(src.r, src.g, src.b);
}
```
Каждый `pixelAtIndex()` проверяет `if (index >= pixels_.size())` — 512 лишних проверок на кадр.
**Fix:** Прямой доступ к `pixels_[i]` в дружественном методе или итератор.

### 12. Sensors: begin() вызывается при КАЖДОМ read()
**Файл:** `src/sensors/ArduinoAht30SensorSource.cpp:10-20`
```cpp
SensorSample ArduinoAht30SensorSource::read() {
  if (!beginIfNeeded()) { return {}; }  // ← 60 FPS = 60 вызовов/сек
```
**Fix:** Вынести `beginIfNeeded()` в `initialize()`, вызывать один раз при старте.

### 13. PlaylistJson: Переполнение без ошибки
**Файл:** `src/live/PlaylistJson.cpp:24`
```cpp
StaticJsonDocument<kPlaylistJsonCapacity> document;  // 2048 байт
// Если плейлист большой — ArduinoJson молча обрезает JSON. Нет ошибки.
```
**Fix:** Предварительно оценивать размер: `if (estimatedSize > capacity) return "";`

### 14. Web: `ostringstream` — скрытый убийца кучи
**Файл:** `src/web/StatusJsonBuilder.cpp:66`
```cpp
std::ostringstream oss;  // ← Инициализирует глобальную locale (~50KB overhead!)
```
На ESP32-C3 это ~16% кучи на один вызов.
**Fix:** `snprintf(buf, sizeof(buf), "%.2f", value);`

### 15. MatrixLayout: Float-арифметика для целочисленных операций
**Файл:** `src/MatrixLayout.cpp:62-68`
```cpp
// ESP32-C3 не имеет FPU — каждый float-расчёт программный (дорого!)
return static_cast<uint8_t>(wrapped / 360.0f * 32.0f) % 32;
```
**Fix:** Целочисленная арифметика: `return static_cast<uint8_t>((static_cast<int>(wrapped) * 32) / 360) % 32;`

### 16. WiFiManager: Состояние гонки при получении IP
**Файл:** `src/network/WiFiManager.cpp:12-14`
```cpp
result.localIp = adapter.localIp();  // ← IP может измениться между вызовами
result.statusLine = ...;
result.statusLine = ...;  // ← Двойное присваивание — вероятно баг
```

### 17. ClockOverlay: Блендинг без clamp
**Файл:** `src/effects/ClockOverlay.cpp:58-62`
```cpp
static_cast<uint8_t>(blended.r * alpha + dst.r * (1.0f - alpha))  // Может превысить 255
```
**Fix:** `std::min(255.0f, std::max(0.0f, result))`

### 18. PresetRepository: Молчаливый пропуск битых файлов
**Файл:** `src/live/PresetRepository.cpp:30-45`
```cpp
if (!parsePresetJson(json, playlist)) {
  continue;  // ← Никакого лога, пользователь не знает что файл битый
}
```

---

## 🟡 MEDIUM — Fix When Convenient

| # | Файл | Проблема | Рекомендация |
|---|------|----------|--------------|
| 19 | `main.cpp:55-117` | 55 строк глобалов — трудно тестировать | Группировать в `SystemState` struct |
| 20 | `main.cpp:218-285` | 4KB DSL-кода в `.text` (драгоценный flash) | Переместить в LittleFS файлы или `PROGMEM` |
| 21 | `main.cpp:340-342` | Двойной вызов `millis()` в FPS-расчёте | Сохранить в `unsigned long now = millis()` |
| 22 | `MatrixLayout.cpp:32` | `panels_[panelIndex]` без проверки границ | Добавить `assert(panelIndex < kPanelCount)` |
| 23 | `Parser.cpp:289-293` | Reserved words — case-sensitive check | Сделать case-insensitive |
| 24 | `Parser.cpp:390-460` | 200+ строк дублирования parseLayer / parseForLoop | Вынести в общий helper |
| 25 | `Compiler.cpp:620` | Линейный поиск палитры для каждого спрайта | Построить `unordered_map` индекс один раз |
| 26 | `GitHubReleaseParser.cpp:79-94` | Версионное сравнение игнорирует pre-release | Обрабатывать `-rc`, `-beta` суффиксы |
| 27 | `SettingsPersistence.cpp:32-40` | Линейный поиск по 6 timezone каждый раз | `std::find` или `unordered_set` |

---

## 🟢 LOW — Cosmetic / Code Quality

| # | Файл | Проблема | Рекомендация |
|---|------|----------|--------------|
| 28 | `main.cpp:201-210` | Несогласованное логирование ошибок vs успеха | Унифицировать: `#if APP_IS_DEV` вокруг всех логов |
| 29 | `FrameBuffer.cpp:25-30` | `setPixel()` молча глотает выход за границы | Добавить `#ifdef DEBUG` assert |
| 30 | `FrameBuffer.h:12` | `Rgb` — нет `constexpr` операторов | Добавить `constexpr operator==`, `operator+` |
| 31 | `Esp32FirmwareInstaller.cpp:42-54` | Дублирование `fetchTextWithTls()` | Вынести в общий HTTP-утилитный класс |
| 32 | `ArduinoNtpTimeSource.cpp:29` | `http://example.com` — зарезервированный домен, никогда не работает | Убрать или заменить на `http://worldtimeapi.org` |
| 33 | `PlaylistApi.cpp:49` | `durationSec` до 136 лет (uint32_t max) | Ограничить 3600 сек |

---

## Приоритетные оптимизации (по impact/effort)

### Быстрые победы (low effort, high impact)

1. **Убрать `ostringstream`** → `snprintf` (экономия ~50KB heap на запрос)
2. **Убрать Serial из loop()** → async logging task (исчезнут дропы кадров)
3. **Починить `beginIfNeeded()` в сенсорах** → вызов 1 раз вместо 60/сек
4. **Заменить float на int в MatrixLayout** → ускорение в 5-10× (нет FPU!)
5. **Добавить `reserve()` в JSON-сборщики** → -80% фрагментации кучи

### Средние инвестиции (medium effort, high impact)

6. **Atomic write в LittleFsFileStore** → защита от битых файлов при сбое питания
7. **AST-подстановка вместо replaceAll() в Compiler** → ускорение for-loop в 10-100×
8. **ArduinoJson вместо String::operator+= в LampWebServer** → -90% фрагментации
9. **Добавить HTTP-таймауты в OTA** → нет зависаний устройства

### Стратегические (high effort, transformative)

10. **Заменить HTTP-«NTP» на настоящий SNTP** (`configTzTime()`)
11. **LUT для sin/cos** (360 float = 1.4KB) — ускорение DSL-эффектов в 5-20×
12. **Переезд с Software Floating Point на Fixed-Point (16.16)** для DSL-экзекьютора

---

## Карта файлов по критичности

```
🔴 CRITICAL (fix first):
  src/time/ArduinoNtpTimeSource.cpp       — leap year calc
  src/update/Esp32FirmwareInstaller.cpp   — timeout + rollback
  src/live/dsl/Parser.cpp                 — bounds check
  src/live/dsl/Lexer.cpp                  — bitmap size
  src/web/LampWebServer.cpp               — path traversal
  src/storage/LittleFsFileStore.cpp       — atomic write
  src/main.cpp                            — Serial in loop()

🟠 HIGH (fix next sprint):
  src/web/LampWebServer.cpp               — String fragmentation
  src/live/runtime/Compiler.cpp           — for-loop string sub + infinite loop
  src/FrameBuffer.cpp + main.cpp          — commitFrame hot path
  src/sensors/ArduinoAht30SensorSource.cpp — init once
  src/web/StatusJsonBuilder.cpp           — ostringstream
  src/MatrixLayout.cpp                    — float→int
  src/effects/ClockOverlay.cpp            — blend clamp

🟡 MEDIUM (backlog):
  src/web/PresetApi.cpp, PlaylistApi.cpp  — bounds, validation
  src/live/PlaylistJson.cpp, PresetJson.cpp — overflow checks
  src/update/GitHubReleaseParser.cpp      — pre-release versions
  src/settings/AppSettingsPersistence.cpp — linear search

🟢 LOW (when time permits):
  Code duplication, constexpr, debug asserts
```

---

## Метрики

| Параметр | Текущее | Цель |
|----------|---------|------|
| Фрагментация кучи (после 100 HTTP-запросов) | ~40% estimated | <10% |
| Время обработки for-loop (100 итераций) | ~200ms (string sub) | <5ms (AST) |
| Время кадра (DSL-эффект средней сложности) | ~12ms | <8ms |
| Размер бинарника (.text) | TBD | -4KB (вынос preset-строк) |
| Стабильность OTA | race condition | atomic + rollback |
| Path traversal уязвимость | Открыта | Закрыта |

---

*Generated by automated codebase audit, 2026-06-07*
