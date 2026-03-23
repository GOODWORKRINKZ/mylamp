export type ShellTemplateOptions = {
  isDevServer: boolean;
  selectedScenario: string;
  scenarioOptions: string;
  starterSnippetList: string;
  helpSections: string;
  networkModalMarkup: string;
  firmwareModalMarkup: string;
  timeModalMarkup: string;
};

export function renderShellMarkup(options: ShellTemplateOptions): string {
  return `
    <main class="shell">
      <header class="shell__header">
        <div class="shell__brand">
          <h1 class="shell__title">Моя Лампа</h1>
          <span class="shell__subtitle">Лайвкодинг огоньков</span>
        </div>
        <div class="shell__header-actions">
          ${options.isDevServer ? `
          <section class="dev-panel">
            <label class="dev-panel__label" for="dev-scenario-select">Сценарий</label>
            <select class="dev-panel__select" id="dev-scenario-select">${options.scenarioOptions}</select>
            <button class="dev-panel__button" id="dev-reset-button" type="button">Сброс</button>
            <p class="dev-panel__description" id="dev-scenario-description"></p>
          </section>` : ""}
          <div class="header-device-actions">
            <button class="icon-button" id="network-settings-button" type="button" aria-label="Настройка сети">WiFi</button>
            <button class="icon-button" id="firmware-settings-button" type="button" aria-label="Информация о прошивке">FW</button>
          </div>
        </div>
      </header>

      <div class="workspace">
        <section class="workspace__editor">
          <div class="editor-bar">
            <div class="editor-bar__actions">
              <button id="new-effect-button" type="button">Новый</button>
              <button id="validate-button" type="button">Проверить</button>
              <button id="run-button" type="button">Запустить</button>
              <button id="save-button" type="button">Сохранить</button>
            </div>
            <div class="editor-bar__status" id="editor-status">Кликни в код и печатай</div>
          </div>
          <div class="editor-wrap">
            <label class="editor-surface" for="editor-code">
              <span class="editor-surface__badge">DSL</span>
              <textarea
                class="code-editor"
                id="editor-code"
                spellcheck="false"
                autocapitalize="off"
                autocomplete="off"
                autocorrect="off"
                placeholder="effect &quot;my_effect&quot;&#10;&#10;sprite dot {&#10;  bitmap &quot;&quot;&quot;&#10;  #&#10;  &quot;&quot;&quot;&#10;}&#10;&#10;layer paint {&#10;  use dot&#10;  color rgb(255, 120, 80)&#10;  x = 10&#10;  y = 6&#10;  scale = 2&#10;  visible = 1&#10;}"
              ></textarea>
            </label>
          </div>
          <div class="diagnostics-bar">
            <p class="diagnostics-bar__text" id="diagnostics-summary">Здесь появятся подсказки и результат проверки.</p>
            <span class="diagnostics-bar__badge" id="diagnostics-status">Готово</span>
          </div>
        </section>

        <aside class="workspace__sidebar">
          <nav class="sidebar-tabs" role="tablist">
            <button class="sidebar-tab sidebar-tab--active" role="tab" aria-selected="true" data-tab="presets" type="button">Идеи</button>
            <button class="sidebar-tab" role="tab" aria-selected="false" data-tab="queue" type="button">Лампа</button>
            <button class="sidebar-tab" role="tab" aria-selected="false" data-tab="help" type="button">Справка</button>
          </nav>
          <div class="sidebar-panel sidebar-panel--active" id="tab-presets" role="tabpanel">
            <p class="sidebar-panel__hint" id="editor-hint">Выбери идею и подкрути под себя.</p>
            <ul class="item-list">${options.starterSnippetList}</ul>
          </div>
          <div class="sidebar-panel" id="tab-queue" role="tabpanel" hidden>
            <div class="key-value"><span>Сейчас включено</span><strong id="runtime-preset">-</strong></div>
            <div class="key-value"><span>Автосмена</span><strong id="runtime-autoplay">-</strong></div>
            <div class="key-value"><span>Очередь огоньков</span><strong id="runtime-playlist">-</strong></div>
            <div class="key-value"><span>Запасной режим</span><strong id="runtime-effect">-</strong></div>
          </div>
          <div class="sidebar-panel" id="tab-help" role="tabpanel" hidden>
            ${options.helpSections}
          </div>
        </aside>
      </div>

      <footer class="statusbar" aria-label="Состояние лампы">
        <div class="statusbar__item"><span>Сборка</span><strong id="statusbar-build">-</strong></div>
        <div class="statusbar__item"><span>Preset</span><strong id="statusbar-preset">-</strong></div>
        <div class="statusbar__item"><span>Автосмена</span><strong id="statusbar-autoplay">-</strong></div>
        <div class="statusbar__item"><span>Очередь</span><strong id="statusbar-playlist">-</strong></div>
        <div class="statusbar__item"><span>Сеть</span><strong id="statusbar-network">-</strong></div>
        <button class="statusbar__item statusbar__action" id="statusbar-clock-action" type="button">
          <span>Часы</span>
          <strong id="statusbar-clock">-</strong>
        </button>
        <div class="statusbar__item"><span>Сенсор</span><strong id="statusbar-sensor">-</strong></div>
        <div class="statusbar__item"><span>Темп</span><strong id="statusbar-temp">-</strong></div>
        <div class="statusbar__item"><span>Влажн</span><strong id="statusbar-humidity">-</strong></div>
      </footer>

      ${options.networkModalMarkup}
      ${options.firmwareModalMarkup}
      ${options.timeModalMarkup}
    </main>`;
}