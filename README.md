# mylamp

Пиксельная лампа на ESP32-C3 Super Mini с двумя матрицами WS2812B 16x16, свернутыми в цилиндр. Проект сочетает firmware на PlatformIO/Arduino, web UI для настройки и editor-first сценарий для live coding эффектов.

## Что умеет проект сейчас

Уже есть:
- firmware-скелет под ESP32-C3 Super Mini;
- логический canvas 32x16 поверх двух панелей 16x16;
- web API статуса, preset-ов и playlist-ов;
- web UI для локальной работы с editor-first UX;
- локальный Vite dev server со стабовым backend без железа;
- базовый DSL-контракт для live coding и заготовки эффектов;
- работа с AHT30, временем и сетевым статусом внутри firmware.

В процессе:
- полноценная device-side реализация `POST /api/live/validate` и `POST /api/live/run`;
- полная сквозная компиляция и запуск DSL на устройстве из web UI;
- довязка UX вокруг preset manager и playlist editor.

Это важное ограничение: синтаксис DSL, UI и dev/mock flow уже есть, но live validate/run на реальном устройстве пока ещё не доведены до конца. README ниже это явно учитывает.

## Железо

Целевая плата:
- ESP32-C3 Super Mini

Периферия:
- 2x WS2812B 16x16
- AHT30 по I2C

Логическая поверхность:
- ширина: `32`
- высота: `16`
- общее число пикселей: `512`

Текущие firmware-константы:
- LED data pin: `GPIO2`
- I2C SDA: `GPIO8`
- I2C SCL: `GPIO9`
- brightness по умолчанию: `32`
- текущий верхний лимит brightness: `96`

## Подключение

Минимальная схема:

1. Обе матрицы WS2812B подключаются к внешнему питанию.
2. `DIN` первой матрицы подключается к `GPIO2` ESP32-C3.
3. Если матрицы соединены последовательно, `DOUT` первой идёт в `DIN` второй.
4. AHT30 подключается по I2C:
	 - `SDA` -> `GPIO8`
	 - `SCL` -> `GPIO9`
5. Земля ESP32, блок питания матриц и датчик должны иметь общую `GND`.

Практические замечания:
- не питай матрицы от USB самой ESP32-C3, используй отдельный блок питания;
- держи яркость ограниченной, особенно на полном белом;
- если матрицы ведут себя нестабильно, сначала проверь общее питание и землю, а не код;
- порядок панелей и serpentine mapping сейчас централизован в `MatrixLayout`, поэтому физические перевороты лучше исправлять в firmware, а не в эффектах.

## Сетевая модель

По умолчанию прошивка умеет работать через Wi-Fi.

Текущие константы по умолчанию:
- AP prefix: `MYLAMP`
- AP password: `12345678`
- timezone: `UTC0`
- NTP servers:
	- `pool.ntp.org`
	- `time.nist.gov`

На практике это означает:
- устройство может поднять собственную точку доступа;
- web UI и API живут на самой лампе;
- синхронизация времени используется для clock/status сценариев;
- статус датчика, сети и времени доступен через `/api/status`.

## Структура репозитория

- `platformio.ini` — PlatformIO environments
- `include/AppConfig.h` — аппаратные и runtime-константы
- `src/main.cpp` — сборка runtime и web services
- `src/web/` — HTTP API и web-server glue
- `src/live/` — live coding runtime, DSL и storage services
- `frontend/` — web UI и local dev tooling
- `resources/dist/` — frontend build для embedding
- `docs/` — архитектура и планы

## Быстрый старт

### 1. Сборка firmware

```bash
pio run -e esp32-c3-supermini-dev
```

Что это делает:
- собирает Arduino firmware для `esp32-c3-devkitm-1`;
- подставляет build info:
	- `APP_VERSION=0.1.0-dev`
	- `APP_CHANNEL=dev`
	- `APP_BOARD=esp32-c3-supermini`
- запускает pre-build script для embedding frontend-ресурсов.

### 2. Прошивка на устройство

Команда зависит от того, какой serial-порт выдаст система. Типовой сценарий:

```bash
pio run -e esp32-c3-supermini-dev -t upload
```

Если нужен serial monitor:

```bash
pio device monitor -b 115200
```

### 3. Локальная frontend-разработка без железа

