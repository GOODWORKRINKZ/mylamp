import "./styles/app.css";

const app = document.querySelector<HTMLDivElement>("#app");

if (!app) {
  throw new Error("App root not found");
}

app.innerHTML = `
  <main class="shell">
    <header class="shell__header">
      <div>
        <p class="eyebrow">mylamp live coding</p>
        <h1>Editor-first shell</h1>
      </div>
      <div class="status-pill">v1 scaffold</div>
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
          <pre class="code-block">effect "warm_waves"

brightness 0.4
speed 1.0

pixel hsv(nx + t * 0.1, 1.0, 1.0)</pre>
        </div>
      </section>

      <aside class="sidebar">
        <section class="panel">
          <div class="panel__header">
            <h2>Диагностика</h2>
          </div>
          <div class="panel__body">
            <p>Здесь появятся ошибки DSL, подсказки и статус компиляции.</p>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Пресеты</h2>
          </div>
          <div class="panel__body">
            <ul class="item-list">
              <li>Радуга</li>
              <li>Теплые волны</li>
              <li>Летающее сердечко</li>
            </ul>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Плейлист</h2>
          </div>
          <div class="panel__body">
            <p>Автопереключение пресетов по таймеру появится здесь.</p>
          </div>
        </section>

        <section class="panel">
          <div class="panel__header">
            <h2>Настройки лампы</h2>
          </div>
          <div class="panel__body">
            <p>Сеть, время, яркость и служебный статус будут собраны в этом блоке.</p>
          </div>
        </section>
      </aside>
    </section>
  </main>
`;