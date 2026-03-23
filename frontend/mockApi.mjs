const scenarioHeader = "x-dev-scenario";

const defaultScenarioId = "happy-path";
const validScenarios = new Set([
  "happy-path",
  "autoplay",
  "dsl-error",
  "offline-ish",
  "sensor-missing",
]);

const snippetSources = {
  "warm-waves": `effect "warm_waves"

sprite band {
  bitmap """
  ####
  """
}

layer wave {
  use band
  color rgb(255, 160, 80)
  x = 2
  y = 6
  scale = 3
  visible = 1
}`,
  clock: `effect "clock"

sprite dot {
  bitmap """
  #
  """
}

layer indicator {
  use dot
  color rgb(80, 180, 255)
  x = 15
  y = 7
  scale = 2
  visible = 1
}`,
  heart: `effect "heart"

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
  color rgb(255, 50, 90)
  x = 10
  y = 4
  scale = 2
  visible = 1
}`,
};

const scenarioState = new Map();

function getScenarioId(req) {
  const headerValue = req.headers[scenarioHeader];
  const candidate = Array.isArray(headerValue) ? headerValue[0] : headerValue;
  return candidate && validScenarios.has(candidate) ? candidate : defaultScenarioId;
}

function makePreset(id, name, source, tags) {
  const now = "2026-03-23T18:45:00Z";
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

function makePlaylist() {
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

function makeNetworkSettings(overrides = {}) {
  return {
    mode: "ap",
    accessPointName: "MYLAMP",
    clientSsid: "",
    clientPassword: "",
    ...overrides,
  };
}

function createMockState(scenarioId) {
  const presets = [
    makePreset("warm-waves", "Теплые волны", snippetSources["warm-waves"], ["warm", "ambient"]),
    makePreset("clock", "Часы", snippetSources.clock, ["utility"]),
    makePreset("heart", "Летающее сердечко", snippetSources.heart, ["kids"]),
  ];
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
        update: createUpdateSnapshot(common, {
          updateChannel: "dev",
          updateState: "up-to-date",
        }),
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
        update: createUpdateSnapshot(common, {
          updateChannel: "dev",
          updateState: "available",
          availableVersion: "feature-bootstrap-lamp-foundation-a1b2c3d",
        }),
      };
    case "offline-ish":
      return {
        networkSettings: makeNetworkSettings({ mode: "client", clientSsid: "OfficeWiFi", clientPassword: "secret123" }),
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
        update: createUpdateSnapshot(common, {
          updateChannel: "dev",
          updateState: "error",
          updateError: "GitHub Releases недоступен",
        }),
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
        update: createUpdateSnapshot(common, {
          updateChannel: "stable",
          updateState: "idle",
        }),
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
        update: createUpdateSnapshot(common, {
          updateChannel: "dev",
          updateState: "idle",
        }),
      };
  }
}

function getState(scenarioId) {
  if (!scenarioState.has(scenarioId)) {
    scenarioState.set(scenarioId, createMockState(scenarioId));
  }
  return scenarioState.get(scenarioId);
}

function resetState(scenarioId) {
  const seeded = createMockState(scenarioId);
  scenarioState.set(scenarioId, seeded);
  return seeded;
}

function sendJson(res, statusCode, payload) {
  res.statusCode = statusCode;
  res.setHeader("Content-Type", "application/json; charset=utf-8");
  res.end(JSON.stringify(payload));
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on("data", (chunk) => chunks.push(Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk)));
    req.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
    req.on("error", reject);
  });
}

function parseJson(body) {
  if (!body.trim()) {
    return null;
  }

  try {
    return JSON.parse(body);
  } catch {
    return null;
  }
}

function parseRequestBody(body) {
  const json = parseJson(body);
  if (json) {
    return json;
  }

  const params = new URLSearchParams(body);
  const parsed = {};

  for (const [key, value] of params.entries()) {
    parsed[key] = value;
  }

  return parsed;
}

function createUpdateSnapshot(status, overrides = {}) {
  return {
    version: status.version,
    channel: status.channel,
    board: status.board,
    hardwareType: "c3-cylinder32x16",
    updateChannel: status.channel,
    updateState: "idle",
    availableVersion: "",
    updateError: "",
    ...overrides,
  };
}

function findPreset(state, presetId) {
  return state.presets.find((preset) => preset.id === presetId);
}

function findPlaylist(state, playlistId) {
  return state.playlists.find((playlist) => playlist.id === playlistId);
}

function buildDslResponse(scenarioId) {
  if (scenarioId === "dsl-error") {
    return {
      ok: false,
      errors: [
        {
          line: 4,
          column: 9,
          message: "Неизвестная функция hum(). Возможно, ты хотел humidity().",
        },
      ],
    };
  }

  return { ok: true, errors: [] };
}