```bash
cd frontend
npm install
npm run dev
```

Этот режим поднимает Vite dev server и стабовый backend.

Что доступно в dev mode:
- `/api/status`
- `/api/live/validate`
- `/api/live/run`
- `/api/presets/*`
- `/api/playlists/*`
- `/__dev/reset`

Поддерживаемые сценарии:
- `happy-path`
- `autoplay`
- `dsl-error`
- `offline-ish`
- `sensor-missing`

Сценарий можно выбрать:
- через dev-панель в UI;
- через query param, например `http://127.0.0.1:5173/?scenario=autoplay`.

### 4. Frontend production build

```bash
cd frontend
npm run build
```

Результат попадает в `resources/dist/` и затем может быть встроен в firmware.

## Web UI и workflow эффектов

Текущий editor-first сценарий такой:

1. Открываешь web UI.
2. Нажимаешь `Новый эффект` или выбираешь готовую идею справа.
3. Редактируешь DSL-код в textarea-редакторе.
4. Имя эффекта задаёшь в первой строке:

```txt
effect "my_effect"
```

5. Кнопка `Проверить` отправляет код на `POST /api/live/validate`.
6. Кнопка `Запустить` отправляет код на `POST /api/live/run`.
7. Кнопка `Сохранить` сохраняет preset через `PUT /api/presets/:presetId`.

Важно:
- в local dev mode эти запросы работают через mock backend;
- на реальном устройстве preset API уже существует, но live validate/run endpoints пока не доведены до финальной реализации;
- поэтому UI и DSL уже можно использовать как контракт, а device-side runtime ещё дорабатывается.

## DSL для live coding

Язык специально сделан маленьким и предсказуемым, чтобы:
- его было проще учить;
- его было проще валидировать на ESP32-C3;
- LLM не расползались в выдуманный синтаксис.

### Базовые сущности

DSL v1 опирается на три основные конструкции:
- `effect`
- `sprite`
- `layer`

### 1. Effect

Каждый эффект начинается с имени:

```txt
effect "rainbow"
```

Это имя:
- показывает, как называется текущий DSL-эффект;
- используется в UI;
- участвует в сохранении preset-а.

### 2. Sprite

`sprite` описывает форму через bitmap-маску:

```txt
sprite heart {
	bitmap """
	.##.##.
	#######
	#######
	.#####.
	..###..
	...#...
	"""
}
```

Смысл символов:
- `#` — закрашенный пиксель
- `.` — пустой пиксель

### 3. Layer

`layer` выбирает sprite и задаёт его поведение:

```txt
layer love {
	use heart
	color rgb(255, 40, 80)
	x = 10 + sin(t) * 4
	y = 4 + cos(t * 1.2) * 2
	scale = 1
	visible = 1
}
```

Поддерживаемые свойства слоя v1:
- `use`
- `x`
- `y`
- `scale`
- `color`
- `visible`

### Переменные

Доступные переменные:
- `t` — время в секундах с момента старта эффекта
- `dt` — дельта времени между кадрами
- `x`, `y` — текущие координаты пикселя
- `nx`, `ny` — нормализованные координаты от `0` до `1`

### Функции

Поддерживаемые математические функции:
- `sin(value)`
- `cos(value)`
- `abs(value)`
- `min(a, b)`
- `max(a, b)`
- `clamp(v, a, b)`

Поддерживаемые сенсорные функции:
- `temp()`
- `humidity()`

Поддерживаемые цветовые конструкторы:
- `rgb(r, g, b)`
- `hsv(h, s, v)`

### Минимальный шаблон эффекта

```txt
effect "my_effect"

sprite dot {
	bitmap """
	#
	"""
}

layer paint {
	use dot
	color rgb(255, 120, 80)
	x = 10
	y = 6
	scale = 2
	visible = 1
}
```

### Пример эффекта

```txt
effect "heart"

sprite heart {
	bitmap """
	.##.##.
	#######
	#######
	.#####.
	..###..
	...#...
	"""
}

layer love {
	use heart
	color rgb(255, 30 + 20 * sin(t * 5), 80)
	x = 10 + sin(t) * 4
	y = 4 + cos(t * 1.2) * 2
	scale = 1 + abs(sin(t * 2))
	visible = 1
}
```

