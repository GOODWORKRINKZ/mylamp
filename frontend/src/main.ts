import "./styles/app.css";
import { editorHelpSections } from "./editor/help";
import { starterSnippets, type StarterSnippet } from "./editor/snippets";
import { defaultScenarioId, isScenarioId, scenarioDefinitions } from "./dev/mockScenarios";
import { renderShellMarkup } from "./ui/shellTemplate";
import type {
  LiveDiagnosticResponse,
  NetworkSettingsPayload,
  PlaylistEntryPayload,
  PlaylistPayload,
  PresetListItem,
  PresetPayload,
  ScenarioId,
  StatusPayload,
  TimeSettingsPayload,
  UpdateCheckPayload,
  UpdateCurrentPayload,
  UpdateInstallPayload,
} from "./dev/mockTypes";

const app = document.querySelector<HTMLDivElement>("#app");

if (!app) {
  throw new Error("App root not found");
}

const devScenarioStorageKey = "mylamp-dev-scenario";
const isDevServer = Boolean((import.meta as ImportMeta & { env?: { DEV?: boolean } }).env?.DEV);

function readScenarioFromUrl(): ScenarioId | null {
  const params = new URLSearchParams(window.location.search);
  const scenario = params.get("scenario");
  return scenario && isScenarioId(scenario) ? scenario : null;
}

function getSelectedScenario(): ScenarioId {
  const urlScenario = readScenarioFromUrl();
  if (urlScenario) {
    localStorage.setItem(devScenarioStorageKey, urlScenario);
    return urlScenario;
  }

  const storedScenario = localStorage.getItem(devScenarioStorageKey);
  return storedScenario && isScenarioId(storedScenario) ? storedScenario : defaultScenarioId;
}

let selectedScenario: ScenarioId = getSelectedScenario();
let currentUpdateSnapshot: UpdateCurrentPayload | null = null;
let currentNetworkSettings: NetworkSettingsPayload | null = null;
let currentTimeSettings: TimeSettingsPayload | null = null;
let currentStatus: StatusPayload | null = null;
let updateBusyAction: "" | "check" | "install" | "settings" = "";
let updateRebootPending = false;
let updateRebootAttempt = 0;
let networkModalOpen = false;
let firmwareModalOpen = false;
let timeModalOpen = false;
let networkModalLoading = false;
let networkModalSaving = false;
let networkSettingsLoaded = false;
let timeModalLoading = false;
let timeModalSaving = false;
let timeSettingsLoaded = false;
let presetListItems: PresetListItem[] = [];
let presetListLoading = false;
let playlistItems: PlaylistPayload[] = [];
let playlistListLoading = false;
let activeEditingPresetId = "";
let activeEditingEffectName = "";
let activeEditingPlaylistId = "";
let playlistEditorDraft: PlaylistPayload | null = null;

const timezoneOptions: Array<{ value: string; label: string }> = [
  { value: "UTC0", label: "UTC" },
  { value: "EET-2EEST,M3.5.0/3,M10.5.0/4", label: "Kyiv / EET" },
  { value: "MSK-3", label: "Moscow / MSK" },
  { value: "CET-1CEST,M3.5.0,M10.5.0/3", label: "Berlin / CET" },
  { value: "EST5EDT,M3.2.0/2,M11.1.0/2", label: "New York / EST" },
  { value: "PST8PDT,M3.2.0/2,M11.1.0/2", label: "Los Angeles / PST" },
];

const blankEffectTemplate = `effect "new_effect"

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
}`;

function renderStarterSnippetList(): string {
  return starterSnippets
    .map(
      (snippet) => `
        <li class="snippet-item">
          <button class="snippet-button" data-snippet-id="${snippet.id}" type="button">
            <strong>${snippet.name}</strong>
            <span>${snippet.description}</span>
          </button>
        </li>`,
    )
    .join("");
}

function renderHelpSections(): string {
  return editorHelpSections
    .map(
      (section) => `
        <section class="help-card">
          <h3>${section.title}</h3>
          <ul class="item-list item-list--compact">
            ${section.items
              .map((item) => `<li><strong>${item.term}</strong> - ${item.description}</li>`)
              .join("")}
          </ul>
        </section>`,
    )
    .join("");
}

function getEditorValue(): string {
  const editor = document.getElementById("editor-code") as HTMLTextAreaElement | null;
  return editor?.value ?? "";
}

function readEffectName(source: string): string {
  const match = source.match(/^\s*effect\s+"([^"\n]+)"/m);
  return match?.[1]?.trim() ?? "";
}

function buildPresetId(effectName: string): string {
  return effectName
    .trim()
    .toLowerCase()
    .replace(/\s+/g, "-")
    .replace(/[^\p{L}\p{N}_-]+/gu, "-")
    .replace(/-+/g, "-")
    .replace(/^-|-$/g, "");
}

function escapeHtml(value: string): string {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function formatPresetTimestamp(value: string): string {
  const parsed = new Date(value);
  if (Number.isNaN(parsed.getTime())) {
    return value || "Без даты";
  }

  return new Intl.DateTimeFormat("ru-RU", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  }).format(parsed);
}

function formatPlaylistSummary(playlist: PlaylistPayload): string {
  const enabledCount = playlist.entries.filter((entry) => entry.enabled).length;
  const totalDurationSec = playlist.entries
    .filter((entry) => entry.enabled)
    .reduce((sum, entry) => sum + entry.durationSec, 0);
  const minutes = totalDurationSec > 0 ? `${Math.max(1, Math.round(totalDurationSec / 60))} мин` : "0 мин";
  return `${enabledCount}/${playlist.entries.length} активны · ${minutes} · ${playlist.repeat ? "повтор" : "один круг"}`;
}

function buildPlaylistId(name: string): string {
  return buildPresetId(name);
}

function clonePlaylist(playlist: PlaylistPayload): PlaylistPayload {
  return {
    id: playlist.id,
    name: playlist.name,
    repeat: playlist.repeat,
    entries: playlist.entries.map((entry) => ({ ...entry })),
  };
}

function createDefaultPlaylistEntry(): PlaylistEntryPayload {
  return {
    presetId: presetListItems[0]?.id || "",
    durationSec: 60,
    enabled: true,
  };
}

function createEmptyPlaylistDraft(): PlaylistPayload {
  return {
    id: "",
    name: "",
    repeat: true,
    entries: [createDefaultPlaylistEntry()],
  };
}

function getPlaylistNameById(playlistId: string): string {
  if (!playlistId) {
    return "";
  }

  return playlistItems.find((playlist) => playlist.id === playlistId)?.name || playlistId;
}

function renderPresetList(): void {
  const container = document.getElementById("saved-presets-list");
  const meta = document.getElementById("saved-presets-meta");
  if (!container || !meta) {
    return;
  }

  if (presetListLoading) {
    meta.textContent = "Загружаем...";
    container.innerHTML = `<div class="preset-library__empty">Смотрим, что уже сохранено на лампе.</div>`;
    return;
  }

  meta.textContent = presetListItems.length === 0 ? "Пока пусто" : `${presetListItems.length} шт.`;

  if (presetListItems.length === 0) {
    container.innerHTML = `<div class="preset-library__empty">Пока нет сохранённых preset-ов. Сохрани текущий эффект, и он появится здесь.</div>`;
    return;
  }

  const activePresetId = currentStatus?.activePresetId ?? "";
  container.innerHTML = presetListItems
    .map((item) => {
      const activeClass = item.id === activePresetId ? " preset-item--active" : "";
      return `
        <article class="preset-item${activeClass}">
          <div class="preset-item__header">
            <strong class="preset-item__name">${escapeHtml(item.name)}</strong>
            <span class="preset-item__date">${escapeHtml(formatPresetTimestamp(item.updatedAt))}</span>
          </div>
          <div class="preset-item__actions">
            <button class="preset-item__button preset-item__button--secondary" data-preset-action="open" data-preset-id="${escapeHtml(item.id)}" type="button">Открыть</button>
            <button class="preset-item__button" data-preset-action="activate" data-preset-id="${escapeHtml(item.id)}" type="button">На лампу</button>
            <button class="preset-item__button preset-item__button--danger" data-preset-action="delete" data-preset-id="${escapeHtml(item.id)}" data-preset-name="${escapeHtml(item.name)}" type="button">Удалить</button>
          </div>
        </article>`;
    })
    .join("");
}