export async function handleApi(req, res) {
  const url = new URL(req.url || "/", "http://localhost");
  const pathname = url.pathname;
  const method = req.method || "GET";
  const scenarioId = getScenarioId(req);
  const state = getState(scenarioId);

  if (pathname === "/__dev/reset" && method === "POST") {
    sendJson(res, 200, { ok: true, scenario: scenarioId, status: resetState(scenarioId).status });
    return true;
  }

  if (pathname === "/api/status" && method === "GET") {
    sendJson(res, 200, state.status);
    return true;
  }

  if (pathname === "/api/settings/network" && method === "GET") {
    sendJson(res, 200, {
      mode: state.networkSettings.mode,
      accessPointName: state.networkSettings.accessPointName,
      clientSsid: state.networkSettings.clientSsid,
    });
    return true;
  }

  if (pathname === "/api/settings/network" && method === "POST") {
    const payload = parseRequestBody(await readBody(req));
    const mode = payload && (payload.mode === "ap" || payload.mode === "client") ? payload.mode : "";
    if (!mode) {
      sendJson(res, 400, { error: "invalid mode" });
      return true;
    }

    state.networkSettings.mode = mode;
    state.networkSettings.accessPointName = payload.accessPointName || "MYLAMP";
    state.networkSettings.clientSsid = payload.clientSsid || "";
    state.networkSettings.clientPassword = payload.clientPassword || "";
    state.status.networkMode = mode;
    state.status.networkStatus = mode === "client"
      ? `WiFi: ${state.networkSettings.clientSsid || "configured"}`
      : `AP: ${state.networkSettings.accessPointName}`;

    sendJson(res, 200, {
      mode: state.networkSettings.mode,
      accessPointName: state.networkSettings.accessPointName,
      clientSsid: state.networkSettings.clientSsid,
    });
    return true;
  }

  if (pathname === "/api/update/current" && method === "GET") {
    sendJson(res, 200, state.update);
    return true;
  }

  if (pathname === "/api/update/settings" && method === "GET") {
    sendJson(res, 200, { channel: state.update.updateChannel });
    return true;
  }

  if (pathname === "/api/update/settings" && method === "POST") {
    const payload = parseRequestBody(await readBody(req));
    const channel = payload && (payload.channel === "stable" || payload.channel === "dev") ? payload.channel : "";
    if (!channel) {
      sendJson(res, 400, { error: "invalid update channel" });
      return true;
    }

    state.update.updateChannel = channel;
    sendJson(res, 200, { channel });
    return true;
  }

  if (pathname === "/api/update/check" && method === "POST") {
    const payload = parseRequestBody(await readBody(req));
    const channelOverride = payload && (payload.channel === "stable" || payload.channel === "dev") ? payload.channel : "";
    const effectiveChannel = channelOverride || state.update.updateChannel;

    state.update.updateChannel = effectiveChannel;

    if (scenarioId === "offline-ish") {
      state.update.updateState = "error";
      state.update.availableVersion = "";
      state.update.updateError = "GitHub Releases недоступен";
      sendJson(res, 200, {
        hasUpdate: false,
        channel: effectiveChannel,
        version: "",
        assetName: "",
        downloadUrl: "",
        checksumUrl: "",
        error: state.update.updateError,
      });
      return true;
    }

    const version = effectiveChannel === "stable" ? "v0.1.0" : "feature-bootstrap-lamp-foundation-a1b2c3d";
    const hasUpdate = scenarioId === "dsl-error" || effectiveChannel === "dev";

    state.update.updateState = hasUpdate ? "available" : "up-to-date";
    state.update.availableVersion = hasUpdate ? version : "";
    state.update.updateError = "";

    sendJson(res, 200, {
      hasUpdate,
      channel: effectiveChannel,
      version: hasUpdate ? version : "",
      assetName: hasUpdate ? `mylamp-c3-cylinder32x16-${effectiveChannel === "stable" ? "v0.1.0-release" : `dev-${version}`}.bin` : "",
      downloadUrl: hasUpdate ? `https://example.invalid/${effectiveChannel}/${version}.bin` : "",
      checksumUrl: hasUpdate ? `https://example.invalid/${effectiveChannel}/${version}.sha256` : "",
      error: "",
    });
    return true;
  }

  if (pathname === "/api/update/install" && method === "POST") {
    if (!state.update.availableVersion) {
      sendJson(res, 400, { success: false, error: "no update available" });
      return true;
    }

    state.update.updateState = "completed";
    state.update.updateError = "";
    sendJson(res, 200, { success: true, rebooting: true });
    return true;
  }

  if ((pathname === "/api/live/validate" || pathname === "/api/live/run") && method === "POST") {
    const response = buildDslResponse(scenarioId);
    if (pathname === "/api/live/run" && response.ok) {
      state.status.liveErrorSummary = "";
    }
    sendJson(res, 200, response);
    return true;
  }

  if (pathname === "/api/presets" && method === "GET") {
    sendJson(res, 200, { items: state.presets.map(({ id, name, updatedAt }) => ({ id, name, updatedAt })) });
    return true;
  }

  if (pathname === "/api/presets" && method === "PUT") {
    const presetId = url.searchParams.get("id") || "";
    if (!presetId) {
      sendJson(res, 400, { error: "preset id is required" });
      return true;
    }

    const payload = parseJson(await readBody(req));
    if (!payload || !payload.name || !payload.source) {
      sendJson(res, 400, { error: "invalid preset payload" });
      return true;
    }
    const brightnessCap =
      payload.options && typeof payload.options.brightnessCap === "number"
        ? payload.options.brightnessCap
        : 0.35;
    const preset = {
      id: presetId,
      name: payload.name,
      source: payload.source,
      createdAt: payload.createdAt || new Date().toISOString(),
      updatedAt: new Date().toISOString(),
      tags: payload.tags || [],
      options: { brightnessCap },
    };
    const existingIndex = state.presets.findIndex((item) => item.id === presetId);
    if (existingIndex >= 0) {
      state.presets[existingIndex] = preset;
    } else {
      state.presets.push(preset);
    }
    sendJson(res, 200, preset);
    return true;
  }

  if (pathname.startsWith("/api/presets/")) {
    const suffix = pathname.slice("/api/presets/".length);
    if (suffix.endsWith("/activate") && method === "POST") {
      const presetId = suffix.slice(0, -"/activate".length);
      const preset = findPreset(state, presetId);
      if (!preset) {
        sendJson(res, 404, { error: "preset not found" });
        return true;
      }
      state.status.activePresetId = preset.id;
      state.status.activePresetName = preset.name;
      state.status.autoplayEnabled = false;
      state.status.activePlaylistId = "";
      sendJson(res, 200, { ok: true, activePresetId: preset.id, temporary: false, autoplayActive: false });
      return true;
    }

    if (method === "GET") {
      const preset = findPreset(state, suffix);
      sendJson(res, preset ? 200 : 404, preset || { error: "preset not found" });
      return true;
    }

    if (method === "PUT") {
      const payload = parseJson(await readBody(req));
      if (!payload || !payload.name || !payload.source) {
        sendJson(res, 400, { error: "invalid preset payload" });
        return true;
      }
      const brightnessCap =
        payload.options && typeof payload.options.brightnessCap === "number"
          ? payload.options.brightnessCap
          : 0.35;
      const preset = {
        id: suffix,
        name: payload.name,
        source: payload.source,
        createdAt: payload.createdAt || new Date().toISOString(),
        updatedAt: new Date().toISOString(),
        tags: payload.tags || [],
        options: { brightnessCap },
      };
      const existingIndex = state.presets.findIndex((item) => item.id === suffix);
      if (existingIndex >= 0) {
        state.presets[existingIndex] = preset;
      } else {
        state.presets.push(preset);
      }
      sendJson(res, 200, preset);
      return true;
    }

    if (method === "DELETE") {
      state.presets = state.presets.filter((preset) => preset.id !== suffix);
      sendJson(res, 200, { ok: true });
      return true;
    }
  }

  if (pathname.startsWith("/api/playlists/")) {
    if (pathname === "/api/playlists/stop" && method === "POST") {
      state.status.autoplayEnabled = false;
      state.status.activePlaylistId = "";
      sendJson(res, 200, { ok: true });
      return true;
    }

    const suffix = pathname.slice("/api/playlists/".length);
    if (suffix.endsWith("/start") && method === "POST") {
      const playlistId = suffix.slice(0, -"/start".length);
      const playlist = findPlaylist(state, playlistId);
      if (!playlist) {
        sendJson(res, 404, { error: "playlist not found" });
        return true;
      }
      state.status.activePlaylistId = playlist.id;
      state.status.autoplayEnabled = true;
      const firstEntry = playlist.entries.find((entry) => entry.enabled);
      const preset = firstEntry ? findPreset(state, firstEntry.presetId) : undefined;
      if (preset) {
        state.status.activePresetId = preset.id;
        state.status.activePresetName = preset.name;
      }
      sendJson(res, 200, {
        ok: true,
        activePlaylistId: playlist.id,
        active: true,
        autoplayActive: true,
        activeEntryIndex: 0,
      });
      return true;
    }

    if (method === "PUT") {
      const payload = parseJson(await readBody(req));
      if (!payload || !payload.name || !Array.isArray(payload.entries)) {
        sendJson(res, 400, { error: "invalid playlist payload" });
        return true;
      }
      const repeat = typeof payload.repeat === "boolean" ? payload.repeat : false;
      const playlist = {
        id: suffix,
        name: payload.name,
        repeat,
        entries: payload.entries.map((entry) => ({
          presetId: entry.presetId || "",
          durationSec: entry.durationSec || 60,
          enabled: typeof entry.enabled === "boolean" ? entry.enabled : true,
        })),
      };
      const existingIndex = state.playlists.findIndex((item) => item.id === suffix);
      if (existingIndex >= 0) {
        state.playlists[existingIndex] = playlist;
      } else {
        state.playlists.push(playlist);
      }
      sendJson(res, 200, playlist);
      return true;
    }

    if (method === "DELETE") {
      state.playlists = state.playlists.filter((playlist) => playlist.id !== suffix);
      sendJson(res, 200, { ok: true });
      return true;
    }
  }

  return false;
}

export function mockApiPlugin() {
  return {
    name: "mylamp-mock-api",
    apply: "serve",
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        void handleApi(req, res).then((handled) => {
          if (!handled) {
            next();
          }
        });
      });
    },
  };
}