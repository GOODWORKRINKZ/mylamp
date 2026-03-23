import { starterSnippets } from "../editor/snippets";
import type { MockState, NetworkSettingsPayload, PlaylistPayload, PresetPayload, ScenarioDefinition, ScenarioId } from "./mockTypes";

const now = "2026-03-23T18:45:00Z";

function makePreset(id: string, name: string, source: string, tags: string[]): PresetPayload {
  return {
    id,
    name,
    source,
    createdAt: now,
    updatedAt: now,
    tags,
    options: { brightnessCap: 0.35 },
  };
}

function makePlaylist(): PlaylistPayload {
  return {
    id: "evening",
    name: "Evening Loop",
    repeat: true,
    entries: [
      { presetId: "warm-waves", durationSec: 90, enabled: true },
      { presetId: "clock", durationSec: 60, enabled: true },
    ],
  };
}

function makeNetworkSettings(overrides: Partial<NetworkSettingsPayload> = {}): NetworkSettingsPayload {
  return {
    mode: "ap",
    accessPointName: "MYLAMP-DEV",
    clientSsid: "",
    ...overrides,
  };
}

const snippetById = new Map(starterSnippets.map((snippet) => [snippet.id, snippet]));

function seedPresets(): PresetPayload[] {
  return [
    makePreset("warm-waves", "Теплые волны", snippetById.get("warm-waves")?.source ?? "", ["warm", "ambient"]),
    makePreset("clock", "Часы", snippetById.get("clock")?.source ?? "", ["utility"]),
    makePreset("heart", "Летающее сердечко", snippetById.get("heart")?.source ?? "", ["kids"]),
  ];
}

export const scenarioDefinitions: ScenarioDefinition[] = [
  { id: "happy-path", label: "Happy Path", description: "Лампа онлайн, preset активен, ошибок нет." },
  { id: "autoplay", label: "Autoplay", description: "Активен playlist и включено автопереключение." },
  { id: "dsl-error", label: "DSL Error", description: "Валидация и запуск возвращают русские diagnostics." },
  { id: "offline-ish", label: "Offline-ish", description: "Сеть нестабильна, статус деградирован, но UI жив." },
  { id: "sensor-missing", label: "Sensor Missing", description: "Датчик не отвечает, temperature/humidity пустые." },
];

export const defaultScenarioId: ScenarioId = "happy-path";

export function isScenarioId(value: string): value is ScenarioId {
  return scenarioDefinitions.some((scenario) => scenario.id === value);
}

export function createMockState(scenarioId: ScenarioId): MockState {
  const presets = seedPresets();
  const playlists = [makePlaylist()];
  const common = {
    version: "0.1.0-dev",
    channel: "dev",
    board: "esp32-c3-supermini",
    networkMode: "ap",
    clockStatus: "Clock: NTP ok",
    currentTime: "21:14:08",
    activeEffect: "boot-solid",
  };

  switch (scenarioId) {
    case "autoplay":
      return {
        networkSettings: makeNetworkSettings(),
        presets,
        playlists,
        status: {
          ...common,
          networkStatus: "AP: MYLAMP-DEV",
          sensorStatus: "Sensor: ok",
          temperatureC: 24.6,
          humidityPercent: 41.2,
          activePresetId: "clock",
          activePresetName: "Часы",
          autoplayEnabled: true,
          activePlaylistId: "evening",
          liveErrorSummary: "",
        },
      };
    case "dsl-error":
      return {
        networkSettings: makeNetworkSettings(),
        presets,
        playlists,
        status: {
          ...common,
          networkStatus: "AP: MYLAMP-DEV",
          sensorStatus: "Sensor: ok",
          temperatureC: 23.1,
          humidityPercent: 44.8,
          activePresetId: "warm-waves",
          activePresetName: "Теплые волны",
          autoplayEnabled: false,
          activePlaylistId: "",
          liveErrorSummary: "Строка 4: неизвестная функция hum(). Возможно, ты хотел humidity().",
        },
      };
    case "offline-ish":
      return {
        networkSettings: makeNetworkSettings({ mode: "client", clientSsid: "OfficeWiFi" }),
        presets,
        playlists,
        status: {
          ...common,
          networkStatus: "WiFi: reconnecting",
          sensorStatus: "Sensor: stale",
          temperatureC: 22.4,
          humidityPercent: 39.7,
          activePresetId: "heart",
          activePresetName: "Летающее сердечко",
          autoplayEnabled: false,
          activePlaylistId: "",
          liveErrorSummary: "",
        },
      };
    case "sensor-missing":
      return {
        networkSettings: makeNetworkSettings(),
        presets,
        playlists,
        status: {
          ...common,
          networkStatus: "AP: MYLAMP-DEV",
          sensorStatus: "Sensor: unavailable",
          temperatureC: null,
          humidityPercent: null,
          activePresetId: "warm-waves",
          activePresetName: "Теплые волны",
          autoplayEnabled: false,
          activePlaylistId: "",
          liveErrorSummary: "",
        },
      };
    case "happy-path":
    default:
      return {
        networkSettings: makeNetworkSettings(),
        presets,
        playlists,
        status: {
          ...common,
          networkStatus: "AP: MYLAMP-DEV",
          sensorStatus: "Sensor: ok",
          temperatureC: 24.2,
          humidityPercent: 40.1,
          activePresetId: "warm-waves",
          activePresetName: "Теплые волны",
          autoplayEnabled: false,
          activePlaylistId: "",
          liveErrorSummary: "",
        },
      };
  }
}