function renderPlaylistPanel(): void {
  const container = document.getElementById("saved-playlists-list");
  const meta = document.getElementById("saved-playlists-meta");
  const stopButton = document.getElementById("stop-playlist-button") as HTMLButtonElement | null;
  if (!container || !meta) {
    return;
  }

  if (stopButton) {
    stopButton.disabled = !currentStatus?.activePlaylistId;
  }

  if (playlistListLoading) {
    meta.textContent = "Загружаем...";
    container.innerHTML = `<div class="playlist-library__empty">Собираем список очередей с лампы.</div>`;
    return;
  }

  meta.textContent = playlistItems.length === 0 ? "Пока пусто" : `${playlistItems.length} шт.`;

  if (playlistItems.length === 0) {
    container.innerHTML = `<div class="playlist-library__empty">Пока нет сохранённых playlist-ов. Следующий шаг из плана — добавить редактор создания.</div>`;
    return;
  }

  const activePlaylistId = currentStatus?.activePlaylistId ?? "";
  container.innerHTML = playlistItems
    .map((playlist) => {
      const activeClass = playlist.id === activePlaylistId ? " playlist-item--active" : "";
      return `
        <article class="playlist-item${activeClass}">
          <div class="playlist-item__header">
            <strong class="playlist-item__name">${escapeHtml(playlist.name)}</strong>
            <span class="playlist-item__summary">${escapeHtml(formatPlaylistSummary(playlist))}</span>
          </div>
          <div class="playlist-item__actions">
            <button class="playlist-item__button playlist-item__button--secondary" data-playlist-action="edit" data-playlist-id="${escapeHtml(playlist.id)}" type="button">Редактировать</button>
            <button class="playlist-item__button" data-playlist-action="start" data-playlist-id="${escapeHtml(playlist.id)}" type="button">Запустить</button>
            <button class="playlist-item__button preset-item__button--danger" data-playlist-action="delete" data-playlist-id="${escapeHtml(playlist.id)}" data-playlist-name="${escapeHtml(playlist.name)}" type="button">Удалить</button>
          </div>
        </article>`;
    })
    .join("");
}

function renderPlaylistEditor(): void {
  const host = document.getElementById("playlist-editor-host");
  if (!host) {
    return;
  }

  if (!playlistEditorDraft) {
    host.innerHTML = "";
    return;
  }

  const title = activeEditingPlaylistId ? `Редактирование «${playlistEditorDraft.name || activeEditingPlaylistId}»` : "Новый playlist";
  const optionsMarkup = presetListItems
    .map(
      (preset) =>
        `<option value="${escapeHtml(preset.id)}">${escapeHtml(preset.name)}</option>`,
    )
    .join("");

  host.innerHTML = `
    <section class="playlist-editor" aria-labelledby="playlist-editor-title">
      <div class="playlist-editor__header">
        <div>
          <div id="playlist-editor-title" class="playlist-editor__title">${escapeHtml(title)}</div>
          <div class="playlist-editor__hint">Собери очередь из сохранённых preset-ов, укажи длительности и сохрани на лампу.</div>
        </div>
      </div>
      <div class="playlist-editor__grid">
        <label class="field-stack" for="playlist-name-input">
          <span>Имя playlist-а</span>
          <input id="playlist-name-input" type="text" value="${escapeHtml(playlistEditorDraft.name)}" placeholder="Например: Evening Loop" />
        </label>
        <label class="playlist-entry__toggle" for="playlist-repeat-input">
          <input id="playlist-repeat-input" type="checkbox"${playlistEditorDraft.repeat ? " checked" : ""} />
          <span>Повторять playlist по кругу</span>
        </label>
      </div>
      <div class="playlist-editor__entries" id="playlist-editor-entries">
        ${playlistEditorDraft.entries.length === 0 ? `<div class="playlist-editor__empty">Список пуст. Добавь хотя бы один entry.</div>` : ""}
        ${playlistEditorDraft.entries
          .map(
            (entry, index) => `
              <section class="playlist-entry">
                <div class="playlist-entry__header">
                  <strong class="playlist-entry__title">Entry ${index + 1}</strong>
                  <button class="playlist-item__button preset-item__button--danger" data-playlist-editor-action="remove-entry" data-entry-index="${index}" type="button">Убрать</button>
                </div>
                <div class="playlist-entry__grid">
                  <label class="field-stack">
                    <span>Preset</span>
                    <select data-entry-field="presetId" data-entry-index="${index}">
                      <option value="">Выбери preset</option>
                      ${optionsMarkup.replace(`value="${escapeHtml(entry.presetId)}"`, `value="${escapeHtml(entry.presetId)}" selected`)}
                    </select>
                  </label>
                  <label class="field-stack">
                    <span>Длительность, сек</span>
                    <input data-entry-field="durationSec" data-entry-index="${index}" type="number" min="1" step="1" value="${entry.durationSec}" />
                  </label>
                  <label class="playlist-entry__toggle">
                    <input data-entry-field="enabled" data-entry-index="${index}" type="checkbox"${entry.enabled ? " checked" : ""} />
                    <span>Активен</span>
                  </label>
                </div>
              </section>`,
          )
          .join("")}
      </div>
      <div class="playlist-editor__actions">
        <button class="playlist-item__button playlist-item__button--secondary" data-playlist-editor-action="add-entry" type="button">Добавить entry</button>
        <button class="playlist-item__button" data-playlist-editor-action="save" type="button">Сохранить playlist</button>
        <button class="playlist-item__button playlist-item__button--secondary" data-playlist-editor-action="cancel" type="button">Отмена</button>
      </div>
    </section>`;
}

function readPlaylistDraftFromForm(): PlaylistPayload | null {
  if (!playlistEditorDraft) {
    return null;
  }

  const nameInput = document.getElementById("playlist-name-input") as HTMLInputElement | null;
  const repeatInput = document.getElementById("playlist-repeat-input") as HTMLInputElement | null;
  const entryNodes = Array.from(document.querySelectorAll<HTMLElement>("[data-entry-index][data-entry-field='presetId']"));

  const entries = entryNodes.map((node) => {
    const index = Number(node.dataset.entryIndex || -1);
    const presetSelect = document.querySelector<HTMLSelectElement>(`[data-entry-field='presetId'][data-entry-index='${index}']`);
    const durationInput = document.querySelector<HTMLInputElement>(`[data-entry-field='durationSec'][data-entry-index='${index}']`);
    const enabledInput = document.querySelector<HTMLInputElement>(`[data-entry-field='enabled'][data-entry-index='${index}']`);

    const entry: PlaylistEntryPayload = {
      presetId: presetSelect?.value || "",
      durationSec: Number(durationInput?.value || 0),
      enabled: Boolean(enabledInput?.checked),
    };

    return entry;
  });

  return {
    id: activeEditingPlaylistId || playlistEditorDraft.id,
    name: nameInput?.value.trim() || "",
    repeat: Boolean(repeatInput?.checked),
    entries,
  };
}

function closePlaylistEditor(): void {
  activeEditingPlaylistId = "";
  playlistEditorDraft = null;
  renderPlaylistEditor();
}

function openNewPlaylistEditor(): void {
  activeEditingPlaylistId = "";
  playlistEditorDraft = createEmptyPlaylistDraft();
  renderPlaylistEditor();
}

function openPlaylistEditor(playlistId: string): void {
  const playlist = playlistItems.find((item) => item.id === playlistId);
  if (!playlist) {
    setText("diagnostics-summary", `Не удалось найти playlist ${playlistId} для редактирования.`);
    setText("diagnostics-status", "Playlist не найден");
    return;
  }

  activeEditingPlaylistId = playlist.id;
  playlistEditorDraft = clonePlaylist(playlist);
  renderPlaylistEditor();
}

function appendPlaylistEntry(): void {
  const draft = readPlaylistDraftFromForm();
  if (!draft) {
    return;
  }

  draft.entries.push(createDefaultPlaylistEntry());
  playlistEditorDraft = draft;
  renderPlaylistEditor();
}

function removePlaylistEntry(index: number): void {
  const draft = readPlaylistDraftFromForm();
  if (!draft) {
    return;
  }

  draft.entries = draft.entries.filter((_, entryIndex) => entryIndex !== index);
  playlistEditorDraft = draft;
  renderPlaylistEditor();
}