### Что в DSL v1 не поддерживается

Не надо ожидать здесь полноценный язык программирования. Сейчас вне scope:
- циклы;
- рекурсия;
- пользовательские функции;
- произвольные скрипты;
- вращение sprite;
- blend modes;
- collision logic;
- любые новые ключевые слова, которые не перечислены выше.

## Как создать новый эффект

Текущий путь в UI:

1. Нажми `Новый эффект`.
2. Получи пустую заготовку.
3. Замени `new_effect` на своё имя.
4. Опиши один или несколько `sprite`.
5. Добавь один или несколько `layer`.
6. Нажми `Проверить`.
7. Если всё хорошо, нажми `Сохранить`.

Совет:
- сначала делай один маленький sprite и один layer;
- только потом добавляй второй слой и более сложные формулы;
- так проще и человеку, и LLM.

## Рекомендации по работе с LLM

LLM лучше всего генерирует эффекты, если ты задаёшь жёсткие рамки.

Хороший запрос к модели должен содержать:
- размер полотна: `32x16`;
- стиль эффекта: радуга, огонь, сердечко, молния, часы и так далее;
- ограничения языка: только `effect`, `sprite`, `layer`, `use`, `x`, `y`, `scale`, `color`, `visible`, `rgb`, `hsv`, `sin`, `cos`, `abs`, `min`, `max`, `clamp`, `temp`, `humidity`;
- запрет на выдумывание новых ключевых слов;
- требование вернуть только DSL-код без объяснений.

Если не зафиксировать правила явно, модель часто начинает выдумывать:
- `for`, `if`, `function`, `let`;
- произвольные поля вроде `rotation`, `speed`, `blend`;
- несуществующие функции и новые секции.

Поэтому лучший вариант — давать модели строгий prompt-шаблон.

### Отдельный prompt-файл

Полный copy-paste prompt вынесен в отдельный файл:

- [docs/LLM_EFFECT_PROMPT.md](/home/builder/mylamp/docs/LLM_EFFECT_PROMPT.md)

Так удобнее:
- README остаётся обзорным документом;
- prompt можно быстро открыть и скопировать целиком;
- правила для LLM можно развивать отдельно от общей документации.

### Пример использования

Пример пользовательского запроса:

```text
Сгенерируй эффект для лампы: тёплое сердечко, которое медленно плывёт слева направо, слегка дышит и мягко меняет яркость.
```

Пример ожидаемого ответа от LLM:

```text
effect "warm_heart"

sprite heart {
	bitmap """
	.##.##.
	#######
	#######
	.#####.
	..###..
	...#...
	"""
}

layer love {
	use heart
	color rgb(255, 80 + 30 * sin(t * 2), 90)
	x = 3 + t * 2
	y = 4 + sin(t * 1.5) * 2
	scale = 1 + abs(sin(t * 2)) * 0.4
	visible = 1
}
```

## Советы по качеству сгенерированных эффектов

Если ответ LLM плохой, обычно помогает уточнить одно из трёх:

1. Форму:
	 - "используй сердце"
	 - "используй молнию"
	 - "используй одну точку и раскрась всё полотно"

2. Движение:
	 - "пульсирует"
	 - "плывёт слева направо"
	 - "мигает короткими вспышками"

3. Цвет:
	 - "тёплые оранжевые тона"
	 - "холодная голубая молния"
	 - "радуга по ширине полотна"

Лучше всего LLM справляется с задачами вида:
- один главный sprite;
- один или два layer;
- простая траектория через `sin`, `cos`, `abs`;
- простой цвет через `rgb` или `hsv`.

## Полезные API-маршруты

Текущие важные endpoints:
- `GET /api/status`
- `POST /api/live/validate`
- `POST /api/live/run`
- `GET /api/presets`
- `PUT /api/presets/:id`
- `POST /api/presets/:id/activate`
- `POST /api/playlists/:id/start`
- `POST /api/playlists/stop`

## Направление OTA и release engineering

Проект целится в схему обновлений по образцу microbbox:
- GitHub Releases как источник OTA-артефактов;
- dev/stable каналы;
- version/channel embedded в firmware build flags.

Полный production-grade OTA workflow ещё продолжает собираться, но архитектурное направление уже зафиксировано.

