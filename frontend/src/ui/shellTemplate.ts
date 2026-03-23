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
    <main class="shell shell--workspace">
      <header class="shell__header shell__header--workspace">
        <div>
          <p class="eyebrow">MyLamp</p>
          <h1>Моя Лампа</h1>
          <p class="shell__subtitle">Учимся лайвкодить</p>
        </div>
        <div class="shell__header-actions">
          ${options.isDevServer ? `
          <section class="dev-panel">
            <label class="dev-panel__label" for="dev-scenario-select">Сценарий для проверки</label>
            <select class="dev-panel__select" id="dev-scenario-select">${options.scenarioOptions}</select>
            <button class="dev-panel__button" id="dev-reset-button" type="button">Сбросить пример</button>
            <p class="dev-panel__description" id="dev-scenario-description"></p>
          </section>` : ""}
          <div class="header-device-actions">
            <button class="icon-button" id="network-settings-button" type="button" aria-label="Настройка сети">WiFi</button>
            <button class="icon-button" id="firmware-settings-button" type="button" aria-label="Информация о прошивке">FW</button>
          </div>
        </div>
      </header>

      <section class="panel panel--editor panel--workspace">
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
            <div class="editor-toolbar__hint" id="editor-hint">Выбери идею рядом с редактором и подкрути её под себя.</div>
            <div class="editor-toolbar__status" id="editor-status">Кликни в код и печатай. Курсор появится внутри поля.</div>
          </div>
          <section class="workspace-dock">
            <details class="workspace-dropdown" id="dropdown-presets" open>
              <summary>Идеи и пресеты</summary>
              <div class="workspace-dropdown__body">
                <ul class="item-list">${options.starterSnippetList}</ul>
              </div>
            </details>
            <details class="workspace-dropdown" id="dropdown-playlist">
              <summary>Очередь</summary>
              <div class="workspace-dropdown__body">
                <div class="key-value"><span>Сейчас включено</span><strong id="runtime-preset">-</strong></div>
                <div class="key-value"><span>Автосмена</span><strong id="runtime-autoplay">-</strong></div>
                <div class="key-value"><span>Очередь огоньков</span><strong id="runtime-playlist">-</strong></div>
                <div class="key-value"><span>Запасной режим</span><strong id="runtime-effect">-</strong></div>
                <div class="status-note">Очередь и runtime summary теперь живут рядом с редактором, а не в отдельной колонке.</div>
              </div>
            </details>
            <details class="workspace-dropdown" id="dropdown-help">
              <summary>Шпаргалка</summary>
              <div class="workspace-dropdown__body workspace-dropdown__body--stack">${options.helpSections}</div>
            </details>
            <details class="workspace-dropdown" id="dropdown-diagnostics" open>
              <summary>Подсказки</summary>
              <div class="workspace-dropdown__body workspace-dropdown__body--stack">
                <p id="diagnostics-summary">Здесь появятся подсказки, ошибки в коде и результат проверки.</p>
                <div class="status-note" id="diagnostics-status">Ждём новости от лампы.</div>
              </div>
            </details>
          </section>
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
      </section>

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