function validatePlaylistDraft(draft: PlaylistPayload): string | null {
  if (presetListItems.length === 0) {
    return "Сначала сохрани хотя бы один preset. Плейлист собирается только из сохранённых эффектов.";
  }

  if (!draft.name.trim()) {
    return "Укажи имя playlist-а.";
  }

  const nextPlaylistId = activeEditingPlaylistId || buildPlaylistId(draft.name);
  if (!nextPlaylistId) {
    return "Не удалось собрать id playlist-а из имени.";
  }

  if (draft.entries.length === 0) {
    return "Добавь хотя бы один entry в playlist.";
  }

  for (let index = 0; index < draft.entries.length; index += 1) {
    const entry = draft.entries[index];
    if (!entry.presetId) {
      return `Entry ${index + 1}: выбери preset.`;
    }
    if (!presetListItems.some((preset) => preset.id === entry.presetId)) {
      return `Entry ${index + 1}: выбранный preset уже недоступен.`;
    }
    if (!Number.isFinite(entry.durationSec) || entry.durationSec < 1) {
      return `Entry ${index + 1}: длительность должна быть не меньше 1 секунды.`;
    }
  }

  return null;
}

async function savePlaylistDraft(): Promise<void> {
  const draft = readPlaylistDraftFromForm();
  if (!draft) {
    return;
  }

  const validationMessage = validatePlaylistDraft(draft);
  if (validationMessage) {
    setText("editor-status", "Playlist заполнен не до конца.");
    setText("diagnostics-summary", validationMessage);
    setText("diagnostics-status", "Проверь playlist");
    return;
  }

  const playlistId = activeEditingPlaylistId || buildPlaylistId(draft.name);
  const payload: PlaylistPayload = {
    id: playlistId,
    name: draft.name.trim(),
    repeat: draft.repeat,
    entries: draft.entries.map((entry) => ({
      presetId: entry.presetId,
      durationSec: Math.max(1, Math.round(entry.durationSec)),
      enabled: entry.enabled,
    })),
  };

  setText("editor-status", "Сохраняем playlist...");

  try {
    const response = await fetch(`/api/playlists/${encodeURIComponent(playlistId)}`, {
      method: "PUT",
      headers: buildJsonHeaders(),
      body: JSON.stringify({
        name: payload.name,
        repeat: payload.repeat,
        entries: payload.entries,
      }),
    });
    const result = (await response.json()) as PlaylistPayload & { error?: string };
    if (!response.ok) {
      throw new Error(result.error || `HTTP ${response.status}`);
    }

    setText("editor-status", "Playlist сохранён.");
    setText("diagnostics-summary", `Playlist «${payload.name}» сохранён. Теперь его можно запускать на лампе.`);
    setText("diagnostics-status", `Playlist id: ${playlistId}`);
    closePlaylistEditor();
    await loadPlaylistList();
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось сохранить playlist.");
    setText("diagnostics-summary", `Не удалось сохранить playlist: ${message}`);
    setText("diagnostics-status", "Сохранение не удалось");
  }
}

function buildJsonHeaders(): Record<string, string> {
  return {
    ...buildApiHeaders(),
    "Content-Type": "application/json",
  };
}

function buildFormHeaders(): Record<string, string> {
  return {
    ...buildApiHeaders(),
    "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
  };
}

function buildFormBody(values: Record<string, string>): string {
  const params = new URLSearchParams();

  Object.entries(values).forEach(([key, value]) => {
    params.set(key, value);
  });

  return params.toString();
}

function renderNetworkModal(): string {
  return `
    <div class="modal" id="network-modal" hidden>
      <div class="modal__backdrop" data-network-close="overlay"></div>
      <section class="modal__dialog" role="dialog" aria-modal="true" aria-labelledby="network-modal-title">
        <div class="modal__header">
          <div>
            <p class="eyebrow">Сеть</p>
            <h2 id="network-modal-title">Настройка сети</h2>
          </div>
          <button class="modal__close" id="network-close-button" type="button" aria-label="Закрыть">Закрыть</button>
        </div>
        <div class="modal__body">
          <p class="modal__summary" id="network-summary">Открой модалку и лампа подгрузит текущие настройки сети.</p>
          <div class="status-note" id="network-settings-status">Ждём запрос к настройкам сети.</div>
          <label class="field-stack" for="network-mode-select">
            <span>Режим сети</span>
            <select id="network-mode-select">
              <option value="ap">Точка доступа</option>
              <option value="client">Клиент Wi-Fi</option>
            </select>
          </label>
          <label class="field-stack" for="network-ap-name-input">
            <span>Имя точки доступа</span>
            <input id="network-ap-name-input" type="text" placeholder="MYLAMP-DEV" />
          </label>
          <label class="field-stack" for="network-ssid-input">
            <span>SSID домашней сети</span>
            <input id="network-ssid-input" type="text" placeholder="MyWiFi" />
          </label>
          <label class="field-stack" for="network-password-input">
            <span>Пароль</span>
            <input id="network-password-input" type="password" placeholder="Введите пароль" />
          </label>
          <p class="modal__hint" id="network-mode-hint">В режиме точки доступа лампа раздаёт собственную сеть.</p>
          <div class="modal__actions">
            <button id="network-save-button" type="button">Сохранить</button>
            <button class="button-secondary" id="network-cancel-button" type="button">Закрыть</button>
          </div>
        </div>
      </section>
    </div>`;
}

function renderFirmwareModal(): string {
  return `
    <div class="modal" id="firmware-modal" hidden>
      <div class="modal__backdrop" data-firmware-close="overlay"></div>
      <section class="modal__dialog" role="dialog" aria-modal="true" aria-labelledby="firmware-modal-title">
        <div class="modal__header">
          <div>
            <p class="eyebrow">Firmware</p>
            <h2 id="firmware-modal-title">Прошивка и OTA</h2>
          </div>
          <button class="modal__close" id="firmware-close-button" type="button" aria-label="Закрыть">Закрыть</button>
        </div>
        <div class="modal__body">
          <p id="update-summary">Сейчас здесь появится статус OTA, канал обновлений и доступная версия.</p>
          <div class="status-note" id="update-status-note">Пробуем получить OTA-сводку с лампы.</div>
          <div class="key-value"><span>Текущая версия</span><strong id="update-version">-</strong></div>
          <div class="key-value"><span>Канал</span><strong id="update-channel">-</strong></div>
          <div class="key-value"><span>Состояние</span><strong id="update-runtime-state">-</strong></div>
          <div class="key-value"><span>Доступная версия</span><strong id="update-available-version">-</strong></div>
          <div class="key-value"><span>Последняя ошибка</span><strong id="update-error">-</strong></div>
          <label class="field-stack" for="update-channel-select">
            <span>Канал обновлений</span>
            <select id="update-channel-select">
              <option value="stable">stable</option>
              <option value="dev">dev</option>
            </select>
          </label>
          <div class="panel__actions panel__actions--wide">
            <button id="update-check-button" type="button">Проверить обновление</button>
            <button id="update-install-button" type="button">Установить</button>
          </div>
        </div>
      </section>
    </div>`;
}

function renderTimeModal(): string {
  return `
    <div class="modal" id="time-modal" hidden>
      <div class="modal__backdrop" data-time-close="overlay"></div>
      <section class="modal__dialog" role="dialog" aria-modal="true" aria-labelledby="time-modal-title">
        <div class="modal__header">
          <div>
            <p class="eyebrow">Time</p>
            <h2 id="time-modal-title">Время и часовой пояс</h2>
          </div>
          <button class="modal__close" id="time-close-button" type="button" aria-label="Закрыть">Закрыть</button>
        </div>
        <div class="modal__body">
          <p class="modal__summary" id="time-summary">Открой модалку и лампа подгрузит текущий часовой пояс.</p>
          <div class="status-note" id="time-settings-status">Ждём запрос к настройкам времени.</div>
          <label class="field-stack" for="time-timezone-select">
            <span>Часовой пояс</span>
            <select id="time-timezone-select">
              ${timezoneOptions.map((option) => `<option value="${option.value}">${option.label}</option>`).join("")}
            </select>
          </label>
          <div class="modal__actions">
            <button id="time-save-button" type="button">Сохранить</button>
            <button class="button-secondary" id="time-cancel-button" type="button">Закрыть</button>
          </div>
        </div>
      </section>
    </div>`;
}

function formatDiagnostics(response: LiveDiagnosticResponse): string {
  if (response.ok || response.errors.length === 0) {
    return "Ошибок не найдено.";
  }

  return response.errors
    .map((error) => `Строка ${error.line}, столбец ${error.column}: ${error.message}`)
    .join("\n");
}

