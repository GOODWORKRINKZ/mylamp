export type ScenarioId =
  | "happy-path"
  | "autoplay"
  | "dsl-error"
  | "offline-ish"
  | "sensor-missing";

export type StatusPayload = {
  version: string;
  channel: string;
  board: string;
  networkMode: string;
  networkStatus: string;
  clockStatus: string;
  currentTime: string;
  sensorStatus: string;
  temperatureC: number | null;
  humidityPercent: number | null;
  activeEffect: string;
  activePresetId: string;
  activePresetName: string;
  autoplayEnabled: boolean;
  activePlaylistId: string;
  liveErrorSummary: string;
};

export type LiveDiagnostic = {
  line: number;
  column: number;
  message: string;
};

export type LiveDiagnosticResponse = {
  ok: boolean;
  errors: LiveDiagnostic[];
};

export type PresetPayload = {
  id: string;
  name: string;
  source: string;
  createdAt: string;
  updatedAt: string;
  tags: string[];
  options: {
    brightnessCap: number;
  };
};

export type PlaylistEntryPayload = {
  presetId: string;
  durationSec: number;
  enabled: boolean;
};

export type PlaylistPayload = {
  id: string;
  name: string;
  repeat: boolean;
  entries: PlaylistEntryPayload[];
};

export type ScenarioDefinition = {
  id: ScenarioId;
  label: string;
  description: string;
};

export type MockState = {
  status: StatusPayload;
  presets: PresetPayload[];
  playlists: PlaylistPayload[];
};