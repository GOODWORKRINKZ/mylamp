import assert from "assert";

async function main() {
  const { renderShellMarkup } = await import("../.tmp-tests/shellTemplate.js");

  const html = renderShellMarkup({
    isDevServer: true,
    selectedScenario: "happy-path",
    scenarioOptions: '<option value="happy-path" selected>Happy Path</option>',
    starterSnippetList: '<li class="snippet-item">Snippet</li>',
    helpSections: '<section class="help-card">Help</section>',
    networkModalMarkup: '<div id="network-modal" hidden></div>',
    firmwareModalMarkup: '<div id="firmware-modal" hidden></div>',
    timeModalMarkup: '<div id="time-modal" hidden></div>',
  });

  assert.match(html, /id="network-settings-button"/);
  assert.match(html, /aria-label="Настройка сети"/);
  assert.match(html, /title="Настройка сети"/);
  assert.match(html, /id="firmware-settings-button"/);
  assert.match(html, /aria-label="Прошивка и OTA"/);
  assert.match(html, /title="Прошивка и OTA"/);
  assert.match(html, /id="time-settings-button"/);
  assert.match(html, /aria-label="Часовой пояс"/);
  assert.match(html, /title="Часовой пояс"/);
  assert.match(html, /header-device-actions/);
  assert.match(html, /<svg/);
  assert.match(html, /workspace__editor/);
  assert.match(html, /workspace__sidebar/);
  assert.match(html, /sidebar-tabs/);
  assert.match(html, /tab-presets/);
  assert.match(html, /tab-queue/);
  assert.match(html, /tab-help/);
  assert.match(html, /editor-bar__actions/);
  assert.match(html, /error-console__badge/);
  assert.match(html, /error-console/);
  assert.match(html, /editor-gutter/);
  assert.match(html, /editor-highlight/);
  assert.match(html, /id="statusbar-build"/);
  assert.match(html, /id="statusbar-network"/);
  assert.match(html, /id="statusbar-playlist"/);
  assert.match(html, /id="statusbar-clock-action"/);
  assert.match(html, /id="statusbar-temp"/);
  assert.match(html, /id="firmware-modal"/);
  assert.match(html, /id="time-modal"/);
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});