async function postLiveAction(endpoint: "/api/live/validate" | "/api/live/run"): Promise<void> {
  const source = getEditorValue().trim();
  const effectName = readEffectName(source);

  if (!source) {
    setText("diagnostics-summary", "Сначала напиши код эффекта, потом запускай действие.");
    setText("diagnostics-status", "Редактор пустой");
    return;
  }

  setText("editor-status", endpoint === "/api/live/validate" ? "Проверяем код..." : "Отправляем код на лампу...");

  try {
    const response = await fetch(endpoint, {
      method: "POST",
      headers: buildJsonHeaders(),
      body: JSON.stringify({
        source,
        presetName: effectName || undefined,
      }),
    });

    const payload = (await response.json()) as LiveDiagnosticResponse;

    setText("diagnostics-summary", formatDiagnostics(payload));
    setText(
      "diagnostics-status",
      payload.ok
        ? endpoint === "/api/live/validate"
          ? "Проверка прошла успешно"
          : "Код отправлен на лампу"
        : `Найдено ошибок: ${payload.errors.length}`,
    );
    setText(
      "editor-status",
      payload.ok
        ? endpoint === "/api/live/validate"
          ? "Код проверен. Можно запускать или сохранить."
          : "Код запущен. Смотри, как лампа откликнется."
        : "Проверка завершилась с ошибками.",
    );

    void refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("diagnostics-summary", message);
    setText("diagnostics-status", "Не удалось выполнить запрос");
    setText("editor-status", "Запрос завершился с ошибкой.");
  }
}

