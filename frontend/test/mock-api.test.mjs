import assert from "assert";
import http from "http";
import { handleApi } from "../mockApi.mjs";

async function startMockServer() {
  const server = http.createServer((req, res) => {
    void handleApi(req, res).then((handled) => {
      if (!handled) {
        res.statusCode = 404;
        res.setHeader("Content-Type", "application/json; charset=utf-8");
        res.end(JSON.stringify({ error: "not found" }));
      }
    });
  });

  await new Promise((resolve) => {
    server.listen(0, "127.0.0.1", resolve);
  });

  const address = server.address();
  if (!address || typeof address === "string") {
    throw new Error("Failed to determine mock server address");
  }

  return {
    server,
    baseUrl: `http://127.0.0.1:${address.port}`,
  };
}

function requestJson(baseUrl, pathname, method = "GET", body = null, headers = {}) {
  return new Promise((resolve, reject) => {
    const request = http.request(
      `${baseUrl}${pathname}`,
      {
        method,
        headers,
      },
      (response) => {
        const chunks = [];
        response.on("data", (chunk) => chunks.push(chunk));
        response.on("end", () => {
          const text = Buffer.concat(chunks).toString("utf8");
          let payload = null;

          try {
            payload = text ? JSON.parse(text) : null;
          } catch (error) {
            reject(error);
            return;
          }

          resolve({ statusCode: response.statusCode || 0, payload });
        });
      },
    );

    request.on("error", reject);

    if (body) {
      request.write(body);
    }

    request.end();
  });
}

async function main() {
  const { server, baseUrl } = await startMockServer();

  const cleanup = () => new Promise((resolve) => server.close(resolve));

  try {
    const currentResponse = await requestJson(baseUrl, "/api/update/current", "GET", null, {
      "X-Dev-Scenario": "happy-path",
    });

    assert.equal(currentResponse.statusCode, 200);
    const currentPayload = currentResponse.payload;
    assert.equal(typeof currentPayload.channel, "string");
    assert.equal(typeof currentPayload.updateChannel, "string");
    assert.equal(typeof currentPayload.updateState, "string");
    assert.equal(typeof currentPayload.availableVersion, "string");

    const settingsResponse = await requestJson(baseUrl, "/api/update/settings", "GET", null, {
      "X-Dev-Scenario": "happy-path",
    });

    assert.equal(settingsResponse.statusCode, 200);
    const settingsPayload = settingsResponse.payload;
    assert.equal(settingsPayload.channel, "dev");

    const switchResponse = await requestJson(
      baseUrl,
      "/api/update/settings",
      "POST",
      "channel=stable",
      {
        "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
        "X-Dev-Scenario": "happy-path",
      },
    );
    assert.equal(switchResponse.statusCode, 200);
    assert.equal(switchResponse.payload.channel, "stable");

    const stableCheckResponse = await requestJson(
      baseUrl,
      "/api/update/check",
      "POST",
      "channel=stable",
      {
        "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
        "X-Dev-Scenario": "happy-path",
      },
    );
    assert.equal(stableCheckResponse.statusCode, 200);
    assert.equal(stableCheckResponse.payload.hasUpdate, false);

    const devCheckResponse = await requestJson(
      baseUrl,
      "/api/update/check",
      "POST",
      "channel=dev",
      {
        "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
        "X-Dev-Scenario": "happy-path",
      },
    );
    assert.equal(devCheckResponse.statusCode, 200);
    assert.equal(devCheckResponse.payload.hasUpdate, true);
    assert.equal(typeof devCheckResponse.payload.version, "string");

    const postCheckCurrentResponse = await requestJson(baseUrl, "/api/update/current", "GET", null, {
      "X-Dev-Scenario": "happy-path",
    });
    assert.equal(postCheckCurrentResponse.statusCode, 200);
    assert.equal(postCheckCurrentResponse.payload.updateState, "available");
    assert.equal(postCheckCurrentResponse.payload.updateChannel, "dev");
    assert.equal(postCheckCurrentResponse.payload.availableVersion, devCheckResponse.payload.version);

    const installResponse = await requestJson(baseUrl, "/api/update/install", "POST", null, {
      "X-Dev-Scenario": "happy-path",
    });
    assert.equal(installResponse.statusCode, 200);
    assert.equal(installResponse.payload.success, true);
    assert.equal(installResponse.payload.rebooting, true);

    const postInstallCurrentResponse = await requestJson(baseUrl, "/api/update/current", "GET", null, {
      "X-Dev-Scenario": "happy-path",
    });
    assert.equal(postInstallCurrentResponse.statusCode, 200);
    assert.equal(postInstallCurrentResponse.payload.updateState, "completed");

    const resetResponse = await requestJson(baseUrl, "/__dev/reset", "POST", null, {
      "X-Dev-Scenario": "happy-path",
    });
    assert.equal(resetResponse.statusCode, 200);

    const postResetCurrentResponse = await requestJson(baseUrl, "/api/update/current", "GET", null, {
      "X-Dev-Scenario": "happy-path",
    });
    assert.equal(postResetCurrentResponse.statusCode, 200);
    assert.equal(postResetCurrentResponse.payload.updateState, "idle");
    assert.equal(postResetCurrentResponse.payload.updateChannel, "dev");
  } finally {
    await cleanup();
  }
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});