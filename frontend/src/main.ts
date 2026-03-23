import "./styles/app.css";
import { editorHelpSections } from "./editor/help";
import { starterSnippets, type StarterSnippet } from "./editor/snippets";
import { defaultScenarioId, isScenarioId, scenarioDefinitions } from "./dev/mockScenarios";
import type { LiveDiagnosticResponse, ScenarioId, StatusPayload } from "./dev/mockTypes";

const app = document.querySelector<HTMLDivElement>("#app");

if (!app) {
  throw new Error("App root not found");
}

const devScenarioStorageKey = "mylamp-dev-scenario";
const isDevServer = import.meta.env.DEV;

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

function buildJsonHeaders(): Record<string, string> {
  return {
    ...buildApiHeaders(),
    "Content-Type": "application/json",
  };
}

function formatDiagnostics(response: LiveDiagnosticResponse): string {
  if (response.ok || response.errors.length === 0) {
    return "Ошибок не найдено.";
  }

  return response.errors
    .map((error) => `Строка ${error.line}, столбец ${error.column}: ${error.message}`)
    .join(" ");
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
    if (!response.ok) {
      throw new Error(formatDiagnostics(payload));
    }

    setText("diagnostics-summary", formatDiagnostics(payload));
    setText(
      "diagnostics-status",
      payload.ok
        ? endpoint === "/api/live/validate"
          ? "Проверка прошла успешно"
          : "Код отправлен на лампу"
        : "Найдены ошибки в DSL",
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
  const presetId = buildPresetId(effectName);

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
    const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}`, {
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
  setText("editor-hint", "Это пустая заготовка. Дай эффекту имя в строке effect \"...\" и начинай рисовать.");
  setText("editor-status", "Создан новый пустой эффект. Теперь можно редактировать, проверить или сохранить его.");
  setText("diagnostics-summary", "Новая заготовка открыта. Поменяй имя, спрайты и слои, потом сохрани эффект.");
  setText("diagnostics-status", "Черновик готов");
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

app.innerHTML = `
  <main class="shell">
    <header class="shell__header">
      <div>
        <p class="eyebrow">MyLamp</p>
        <h1>Моя Лампа</h1>
        <p class="shell__subtitle">Учимся лайвкодить</p>
      </div>
      <div class="shell__header-actions">
        ${isDevServer ? `
        <section class="dev-panel">
          <label class="dev-panel__label" for="dev-scenario-select">Сценарий для проверки</label>
          <select class="dev-panel__select" id="dev-scenario-select">
            ${scenarioDefinitions
              .map(
                (scenario) =>
                  `<option value="${scenario.id}"${scenario.id === selectedScenario ? " selected" : ""}>${scenario.label}</option>`,
              )
              .join("")}
          </select>
          <button class="dev-panel__button" id="dev-reset-button" type="button">Сбросить пример</button>
          <p class="dev-panel__description" id="dev-scenario-description"></p>
        </section>` : ""}
        <div class="status-pill" id="build-pill">Загрузка статуса...</div>
      </div>
    </header>

    <section class="shell__grid">
      <section class="panel panel--editor">
        <div class="panel__header">
          <h2>Рисуем огоньки</h2>
          <div class="panel__actions">
            <button id="new-effect-button" type="button">Новый эффект</button>
            <button id="validate-button" type="button">Проверить</button>
            <button id="run-button" type="button">Запустить</button>
            <button id="save-button" type="button">Сохранить</button>
          </div>
        </div>
        <div class="panel__body panel__body--editor">
          <div class="editor-toolbar">
            <div class="editor-toolbar__hint" id="editor-hint">Выбери идею справа и попробуй поменять цвета, форму или движение.</div>
            <div class="editor-toolbar__status" id="editor-status">Кликни в код и печатай. Курсор появится внутри поля.</div>
          </div>
          <label class="editor-surface" for="editor-code">
            <span class="editor-surface__badge">DSL</span>
            <textarea
              class="code-editor"
              id="editor-code"
              spellcheck="false"
              autocapitalize="off"
              autocomplete="off"
              autocorrect="off"
              placeholder="effect \"my_effect\"&#10;&#10;sprite dot {&#10;  bitmap \"\"\"&#10;  #&#10;  \"\"\"&#10;}&#10;&#10;layer paint {&#10;  use dot&#10;  color rgb(255, 120, 80)&#10;  x = 10&#10;  y = 6&#10;  scale = 2&#10;  visible = 1&#10;}"
            ></textarea>
          </label>
        </div>
      </section>

      <aside class="sidebar">
        <section class="panel panel--runtime">
          <div class="panel__header">
            <h2>Что сейчас горит</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <div class="key-value"><span>Сейчас включено</span><strong id="runtime-preset">-</strong></div>
            <div class="key-value"><span>Автосмена</span><strong id="runtime-autoplay">-</strong></div>
            <div class="key-value"><span>Очередь огоньков</span><strong id="runtime-playlist">-</strong></div>
            <div class="key-value"><span>Запасной режим</span><strong id="runtime-effect">-</strong></div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Подсказки</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <p id="diagnostics-summary">Здесь появятся подсказки, ошибки в коде и результат проверки.</p>
            <div class="status-note" id="diagnostics-status">Ждём новости от лампы.</div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Готовые идеи</h2>
          </div>
          <div class="panel__body">
            <ul class="item-list">${renderStarterSnippetList()}</ul>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Очередь огоньков</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <p>Лампа уже умеет сама переключать эффекты. Скоро здесь можно будет собирать свою очередь огоньков.</p>
            <div class="status-note">Если запустить эффект вручную, автосмена сразу остановится.</div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Шпаргалка по командам</h2>
          </div>
          <div class="panel__body panel__body--stack">${renderHelpSections()}</div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Как себя чувствует лампа</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <div class="key-value"><span>Сеть</span><strong id="lamp-network">-</strong></div>
            <div class="key-value"><span>Часы</span><strong id="lamp-clock">-</strong></div>
            <div class="key-value"><span>Сенсор</span><strong id="lamp-sensor">-</strong></div>
            <div class="key-value"><span>Температура</span><strong id="lamp-temp">-</strong></div>
            <div class="key-value"><span>Влажность</span><strong id="lamp-humidity">-</strong></div>
          </div>
        </section>
      </aside>
    </section>
  </main>
`;

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

function buildApiHeaders(): HeadersInit {
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
    void refreshStatus();
  });

  resetButton?.addEventListener("click", async () => {
    await fetch("/__dev/reset", {
      method: "POST",
      headers: buildApiHeaders(),
    });
    void refreshStatus();
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

function formatNumber(value: number | null, suffix: string): string {
  if (value === null || Number.isNaN(value)) {
    return "-";
  }

  return `${value}${suffix}`;
}

function renderStatus(status: StatusPayload): void {
  setText("build-pill", `${status.version} · ${status.channel}`);
  setText("runtime-preset", status.activePresetName || status.activePresetId || "Пока ничего не выбрано");
  setText("runtime-autoplay", status.autoplayEnabled ? "Включено" : "Выключено");
  setText("runtime-playlist", status.activePlaylistId || "Очередь пока не включена");
  setText("runtime-effect", status.activeEffect || "- ");
  setText(
    "diagnostics-summary",
    status.liveErrorSummary || "Пока всё спокойно. Можно пробовать новые идеи и смотреть, как они оживают.",
  );
  setText("diagnostics-status", status.liveErrorSummary ? "Нужно чуть поправить код" : "Лампа готова показывать новые огоньки");
  setText("lamp-network", status.networkStatus || status.networkMode || "-");
  setText("lamp-clock", status.currentTime || status.clockStatus || "-");
  setText("lamp-sensor", status.sensorStatus || "-");
  setText("lamp-temp", formatNumber(status.temperatureC, " °C"));
  setText("lamp-humidity", formatNumber(status.humidityPercent, " %"));
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
    setText("build-pill", "Статус недоступен");
    setText("diagnostics-summary", "Не получилось поговорить с лампой и обновить статус.");
    setText("diagnostics-status", message);
  }
}

void refreshStatus();
applySnippet(starterSnippets[0]);
bindSnippetButtons();
bindActionButtons();
bindDevScenarioControls();
bindEditorFocusHints();
window.setInterval(() => {
  void refreshStatus();
}, 5000);