async function savePreset(): Promise<void> {
  const source = getEditorValue().trim();
  const effectName = readEffectName(source);
  const presetId =
    activeEditingPresetId && activeEditingEffectName === effectName ? activeEditingPresetId : buildPresetId(effectName);

  if (!source) {
    setText("diagnostics-summary", "Нечего сохранять: редактор пустой.");
    setText("diagnostics-status", "Редактор пустой");
    return;
  }

  if (!effectName || !presetId) {
    setText("diagnostics-summary", "Для сохранения укажи имя в первой строке: effect \"my_effect\".");
    setText("diagnostics-status", "Не найдено имя эффекта");
    return;
  }

  setText("editor-status", "Сохраняем эффект...");

  try {
    const response = await fetch(`/api/presets?id=${encodeURIComponent(presetId)}`, {
      method: "PUT",
      headers: buildJsonHeaders(),
      body: JSON.stringify({
        name: effectName,
        source,
        createdAt: new Date().toISOString(),
        updatedAt: new Date().toISOString(),
        tags: [],
        options: { brightnessCap: 0.35 },
      }),
    });

    if (!response.ok) {
      const payload = (await response.json()) as { error?: string };
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("diagnostics-summary", `Эффект «${effectName}» сохранён.`);
    setText("diagnostics-status", `Preset id: ${presetId}`);
    setText("editor-status", "Сохранение завершено.");
    activeEditingPresetId = presetId;
    activeEditingEffectName = effectName;
    await loadPresetList();
    void refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("diagnostics-summary", `Не удалось сохранить эффект: ${message}`);
    setText("diagnostics-status", "Сохранение не удалось");
    setText("editor-status", "Ошибка при сохранении.");
  }
}

function createNewEffect(): void {
  setEditorValue(blankEffectTemplate);
  activeEditingPresetId = "";
  activeEditingEffectName = "";
  setText("editor-hint", "Это пустая заготовка. Дай эффекту имя в строке effect \"...\" и начинай рисовать.");
  setText("editor-status", "Создан новый пустой эффект. Теперь можно редактировать, проверить или сохранить его.");
  setText("diagnostics-summary", "Новая заготовка открыта. Поменяй имя, спрайты и слои, потом сохрани эффект.");
  setText("diagnostics-status", "Черновик готов");
}

async function loadPresetList(): Promise<void> {
  presetListLoading = true;
  renderPresetList();

  try {
    const response = await fetch("/api/presets", { headers: buildApiHeaders() });
    const payload = (await response.json()) as { items?: PresetListItem[]; error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    presetListItems = Array.isArray(payload.items) ? payload.items : [];
    renderPresetList();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    presetListItems = [];
    renderPresetList();
    setText("editor-status", "Не удалось загрузить preset-ы.");
    setText("diagnostics-summary", `Не удалось загрузить список preset-ов: ${message}`);
    setText("diagnostics-status", "Preset-ы недоступны");
  } finally {
    presetListLoading = false;
    renderPresetList();
  }
}

async function loadPlaylistList(): Promise<void> {
  playlistListLoading = true;
  renderPlaylistPanel();

  try {
    const response = await fetch("/api/playlists", { headers: buildApiHeaders() });
    const payload = (await response.json()) as { items?: PlaylistPayload[]; error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    playlistItems = Array.isArray(payload.items) ? payload.items : [];
    renderPlaylistPanel();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    playlistItems = [];
    renderPlaylistPanel();
    setText("editor-status", "Не удалось загрузить playlists.");
    setText("diagnostics-summary", `Не удалось загрузить список playlist-ов: ${message}`);
    setText("diagnostics-status", "Playlist-ы недоступны");
  } finally {
    playlistListLoading = false;
    renderPlaylistPanel();
  }
}

async function loadPresetIntoEditor(presetId: string): Promise<void> {
  setText("editor-status", "Загружаем preset в редактор...");

  try {
    const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}`, { headers: buildApiHeaders() });
    const payload = (await response.json()) as PresetPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setEditorValue(payload.source);
    activeEditingPresetId = payload.id;
    activeEditingEffectName = readEffectName(payload.source) || payload.name;
    setText("editor-hint", `Открыт preset «${payload.name}». Можно править код, проверять и пересохранять.`);
    setText("editor-status", "Preset загружен в редактор.");
    setText("diagnostics-summary", `Открыт preset «${payload.name}». Теперь можно изменить код и сохранить обновление.`);
    setText("diagnostics-status", `Preset id: ${payload.id}`);
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось открыть preset.");
    setText("diagnostics-summary", `Не удалось открыть preset: ${message}`);
    setText("diagnostics-status", "Ошибка загрузки preset-а");
  }
}

async function activatePreset(presetId: string): Promise<void> {
  setText("editor-status", "Активируем preset на лампе...");

  try {
    const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}/activate`, {
      method: "POST",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as { ok?: boolean; error?: string; activePresetId?: string };
    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("editor-status", "Preset отправлен на лампу.");
    setText("diagnostics-summary", `Preset «${presetId}» активирован на лампе.`);
    setText("diagnostics-status", "Preset активирован");
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось активировать preset.");
    setText("diagnostics-summary", `Не удалось активировать preset: ${message}`);
    setText("diagnostics-status", "Активация не удалась");
  }
}

async function deletePreset(presetId: string, presetName: string): Promise<void> {
  const confirmed = window.confirm(`Удалить preset «${presetName}»?`);
  if (!confirmed) {
    return;
  }

  setText("editor-status", "Удаляем preset...");

  try {
    const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}`, {
      method: "DELETE",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as { ok?: boolean; error?: string };
    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    if (activeEditingPresetId === presetId) {
      activeEditingPresetId = "";
      activeEditingEffectName = "";
    }

    setText("editor-status", "Preset удалён.");
    setText("diagnostics-summary", `Preset «${presetName}» удалён из памяти лампы.`);
    setText("diagnostics-status", "Preset удалён");
    await loadPresetList();
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось удалить preset.");
    setText("diagnostics-summary", `Не удалось удалить preset: ${message}`);
    setText("diagnostics-status", "Удаление не удалось");
  }
}

async function startPlaylist(playlistId: string): Promise<void> {
  setText("editor-status", "Запускаем playlist на лампе...");

  try {
    const response = await fetch(`/api/playlists/${encodeURIComponent(playlistId)}/start`, {
      method: "POST",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as { ok?: boolean; error?: string };
    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("editor-status", "Playlist запущен.");
    setText("diagnostics-summary", `Очередь «${getPlaylistNameById(playlistId)}» запущена на лампе.`);
    setText("diagnostics-status", "Playlist активен");
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось запустить playlist.");
    setText("diagnostics-summary", `Не удалось запустить playlist: ${message}`);
    setText("diagnostics-status", "Запуск не удался");
  }
}

async function stopPlaylist(): Promise<void> {
  setText("editor-status", "Останавливаем очередь...");

  try {
    const response = await fetch("/api/playlists/stop", {
      method: "POST",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as { ok?: boolean; error?: string };
    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("editor-status", "Очередь остановлена.");
    setText("diagnostics-summary", "Автосмена остановлена. Лампа вернулась в ручной режим.");
    setText("diagnostics-status", "Playlist остановлен");
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось остановить очередь.");
    setText("diagnostics-summary", `Не удалось остановить playlist: ${message}`);
    setText("diagnostics-status", "Остановка не удалась");
  }
}

async function deletePlaylist(playlistId: string, playlistName: string): Promise<void> {
  const confirmed = window.confirm(`Удалить playlist «${playlistName}»?`);
  if (!confirmed) {
    return;
  }

  setText("editor-status", "Удаляем playlist...");

  try {
    const response = await fetch(`/api/playlists/${encodeURIComponent(playlistId)}`, {
      method: "DELETE",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as { ok?: boolean; error?: string };
    if (!response.ok || !payload.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("editor-status", "Playlist удалён.");
    setText("diagnostics-summary", `Playlist «${playlistName}» удалён.`);
    setText("diagnostics-status", "Playlist удалён");
    if (activeEditingPlaylistId === playlistId) {
      closePlaylistEditor();
    }
    await loadPlaylistList();
    await refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("editor-status", "Не удалось удалить playlist.");
    setText("diagnostics-summary", `Не удалось удалить playlist: ${message}`);
    setText("diagnostics-status", "Удаление не удалось");
  }
}

function bindPresetListActions(): void {
  const container = document.getElementById("saved-presets-list");
  container?.addEventListener("click", (event) => {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
      return;
    }

    const button = target.closest<HTMLButtonElement>("[data-preset-action]");
    if (!button) {
      return;
    }

    const presetId = button.dataset.presetId;
    const action = button.dataset.presetAction;
    if (!presetId || !action) {
      return;
    }

    if (action === "open") {
      void loadPresetIntoEditor(presetId);
      return;
    }

    if (action === "activate") {
      void activatePreset(presetId);
      return;
    }

    if (action === "delete") {
      void deletePreset(presetId, button.dataset.presetName || presetId);
    }
  });
}

function bindPlaylistActions(): void {
  const container = document.getElementById("saved-playlists-list");
  const stopButton = document.getElementById("stop-playlist-button") as HTMLButtonElement | null;
  const newPlaylistButton = document.getElementById("new-playlist-button") as HTMLButtonElement | null;
  const editorHost = document.getElementById("playlist-editor-host");

  container?.addEventListener("click", (event) => {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
      return;
    }

    const button = target.closest<HTMLButtonElement>("[data-playlist-action]");
    if (!button) {
      return;
    }

    const playlistId = button.dataset.playlistId;
    const action = button.dataset.playlistAction;
    if (!playlistId || !action) {
      return;
    }

    if (action === "edit") {
      openPlaylistEditor(playlistId);
      return;
    }

    if (action === "start") {
      void startPlaylist(playlistId);
      return;
    }

    if (action === "delete") {
      void deletePlaylist(playlistId, button.dataset.playlistName || playlistId);
    }
  });

  stopButton?.addEventListener("click", () => {
    void stopPlaylist();
  });

  newPlaylistButton?.addEventListener("click", () => {
    openNewPlaylistEditor();
  });

  editorHost?.addEventListener("click", (event) => {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
      return;
    }

    const button = target.closest<HTMLButtonElement>("[data-playlist-editor-action]");
    if (!button) {
      return;
    }

    const action = button.dataset.playlistEditorAction;
    if (!action) {
      return;
    }

    if (action === "add-entry") {
      appendPlaylistEntry();
      return;
    }

    if (action === "remove-entry") {
      const index = Number(button.dataset.entryIndex || -1);
      if (index >= 0) {
        removePlaylistEntry(index);
      }
      return;
    }

    if (action === "save") {
      void savePlaylistDraft();
      return;
    }

    if (action === "cancel") {
      closePlaylistEditor();
    }
  });
}

function bindActionButtons(): void {
  const newEffectButton = document.getElementById("new-effect-button") as HTMLButtonElement | null;
  const validateButton = document.getElementById("validate-button") as HTMLButtonElement | null;
  const runButton = document.getElementById("run-button") as HTMLButtonElement | null;
  const saveButton = document.getElementById("save-button") as HTMLButtonElement | null;

  newEffectButton?.addEventListener("click", () => {
    createNewEffect();
  });

  validateButton?.addEventListener("click", () => {
    void postLiveAction("/api/live/validate");
  });

  runButton?.addEventListener("click", () => {
    void postLiveAction("/api/live/run");
  });

  saveButton?.addEventListener("click", () => {
    void savePreset();
  });
}

function describeUpdateState(state: UpdateCurrentPayload["updateState"]): string {
  switch (state) {
    case "checking":
      return "Проверяем релизы";
    case "up-to-date":
      return "Свежая версия уже стоит";
    case "available":
      return "Есть новая прошивка";
    case "installing":
      return "Ставим обновление";
    case "completed":
      return "Обновление завершено";
    case "error":
      return "Ошибка обновления";
    case "idle":
    default:
      return "Ждём ручную проверку";
  }
}

function wait(milliseconds: number): Promise<void> {
  return new Promise((resolve) => {
    window.setTimeout(resolve, milliseconds);
  });
}

async function fetchUpdateStateSnapshot(): Promise<UpdateCurrentPayload> {
  const response = await fetch("/api/update/current", { headers: buildApiHeaders() });
  if (!response.ok) {
    const payload = (await response.json().catch(() => null)) as { error?: string } | null;
    throw new Error(payload?.error || `HTTP ${response.status}`);
  }

  return (await response.json()) as UpdateCurrentPayload;
}

function renderUpdateRebootProgress(attempt: number, maxAttempts: number, extraMessage = ""): void {
  const suffix = extraMessage ? ` ${extraMessage}` : "";
  setText("update-runtime-state", "Ждём reboot");
  setText("update-summary", `Лампа применяет OTA и перезагружается. Восстанавливаем связь, попытка ${attempt}/${maxAttempts}.${suffix}`);
  setText("update-status-note", `Перезагрузка... попытка ${attempt}/${maxAttempts}`);
  setText("update-error", "Короткая потеря связи после OTA здесь ожидаема.");
}

async function waitForUpdateRecovery(expectedVersion: string): Promise<void> {
  const maxAttempts = 5;
  const pauseMs = 2500;

  for (let attempt = 1; attempt <= maxAttempts; attempt += 1) {
    updateRebootAttempt = attempt;
    renderUpdateRebootProgress(attempt, maxAttempts);
    await wait(pauseMs);

    try {
      const payload = await fetchUpdateStateSnapshot();
      renderUpdateState(payload);

      if (payload.version === expectedVersion) {
        updateRebootPending = false;
        updateRebootAttempt = 0;
        setText("update-summary", `Устройство вернулось после перезагрузки. Теперь установлена версия ${payload.version}.`);
        setText("update-status-note", "OTA завершено, связь восстановлена");
        setText("update-error", "Ошибок нет.");
        void refreshStatus();
        return;
      }

      if (payload.updateState !== "completed") {
        updateRebootPending = false;
        updateRebootAttempt = 0;
        setText(
          "update-summary",
          `Связь восстановилась, но устройство ещё сообщает состояние ${describeUpdateState(payload.updateState).toLowerCase()}. Проверь версию ещё раз вручную.`,
        );
        setText("update-status-note", "Устройство снова отвечает");
        void refreshStatus();
        return;
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : "unknown error";
      renderUpdateRebootProgress(attempt, maxAttempts, `Последний ответ: ${message}.`);
    }
  }

  updateRebootPending = false;
  updateRebootAttempt = 0;
  setText("update-summary", "Лампа всё ещё не ответила после OTA. Обнови окно позже или проверь устройство вручную.");
  setText("update-status-note", "Не дождались устройство после reboot");
  setText("update-error", "Связь не восстановилась за 5 попыток.");
}

function describeNetworkMode(mode: NetworkSettingsPayload["mode"]): string {
  return mode === "client"
    ? "В режиме клиента лампа подключается к домашнему Wi-Fi и может получать OTA через интернет."
    : "В режиме точки доступа лампа поднимает свою сеть и ждёт подключения напрямую.";
}

function setElementDisabled(id: string, disabled: boolean): void {
  const node = document.getElementById(id) as HTMLButtonElement | HTMLSelectElement | HTMLInputElement | null;
  if (node) {
    node.disabled = disabled;
  }
}

function setInputValue(id: string, value: string): void {
  const node = document.getElementById(id) as HTMLInputElement | HTMLSelectElement | null;
  if (node) {
    node.value = value;
  }
}

function toggleElementHidden(id: string, hidden: boolean): void {
  const node = document.getElementById(id);
  if (node) {
    node.hidden = hidden;
  }
}

function getNetworkModeValue(): NetworkSettingsPayload["mode"] {
  const select = document.getElementById("network-mode-select") as HTMLSelectElement | null;
  return select?.value === "client" ? "client" : "ap";
}

function syncNetworkModeFields(): void {
  const mode = getNetworkModeValue();
  const disableClientFields = mode === "ap";
  const lockForm = networkModalLoading || networkModalSaving || !networkSettingsLoaded;
  setElementDisabled("network-ssid-input", disableClientFields || lockForm);
  setElementDisabled("network-password-input", disableClientFields || lockForm);
  setElementDisabled("network-mode-select", lockForm);
  setElementDisabled("network-ap-name-input", lockForm);
  setElementDisabled("network-save-button", lockForm);
  setElementDisabled("network-cancel-button", networkModalSaving);
  setElementDisabled("network-close-button", networkModalSaving);
  setText("network-mode-hint", describeNetworkMode(mode));
}

function applyNetworkSettingsToForm(settings: NetworkSettingsPayload): void {
  currentNetworkSettings = settings;
  setInputValue("network-mode-select", settings.mode);
  setInputValue("network-ap-name-input", settings.accessPointName);
  setInputValue("network-ssid-input", settings.clientSsid);
  setInputValue("network-password-input", "");
  syncNetworkModeFields();
}

function readNetworkForm(): NetworkSettingsPayload & { clientPassword: string } {
  const apName = (document.getElementById("network-ap-name-input") as HTMLInputElement | null)?.value?.trim() || "";
  const ssid = (document.getElementById("network-ssid-input") as HTMLInputElement | null)?.value?.trim() || "";
  const password = (document.getElementById("network-password-input") as HTMLInputElement | null)?.value || "";
  const mode = getNetworkModeValue();

  return {
    mode,
    accessPointName: apName,
    clientSsid: mode === "client" ? ssid : "",
    clientPassword: mode === "client" ? password : "",
  };
}

function openNetworkModalShell(): void {
  networkModalOpen = true;
  networkSettingsLoaded = false;
  toggleElementHidden("network-modal", false);
  syncNetworkModeFields();
}

function closeNetworkModal(): void {
  if (networkModalSaving) {
    return;
  }

  networkModalOpen = false;
  toggleElementHidden("network-modal", true);
}

async function refreshNetworkSettings(): Promise<void> {
  networkModalLoading = true;
  networkSettingsLoaded = false;
  syncNetworkModeFields();
  setText("network-summary", "Читаем текущие настройки сети с лампы...");
  setText("network-settings-status", "Загружаем настройки сети");

  try {
    const response = await fetch("/api/settings/network", { headers: buildApiHeaders() });
    const payload = (await response.json()) as NetworkSettingsPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    applyNetworkSettingsToForm(payload);
    networkSettingsLoaded = true;
    setText("network-summary", "Настройки загружены. Можно переключить режим и сохранить новую конфигурацию.");
    setText("network-settings-status", payload.mode === "client" ? "Лампа настроена как Wi-Fi клиент" : "Лампа работает как точка доступа");
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("network-summary", `Не удалось загрузить настройки сети: ${message}`);
    setText("network-settings-status", "Настройки сети недоступны");
  } finally {
    networkModalLoading = false;
    syncNetworkModeFields();
  }
}

async function openNetworkSettingsModal(): Promise<void> {
  openNetworkModalShell();
  await refreshNetworkSettings();
}

async function saveNetworkSettings(): Promise<void> {
  if (!networkSettingsLoaded) {
    setText("network-summary", "Сначала дождись успешной загрузки текущих настроек с лампы.");
    setText("network-settings-status", "Сохранение заблокировано до успешного чтения");
    return;
  }

  const form = readNetworkForm();
  if (form.mode === "client" && !form.clientPassword) {
    setText("network-summary", "Для client-режима пароль нужно ввести заново перед сохранением, иначе лампа потеряет доступ к сети.");
    setText("network-settings-status", "Введите пароль Wi-Fi для сохранения");
    return;
  }

  networkModalSaving = true;
  syncNetworkModeFields();
  setText("network-summary", "Сохраняем настройки сети. Лампа может временно переподключиться.");
  setText("network-settings-status", "Отправляем новые сетевые параметры");

  try {
    const response = await fetch("/api/settings/network", {
      method: "POST",
      headers: buildFormHeaders(),
      body: buildFormBody({
        mode: form.mode,
        accessPointName: form.accessPointName,
        clientSsid: form.clientSsid,
        clientPassword: form.clientPassword,
      }),
    });
    const payload = (await response.json()) as NetworkSettingsPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    applyNetworkSettingsToForm(payload);
    networkSettingsLoaded = true;
    setText(
      "network-summary",
      payload.mode === "client"
        ? `Сеть сохранена. Лампа попробует подключиться к ${payload.clientSsid || "выбранной сети"}.`
        : `Сеть сохранена. Лампа останется в режиме точки доступа ${payload.accessPointName}.`,
    );
    setText("network-settings-status", "Настройки сети сохранены");
    void refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("network-summary", `Не удалось сохранить сеть: ${message}`);
    setText("network-settings-status", "Ошибка при сохранении сети");
  } finally {
    networkModalSaving = false;
    syncNetworkModeFields();
  }
}

function syncUpdateControls(): void {
  const busy = updateBusyAction !== "";
  setElementDisabled("update-channel-select", busy || updateBusyAction === "install");
  setElementDisabled("update-check-button", busy);
  setElementDisabled(
    "update-install-button",
    busy || !currentUpdateSnapshot || currentUpdateSnapshot.updateState !== "available" || !currentUpdateSnapshot.availableVersion,
  );
}

function renderUpdateState(snapshot: UpdateCurrentPayload): void {
  currentUpdateSnapshot = snapshot;
  if (snapshot.updateState !== "completed") {
    updateRebootPending = false;
  }

  const summary = snapshot.updateError
    ? `OTA сообщает об ошибке: ${snapshot.updateError}`
    : snapshot.updateState === "completed"
      ? "Прошивка записана. Устройство уходит в перезапуск, браузер может временно потерять связь."
      : snapshot.updateState === "available" && snapshot.availableVersion
        ? `Найдена версия ${snapshot.availableVersion}. Можно установить прямо из браузера.`
        : snapshot.updateState === "up-to-date"
          ? "На выбранном канале уже стоит актуальная версия."
          : "Пока всё спокойно. Канал можно переключить и проверить релизы вручную.";

  const errorText = snapshot.updateError
    ? snapshot.updateError
    : snapshot.updateState === "completed"
      ? "Это ожидаемое состояние после успешной OTA установки."
      : snapshot.updateState === "available"
        ? "Можно ставить вручную."
        : "Ошибок нет.";

  setText("update-version", snapshot.version || "-");
  setText("update-channel", snapshot.updateChannel || snapshot.channel || "-");
  setText("update-runtime-state", describeUpdateState(snapshot.updateState));
  setText("update-available-version", snapshot.availableVersion || "Обновлений нет");
  setText("update-error", errorText);
  setText("update-summary", summary);
  setText("update-status-note", describeUpdateState(snapshot.updateState));
  setInputValue("update-channel-select", snapshot.updateChannel || snapshot.channel || "stable");
  syncUpdateControls();
}

async function refreshUpdateState(): Promise<void> {
  try {
    const payload = await fetchUpdateStateSnapshot();
    renderUpdateState(payload);
  } catch (error) {
    const message = error instanceof Error ? error.message : "unknown error";
    if (updateRebootPending) {
      const attempt = updateRebootAttempt > 0 ? updateRebootAttempt : 1;
      renderUpdateRebootProgress(attempt, 5, `Последний ответ: ${message}.`);
      setText("update-error", "Связь временно недоступна из-за перезапуска.");
      return;
    }

    setText("update-summary", "Не получилось обновить OTA-сводку.");
    setText("update-status-note", message);
    setText("update-error", message);
  }
}

async function saveUpdateChannel(): Promise<void> {
  const select = document.getElementById("update-channel-select") as HTMLSelectElement | null;
  const channel = select?.value === "stable" ? "stable" : "dev";

  updateBusyAction = "settings";
  syncUpdateControls();
  setText("update-status-note", "Сохраняем канал обновлений...");

  try {
    const response = await fetch("/api/update/settings", {
      method: "POST",
      headers: buildFormHeaders(),
      body: buildFormBody({ channel }),
    });
    const payload = (await response.json()) as { channel?: string; error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("update-summary", `Канал обновлений переключен на ${payload.channel || channel}.`);
    await refreshUpdateState();
    void refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("update-summary", `Не удалось сменить канал: ${message}`);
    setText("update-status-note", "Смена канала не удалась");
    await refreshUpdateState();
  } finally {
    updateBusyAction = "";
    syncUpdateControls();
  }
}

async function checkForUpdates(): Promise<void> {
  const select = document.getElementById("update-channel-select") as HTMLSelectElement | null;
  const channel = select?.value === "stable" ? "stable" : "dev";

  updateBusyAction = "check";
  syncUpdateControls();
  setText("update-status-note", "Проверяем GitHub Releases...");

  try {
    const response = await fetch("/api/update/check", {
      method: "POST",
      headers: buildFormHeaders(),
      body: buildFormBody({ channel }),
    });
    const payload = (await response.json()) as UpdateCheckPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText(
      "update-summary",
      payload.hasUpdate
        ? `Найдена новая версия ${payload.version}. Можно нажимать «Установить».`
        : payload.error || "Подходящих обновлений сейчас нет.",
    );
    await refreshUpdateState();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("update-summary", `Проверка не удалась: ${message}`);
    setText("update-status-note", "Ошибка проверки OTA");
    await refreshUpdateState();
  } finally {
    updateBusyAction = "";
    syncUpdateControls();
  }
}

async function installUpdate(): Promise<void> {
  if (!currentUpdateSnapshot?.availableVersion) {
    return;
  }

  const expectedVersion = currentUpdateSnapshot.availableVersion;

  updateBusyAction = "install";
  syncUpdateControls();
  setText("update-status-note", "Скачиваем и ставим прошивку...");

  try {
    const response = await fetch("/api/update/install", {
      method: "POST",
      headers: buildApiHeaders(),
    });
    const payload = (await response.json()) as UpdateInstallPayload;
    if (!response.ok || !payload.success) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    setText("update-summary", `Прошивка ${currentUpdateSnapshot.availableVersion} установлена. Устройство перезагружается.`);
    setText("update-status-note", "Ждём перезапуск устройства");
    updateRebootPending = true;
    updateRebootAttempt = 0;
    await waitForUpdateRecovery(expectedVersion);
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    updateRebootPending = false;
    updateRebootAttempt = 0;
    setText("update-summary", `Установка не удалась: ${message}`);
    setText("update-status-note", "OTA установка завершилась с ошибкой");
    await refreshUpdateState();
  } finally {
    updateBusyAction = "";
    syncUpdateControls();
  }
}

function bindUpdateControls(): void {
  const select = document.getElementById("update-channel-select") as HTMLSelectElement | null;
  const checkButton = document.getElementById("update-check-button") as HTMLButtonElement | null;
  const installButton = document.getElementById("update-install-button") as HTMLButtonElement | null;

  select?.addEventListener("change", () => {
    void saveUpdateChannel();
  });

  checkButton?.addEventListener("click", () => {
    void checkForUpdates();
  });

  installButton?.addEventListener("click", () => {
    void installUpdate();
  });

  syncUpdateControls();
}

app.innerHTML = renderShellMarkup({
  isDevServer,
  selectedScenario,
  scenarioOptions: scenarioDefinitions
    .map(
      (scenario) =>
        `<option value="${scenario.id}"${scenario.id === selectedScenario ? " selected" : ""}>${scenario.label}</option>`,
    )
    .join(""),
  starterSnippetList: renderStarterSnippetList(),
  helpSections: renderHelpSections(),
  networkModalMarkup: renderNetworkModal(),
  firmwareModalMarkup: renderFirmwareModal(),
  timeModalMarkup: renderTimeModal(),
});

function setText(id: string, value: string): void {
  const node = document.getElementById(id);
  if (node) {
    node.textContent = value;
  }
}

function setEditorValue(value: string): void {
  const editor = document.getElementById("editor-code") as HTMLTextAreaElement | null;
  if (editor) {
    editor.value = value;
  }
}

function setSelectedScenario(nextScenario: ScenarioId): void {
  selectedScenario = nextScenario;
  localStorage.setItem(devScenarioStorageKey, nextScenario);
  const url = new URL(window.location.href);
  url.searchParams.set("scenario", nextScenario);
  window.history.replaceState({}, "", url);
}

function buildApiHeaders(): Record<string, string> {
  if (!isDevServer) {
    return { Accept: "application/json" };
  }

  return {
    Accept: "application/json",
    "X-Dev-Scenario": selectedScenario,
  };
}

function findSnippetById(snippetId: string): StarterSnippet | undefined {
  return starterSnippets.find((snippet) => snippet.id === snippetId);
}

function applySnippet(snippet: StarterSnippet): void {
  setEditorValue(snippet.source);
  activeEditingPresetId = "";
  activeEditingEffectName = "";
  setText("editor-hint", `${snippet.name}: ${snippet.description}`);
  setText("editor-status", "Идея загружена. Теперь можно менять код и смотреть, что получится.");
  setText("diagnostics-summary", `Загружена идея «${snippet.name}». Проверь код и нажми «Запустить», когда будешь готов.`);
}

function bindSnippetButtons(): void {
  const buttons = document.querySelectorAll<HTMLButtonElement>("[data-snippet-id]");
  buttons.forEach((button) => {
    button.addEventListener("click", () => {
      const snippetId = button.dataset.snippetId;
      if (!snippetId) {
        return;
      }

      const snippet = findSnippetById(snippetId);
      if (snippet) {
        applySnippet(snippet);
      }
    });
  });
}

function renderScenarioDescription(): void {
  const description = scenarioDefinitions.find((scenario) => scenario.id === selectedScenario)?.description;
  setText("dev-scenario-description", description ?? "");
}

function bindDevScenarioControls(): void {
  if (!isDevServer) {
    return;
  }

  renderScenarioDescription();

  const select = document.getElementById("dev-scenario-select") as HTMLSelectElement | null;
  const resetButton = document.getElementById("dev-reset-button") as HTMLButtonElement | null;

  select?.addEventListener("change", () => {
    const nextValue = select.value;
    if (!isScenarioId(nextValue)) {
      return;
    }

    setSelectedScenario(nextValue);
    renderScenarioDescription();
    void loadPresetList();
    void loadPlaylistList();
    void refreshStatus();
    void refreshUpdateState();
  });

  resetButton?.addEventListener("click", async () => {
    await fetch("/__dev/reset", {
      method: "POST",
      headers: buildApiHeaders(),
    });
    void loadPresetList();
    void loadPlaylistList();
    void refreshStatus();
    void refreshUpdateState();
  });
}

function bindEditorFocusHints(): void {
  const editor = document.getElementById("editor-code") as HTMLTextAreaElement | null;
  if (!editor) {
    return;
  }

  editor.addEventListener("focus", () => {
    setText("editor-status", "Можно печатать. Курсор уже в коде.");
  });

  editor.addEventListener("blur", () => {
    setText("editor-status", "Редактор не выбран. Кликни в код, чтобы продолжить.");
  });

  editor.addEventListener("input", () => {
    setText("editor-status", "Есть новые правки. Можно проверить, запустить или сохранить идею.");
  });
}

function bindSidebarTabs(): void {
  const tabs = document.querySelectorAll<HTMLButtonElement>(".sidebar-tab");
  tabs.forEach((tab) => {
    tab.addEventListener("click", () => {
      const targetId = tab.dataset.tab;
      if (!targetId) {
        return;
      }

      tabs.forEach((t) => {
        t.classList.remove("sidebar-tab--active");
        t.setAttribute("aria-selected", "false");
      });
      tab.classList.add("sidebar-tab--active");
      tab.setAttribute("aria-selected", "true");

      document.querySelectorAll<HTMLElement>(".sidebar-panel").forEach((panel) => {
        const isTarget = panel.id === `tab-${targetId}`;
        panel.hidden = !isTarget;
        panel.classList.toggle("sidebar-panel--active", isTarget);
      });
    });
  });
}

function bindNetworkSettingsControls(): void {
  const openButton = document.getElementById("network-settings-button") as HTMLButtonElement | null;
  const closeButton = document.getElementById("network-close-button") as HTMLButtonElement | null;
  const cancelButton = document.getElementById("network-cancel-button") as HTMLButtonElement | null;
  const saveButton = document.getElementById("network-save-button") as HTMLButtonElement | null;
  const modeSelect = document.getElementById("network-mode-select") as HTMLSelectElement | null;
  const modal = document.getElementById("network-modal") as HTMLDivElement | null;
  const backdrop = modal?.querySelector<HTMLElement>("[data-network-close='overlay']") ?? null;

  openButton?.addEventListener("click", () => {
    void openNetworkSettingsModal();
  });

  closeButton?.addEventListener("click", () => {
    closeNetworkModal();
  });

  cancelButton?.addEventListener("click", () => {
    closeNetworkModal();
  });

  saveButton?.addEventListener("click", () => {
    void saveNetworkSettings();
  });

  modeSelect?.addEventListener("change", () => {
    syncNetworkModeFields();
  });

  backdrop?.addEventListener("click", () => {
    closeNetworkModal();
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && networkModalOpen) {
      closeNetworkModal();
    }
  });

  syncNetworkModeFields();
}

function syncTimeSettingsControls(): void {
  const lockForm = timeModalLoading || timeModalSaving || !timeSettingsLoaded;
  setElementDisabled("time-timezone-select", lockForm);
  setElementDisabled("time-save-button", lockForm);
  setElementDisabled("time-cancel-button", timeModalSaving);
  setElementDisabled("time-close-button", timeModalSaving);
}

function applyTimeSettingsToForm(settings: TimeSettingsPayload): void {
  currentTimeSettings = settings;
  setInputValue("time-timezone-select", settings.timezone);
  syncTimeSettingsControls();
}

function openTimeModalShell(): void {
  timeModalOpen = true;
  timeSettingsLoaded = false;
  toggleElementHidden("time-modal", false);
  syncTimeSettingsControls();
}

function closeTimeModal(): void {
  if (timeModalSaving) {
    return;
  }

  timeModalOpen = false;
  toggleElementHidden("time-modal", true);
}

async function refreshTimeSettings(): Promise<void> {
  timeModalLoading = true;
  timeSettingsLoaded = false;
  syncTimeSettingsControls();
  setText("time-summary", "Читаем текущий часовой пояс с лампы...");
  setText("time-settings-status", "Загружаем настройки времени");

  try {
    const response = await fetch("/api/settings/time", { headers: buildApiHeaders() });
    const payload = (await response.json()) as TimeSettingsPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    applyTimeSettingsToForm(payload);
    timeSettingsLoaded = true;
    setText("time-summary", "Часовой пояс загружен. Можно выбрать новый и сохранить его на лампе.");
    setText("time-settings-status", payload.timezone);
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("time-summary", `Не удалось загрузить настройки времени: ${message}`);
    setText("time-settings-status", "Настройки времени недоступны");
  } finally {
    timeModalLoading = false;
    syncTimeSettingsControls();
  }
}

async function openTimeSettingsModal(): Promise<void> {
  openTimeModalShell();
  await refreshTimeSettings();
}

async function saveTimeSettings(): Promise<void> {
  if (!timeSettingsLoaded) {
    setText("time-summary", "Сначала дождись успешной загрузки текущих настроек времени.");
    setText("time-settings-status", "Сохранение заблокировано до успешного чтения");
    return;
  }

  const timezone = (document.getElementById("time-timezone-select") as HTMLSelectElement | null)?.value || "UTC0";

  timeModalSaving = true;
  syncTimeSettingsControls();
  setText("time-summary", "Сохраняем часовой пояс и просим лампу пересчитать время.");
  setText("time-settings-status", "Отправляем настройки времени");

  try {
    const response = await fetch("/api/settings/time", {
      method: "POST",
      headers: buildFormHeaders(),
      body: buildFormBody({ timezone }),
    });
    const payload = (await response.json()) as TimeSettingsPayload & { error?: string };
    if (!response.ok) {
      throw new Error(payload.error || `HTTP ${response.status}`);
    }

    applyTimeSettingsToForm(payload);
    timeSettingsLoaded = true;
    setText("time-summary", `Часовой пояс сохранён: ${payload.timezone}.`);
    setText("time-settings-status", "Настройки времени сохранены");
    void refreshStatus();
  } catch (error) {
    const message = error instanceof Error ? error.message : "Неизвестная ошибка";
    setText("time-summary", `Не удалось сохранить время: ${message}`);
    setText("time-settings-status", "Ошибка при сохранении времени");
  } finally {
    timeModalSaving = false;
    syncTimeSettingsControls();
  }
}

function openFirmwareModal(): void {
  firmwareModalOpen = true;
  toggleElementHidden("firmware-modal", false);
}

function closeFirmwareModal(): void {
  firmwareModalOpen = false;
  toggleElementHidden("firmware-modal", true);
}

function bindFirmwareModalControls(): void {
  const openButton = document.getElementById("firmware-settings-button") as HTMLButtonElement | null;
  const closeButton = document.getElementById("firmware-close-button") as HTMLButtonElement | null;
  const modal = document.getElementById("firmware-modal") as HTMLDivElement | null;
  const backdrop = modal?.querySelector<HTMLElement>("[data-firmware-close='overlay']") ?? null;

  openButton?.addEventListener("click", () => {
    openFirmwareModal();
  });

  closeButton?.addEventListener("click", () => {
    closeFirmwareModal();
  });

  backdrop?.addEventListener("click", () => {
    closeFirmwareModal();
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && firmwareModalOpen) {
      closeFirmwareModal();
    }
  });
}

function bindTimeSettingsControls(): void {
  const openButton = document.getElementById("statusbar-clock-action") as HTMLButtonElement | null;
  const headerOpenButton = document.getElementById("time-settings-button") as HTMLButtonElement | null;
  const closeButton = document.getElementById("time-close-button") as HTMLButtonElement | null;
  const cancelButton = document.getElementById("time-cancel-button") as HTMLButtonElement | null;
  const saveButton = document.getElementById("time-save-button") as HTMLButtonElement | null;
  const modal = document.getElementById("time-modal") as HTMLDivElement | null;
  const backdrop = modal?.querySelector<HTMLElement>("[data-time-close='overlay']") ?? null;

  openButton?.addEventListener("click", () => {
    void openTimeSettingsModal();
  });

  headerOpenButton?.addEventListener("click", () => {
    void openTimeSettingsModal();
  });

  closeButton?.addEventListener("click", () => {
    closeTimeModal();
  });

  cancelButton?.addEventListener("click", () => {
    closeTimeModal();
  });

  saveButton?.addEventListener("click", () => {
    void saveTimeSettings();
  });

  backdrop?.addEventListener("click", () => {
    closeTimeModal();
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && timeModalOpen) {
      closeTimeModal();
    }
  });

  syncTimeSettingsControls();
}

function formatNumber(value: number | null, suffix: string): string {
  if (value === null || Number.isNaN(value)) {
    return "-";
  }

  return `${value}${suffix}`;
}

function renderStatus(status: StatusPayload): void {
  currentStatus = status;
  setText("statusbar-build", `${status.version} · ${status.channel}`);
  setText("statusbar-preset", status.activePresetName || status.activePresetId || "Пусто");
  setText("statusbar-autoplay", status.autoplayEnabled ? "Вкл" : "Выкл");
  setText("statusbar-playlist", getPlaylistNameById(status.activePlaylistId) || "Нет");
  setText("statusbar-network", status.networkStatus || status.networkMode || "-");
  setText("statusbar-clock", status.currentTime || status.clockStatus || "-");
  setText("statusbar-sensor", status.sensorStatus || "-");
  setText("statusbar-temp", formatNumber(status.temperatureC, " °C"));
  setText("statusbar-humidity", formatNumber(status.humidityPercent, " %"));
  setText("runtime-preset", status.activePresetName || status.activePresetId || "Пока ничего не выбрано");
  setText("runtime-autoplay", status.autoplayEnabled ? "Включено" : "Выключено");
  setText("runtime-playlist", getPlaylistNameById(status.activePlaylistId) || "Очередь пока не включена");
  setText("runtime-effect", status.activeEffect || "- ");
  setText(
    "diagnostics-summary",
    status.liveErrorSummary || "Пока всё спокойно. Можно пробовать новые идеи и смотреть, как они оживают.",
  );
  setText("diagnostics-status", status.liveErrorSummary ? "Нужно чуть поправить код" : "Лампа готова показывать новые огоньки");
  renderPresetList();
  renderPlaylistPanel();
}

async function refreshStatus(): Promise<void> {
  try {
    const response = await fetch("/api/status", { headers: buildApiHeaders() });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const status = (await response.json()) as StatusPayload;
    renderStatus(status);
  } catch (error) {
    const message = error instanceof Error ? error.message : "unknown error";
    setText("statusbar-build", "Статус недоступен");
    setText("diagnostics-summary", "Не получилось поговорить с лампой и обновить статус.");
    setText("diagnostics-status", message);
  }
}

void refreshStatus();
void refreshUpdateState();
void loadPresetList();
void loadPlaylistList();
applySnippet(starterSnippets[0]);
bindSnippetButtons();
bindActionButtons();
bindPresetListActions();
bindPlaylistActions();
bindUpdateControls();
bindNetworkSettingsControls();
bindFirmwareModalControls();
bindTimeSettingsControls();
bindDevScenarioControls();
bindEditorFocusHints();
bindSidebarTabs();
window.setInterval(() => {
  void refreshStatus();
  void refreshUpdateState();
}, 5000);