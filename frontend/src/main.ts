import "./styles/app.css";
import { editorHelpSections } from "./editor/help";
import { starterSnippets, type StarterSnippet } from "./editor/snippets";

const app = document.querySelector<HTMLDivElement>("#app");

if (!app) {
  throw new Error("App root not found");
}

type StatusPayload = {
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

app.innerHTML = `
  <main class="shell">
    <header class="shell__header">
      <div>
        <p class="eyebrow">mylamp live coding</p>
        <h1>Editor-first shell</h1>
      </div>
      <div class="status-pill" id="build-pill">Загрузка статуса...</div>
    </header>

    <section class="shell__grid">
      <section class="panel panel--editor">
        <div class="panel__header">
          <h2>Редактор эффекта</h2>
          <div class="panel__actions">
            <button type="button">Проверить</button>
            <button type="button">Запустить</button>
            <button type="button">Сохранить</button>
          </div>
        </div>
        <div class="panel__body panel__body--editor">
          <div class="editor-toolbar">
            <div class="editor-toolbar__hint" id="editor-hint">Выбери шаблон справа, чтобы быстро начать.</div>
          </div>
          <pre class="code-block" id="editor-code"></pre>
        </div>
      </section>

      <aside class="sidebar">
        <section class="panel panel--runtime">
          <div class="panel__header">
            <h2>Live runtime</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <div class="key-value"><span>Активный preset</span><strong id="runtime-preset">-</strong></div>
            <div class="key-value"><span>Автовоспроизведение</span><strong id="runtime-autoplay">-</strong></div>
            <div class="key-value"><span>Плейлист</span><strong id="runtime-playlist">-</strong></div>
            <div class="key-value"><span>Эффект fallback</span><strong id="runtime-effect">-</strong></div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Диагностика</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <p id="diagnostics-summary">Здесь появятся ошибки DSL, подсказки и статус компиляции.</p>
            <div class="status-note" id="diagnostics-status">Ожидаем данные от лампы.</div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Пресеты</h2>
          </div>
          <div class="panel__body">
            <ul class="item-list">${renderStarterSnippetList()}</ul>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Плейлист</h2>
          </div>
          <div class="panel__body panel__body--stack">
            <p>Автопереключение уже работает на устройстве. Здесь появится редактирование очереди и durations.</p>
            <div class="status-note">Manual Run по-прежнему останавливает autoplay сразу.</div>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Справка DSL</h2>
          </div>
          <div class="panel__body panel__body--stack">${renderHelpSections()}</div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Настройки лампы</h2>
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

function findSnippetById(snippetId: string): StarterSnippet | undefined {
  return starterSnippets.find((snippet) => snippet.id === snippetId);
}

function applySnippet(snippet: StarterSnippet): void {
  setText("editor-code", snippet.source);
  setText("editor-hint", `${snippet.name}: ${snippet.description}`);
  setText("diagnostics-summary", `Шаблон загружен: ${snippet.name}. Проверь expressions и нажми «Запустить».`);
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

function formatNumber(value: number | null, suffix: string): string {
  if (value === null || Number.isNaN(value)) {
    return "-";
  }

  return `${value}${suffix}`;
}

function renderStatus(status: StatusPayload): void {
  setText("build-pill", `${status.version} · ${status.channel}`);
  setText("runtime-preset", status.activePresetName || status.activePresetId || "Временный запуск / нет preset");
  setText("runtime-autoplay", status.autoplayEnabled ? "Включено" : "Выключено");
  setText("runtime-playlist", status.activePlaylistId || "Нет активного playlist");
  setText("runtime-effect", status.activeEffect || "- ");
  setText(
    "diagnostics-summary",
    status.liveErrorSummary || "Ошибок live runtime нет. Следующий шаг: привязать validate/run/save к редактору.",
  );
  setText("diagnostics-status", status.liveErrorSummary ? "Требуется исправление DSL" : "Runtime готов к запуску preset и autoplay");
  setText("lamp-network", status.networkStatus || status.networkMode || "-");
  setText("lamp-clock", status.currentTime || status.clockStatus || "-");
  setText("lamp-sensor", status.sensorStatus || "-");
  setText("lamp-temp", formatNumber(status.temperatureC, " °C"));
  setText("lamp-humidity", formatNumber(status.humidityPercent, " %"));
}

async function refreshStatus(): Promise<void> {
  try {
    const response = await fetch("/api/status", { headers: { Accept: "application/json" } });
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const status = (await response.json()) as StatusPayload;
    renderStatus(status);
  } catch (error) {
    const message = error instanceof Error ? error.message : "unknown error";
    setText("build-pill", "Статус недоступен");
    setText("diagnostics-summary", "Не удалось получить /api/status");
    setText("diagnostics-status", message);
  }
}

void refreshStatus();
applySnippet(starterSnippets[0]);
bindSnippetButtons();
window.setInterval(() => {
  void refreshStatus();
}, 5000);