/* eslint-disable no-console */

// Configurable base URL for the device REST API
const BASE_URL = "http://192.168.1.8";

// LocalStorage key
const STORAGE_KEY = "esp8266AuthKey";

// Cached DOM references
const els = {
  authStatus: document.getElementById("auth-status"),
  baseUrl: document.getElementById("base-url"),
  setupAuth: document.getElementById("setup-auth"),
  setupSerial: document.getElementById("setup-serial"),
  applySetup: document.getElementById("apply-setup"),
  setupAuthKey: document.getElementById("setup-auth-key"),
  setupAuthKeyValue: document.getElementById("setup-auth-key-value"),
  authInput: document.getElementById("auth-key-input"),
  saveAuthKey: document.getElementById("save-auth-key"),
  clearAuthKey: document.getElementById("clear-auth-key"),
  refreshState: document.getElementById("refresh-state"),
  refreshAll: document.getElementById("refresh-all"),
  stateTable: document.getElementById("state-table"),
  stateSummary: document.getElementById("state-summary"),
  gpioTable: document.getElementById("gpio-table"),
  cronTable: document.getElementById("cron-table"),
  refreshGpio: document.getElementById("refresh-gpio"),
  refreshCron: document.getElementById("refresh-cron"),
  cronExpression: document.getElementById("cron-expression"),
  cronAction: document.getElementById("cron-action"),
  cronPin: document.getElementById("cron-pin"),
  cronValue: document.getElementById("cron-value"),
  addCron: document.getElementById("add-cron"),
  logOutput: document.getElementById("log-output"),
  curlNonce: document.getElementById("curl-nonce"),
  curlState: document.getElementById("curl-state"),
  curlPin: document.getElementById("curl-pin"),
  curlCron: document.getElementById("curl-cron"),
};

let lastState = null;

function addLog(message, type = "info") {
  const entry = document.createElement("div");
  entry.className = `log-entry ${type === "error" ? "log-error" : ""}`;
  const time = new Date().toLocaleTimeString();
  entry.innerHTML = `<span class="log-time">${time}</span>${message}`;
  els.logOutput.prepend(entry);
}

function getAuthKey() {
  return (els.authInput.value || localStorage.getItem(STORAGE_KEY) || "").trim().toLowerCase();
}

function hasAuthKey() {
  return /^[0-9a-f]{64}$/.test(getAuthKey());
}

function updateAuthBadge() {
  if (hasAuthKey()) {
    els.authStatus.textContent = "Auth key: stored";
    els.authStatus.classList.remove("badge-warn");
    els.authStatus.classList.add("badge-muted");
  } else {
    els.authStatus.textContent = "Auth key: missing";
    els.authStatus.classList.add("badge-warn");
    els.authStatus.classList.remove("badge-muted");
  }
}

function showSetupKey(key) {
  if (key) {
    els.setupAuthKey.classList.remove("hidden");
    els.setupAuthKeyValue.textContent = key;
  } else {
    els.setupAuthKey.classList.add("hidden");
    els.setupAuthKeyValue.textContent = "";
  }
}

function formatUptime(seconds) {
  const s = Number(seconds) || 0;
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  const parts = [];
  if (d) parts.push(`${d}d`);
  if (h) parts.push(`${h}h`);
  if (m || parts.length === 0) parts.push(`${m}m`);
  return parts.join(" ");
}

async function requestNonce() {
  const res = await fetch(`${BASE_URL}/api/auth/challenge`);
  if (!res.ok) {
    throw new Error(`Nonce request failed (${res.status})`);
  }
  const data = await res.json();
  return String(data.nonce);
}

async function apiFetch(path, opts = {}) {
  const { method = "GET", body = null, requireAuth = false, preferAuth = false } = opts;
  const headers = opts.headers ? { ...opts.headers } : {};
  let payload = "";

  if (body !== null && body !== undefined) {
    payload = JSON.stringify(body);
    headers["Content-Type"] = "application/json";
  }

  const authKey = getAuthKey();
  const shouldSign = requireAuth || (preferAuth && authKey);

  if (shouldSign && !authKey) {
    throw new Error("Auth key required for this action");
  }

  if (shouldSign) {
    const nonce = await requestNonce();
    const signature = await hmacSha256Hex(authKey, nonce + path + payload);
    headers["X-Nonce"] = nonce;
    headers["X-Auth"] = signature;
  }

  const res = await fetch(`${BASE_URL}${path}`, {
    method,
    headers,
    body: payload === "" ? undefined : payload,
  });

  const contentType = res.headers.get("content-type") || "";
  const isJson = contentType.includes("application/json");
  const data = isJson ? await res.json() : await res.text();

  if (!res.ok) {
    const detail = typeof data === "string" ? data : JSON.stringify(data);
    throw new Error(`HTTP ${res.status}: ${detail}`);
  }
  return data;
}

function renderState(state) {
  els.stateTable.innerHTML = "";
  const { device = {} } = state || {};
  const entries = [
    ["IP", device.ip || "—"],
    ["Chip ID", device.chip ?? "—"],
    ["RSSI", device.rssi != null ? `${device.rssi} dBm` : "—"],
    ["Uptime", formatUptime(device.uptime)],
    ["Auth enabled", device.auth ? "Yes" : "No"],
    ["Serial debug", device.serialDebug ? "On" : "Off"],
  ];

  entries.forEach(([label, value]) => {
    const row = document.createElement("tr");
    row.innerHTML = `<th>${label}</th><td>${value}</td>`;
    els.stateTable.appendChild(row);
  });

  if (typeof device.auth === "boolean") {
    els.setupAuth.checked = !!device.auth;
  }
  if (typeof device.serialDebug === "boolean") {
    els.setupSerial.checked = !!device.serialDebug;
  }

  const pinCount = Object.keys(state?.pins || {}).length;
  const cronCount = Object.keys(state?.cronJobs || {}).length;
  const summary = [
    `Pins: ${pinCount || 0}`,
    `Cron: ${cronCount || 0}`,
    device.auth ? "Auth attiva" : "Auth disabilitata",
  ];
  els.stateSummary.innerHTML = summary.map((text) => `<span class="summary-item">${text}</span>`).join("");
}

function renderPins(state) {
  els.gpioTable.innerHTML = "";
  const pins = state?.pins || {};
  const hasKey = hasAuthKey();

  Object.entries(pins).forEach(([id, pin]) => {
    const row = document.createElement("tr");
    const modes = ["Disabled", ...(pin.capabilities || [])];
    const currentMode = pin.mode || "Disabled";

    const modeSelect = document.createElement("select");
    modes.forEach((mode) => {
      const opt = document.createElement("option");
      opt.value = mode;
      opt.textContent = mode;
      if (mode === currentMode) opt.selected = true;
      modeSelect.appendChild(opt);
    });
    modeSelect.disabled = !hasKey;
    modeSelect.addEventListener("change", async () => {
      const newMode = modeSelect.value;
      if (newMode.toLowerCase() === "output") {
        await updatePin(id, newMode, pin.state ?? 0);
      } else if (newMode.toLowerCase() === "pwm") {
        await updatePin(id, newMode, Number(pin.state ?? 0));
      } else {
        await updatePin(id, newMode, undefined);
      }
    });

    const stateCell = document.createElement("div");
    const modeLower = currentMode.toLowerCase();
    if (modeLower === "output") {
      const btn = document.createElement("button");
      btn.className = "primary";
      btn.textContent = pin.state ? "Turn OFF" : "Turn ON";
      btn.disabled = !hasKey;
      btn.addEventListener("click", async () => {
        const next = pin.state ? 0 : 1;
        await updatePin(id, "Output", next);
      });
      stateCell.appendChild(btn);
    } else if (modeLower === "pwm") {
      const range = document.createElement("input");
      range.type = "range";
      range.min = "0";
      range.max = "1023";
      range.value = pin.state ?? 0;
      range.disabled = !hasKey;
      const valueLabel = document.createElement("span");
      valueLabel.textContent = ` ${range.value}`;
      range.addEventListener("input", () => {
        valueLabel.textContent = ` ${range.value}`;
      });
      range.addEventListener("change", async () => {
        await updatePin(id, "Pwm", Number(range.value));
      });
      stateCell.append(range, valueLabel);
    } else {
      stateCell.textContent = pin.state != null ? pin.state : "—";
    }

    const safetyClass =
      pin.safety === "Safe"
        ? "pill-success"
        : pin.safety === "Warn"
          ? "pill-warn"
          : "pill-danger";
    const safetyPill = `<span class="pill ${safetyClass}">${pin.safety || "Unknown"}</span>`;

    const actions = document.createElement("div");
    actions.className = "controls";
    const refreshBtn = document.createElement("button");
    refreshBtn.className = "ghost";
    refreshBtn.textContent = "Reload";
    refreshBtn.disabled = !hasKey;
    refreshBtn.addEventListener("click", async () => {
      await fetchState();
    });
    actions.appendChild(refreshBtn);

    row.innerHTML = `
      <td>
        <div class="table-title">${id}</div>
        <div class="small-text">${(pin.capabilities || []).join(" · ")}</div>
      </td>
      <td></td>
      <td></td>
      <td>${safetyPill}</td>
      <td></td>
    `;

    row.children[1].appendChild(modeSelect);
    row.children[2].appendChild(stateCell);
    row.children[4].appendChild(actions);

    els.gpioTable.appendChild(row);
  });

  if (Object.keys(pins).length === 0) {
    const empty = document.createElement("tr");
    empty.innerHTML = `<td colspan="5" class="small-text">No pins reported.</td>`;
    els.gpioTable.appendChild(empty);
  }
}

function renderCron(state) {
  els.cronTable.innerHTML = "";
  const cron = state?.cronJobs || {};
  const hasKey = hasAuthKey();

  Object.entries(cron).forEach(([id, job]) => {
    const row = document.createElement("tr");
    const pillClass = job.state === "Active" ? "pill-success" : "pill-warn";
    const deleteBtn = document.createElement("button");
    deleteBtn.className = "danger";
    deleteBtn.textContent = "Delete";
    deleteBtn.disabled = !hasKey;
    deleteBtn.addEventListener("click", async () => {
      await deleteCron(id);
    });

    const actionCell = document.createElement("div");
    actionCell.className = "controls";
    actionCell.appendChild(deleteBtn);

    row.innerHTML = `
      <td>${id}</td>
      <td>${job.cron || "?"}</td>
      <td>${job.action || "?"}</td>
      <td>${job.pin ?? "—"}</td>
      <td>${job.value ?? "—"}</td>
      <td><span class="pill ${pillClass}">${job.state || "Unknown"}</span></td>
      <td></td>
    `;

    row.children[6].appendChild(actionCell);
    els.cronTable.appendChild(row);
  });

  if (Object.keys(cron).length === 0) {
    const empty = document.createElement("tr");
    empty.innerHTML = `<td colspan="7" class="small-text">No cron jobs configured.</td>`;
    els.cronTable.appendChild(empty);
  }
}

async function fetchState() {
  try {
    const data = await apiFetch("/api/state", { preferAuth: true });
    lastState = data;
    renderState(data);
    renderPins(data);
    renderCron(data);
    addLog("State updated");
  } catch (err) {
    console.error(err);
    addLog(err.message, "error");
  }
}

async function applySetup() {
  try {
    const payload = {
      auth: !!els.setupAuth.checked,
      serialDebug: !!els.setupSerial.checked,
    };
    const data = await apiFetch("/api/setup", { method: "POST", body: payload, requireAuth: payload.auth });
    showSetupKey(data.authKey);
    addLog("Setup applied");
    await fetchState();
  } catch (err) {
    console.error(err);
    showSetupKey(null);
    addLog(`Setup error: ${err.message}`, "error");
  }
}

async function updatePin(id, mode, state) {
  try {
    const payload = { id, mode };
    if (state !== undefined) payload.state = state;
    await apiFetch("/api/pin/set", { method: "PATCH", body: payload, requireAuth: true });
    addLog(`Updated ${id} → ${mode}${state !== undefined ? ` (${state})` : ""}`);
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Pin update failed: ${err.message}`, "error");
  }
}

async function addCronJob() {
  try {
    const payload = {
      cron: els.cronExpression.value.trim(),
      action: els.cronAction.value,
      pin: els.cronPin.value.trim(),
      value: els.cronValue.value === "" ? undefined : Number(els.cronValue.value),
    };
    await apiFetch("/api/cron/set", { method: "PATCH", body: payload, requireAuth: true });
    addLog("Cron job added");
    els.cronExpression.value = "";
    els.cronPin.value = "";
    els.cronValue.value = "";
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Cron add failed: ${err.message}`, "error");
  }
}

async function deleteCron(id) {
  try {
    await apiFetch(`/api/cron?id=${encodeURIComponent(id)}`, { method: "DELETE", requireAuth: true });
    addLog(`Cron #${id} deleted`);
    await fetchState();
  } catch (err) {
    console.error(err);
    addLog(`Cron delete failed: ${err.message}`, "error");
  }
}

function initAuthControls() {
  const stored = localStorage.getItem(STORAGE_KEY) || "";
  if (stored) {
    els.authInput.value = stored;
  }
  updateAuthBadge();

  els.saveAuthKey.addEventListener("click", () => {
    const key = els.authInput.value.trim().toLowerCase();
    if (!/^[0-9a-f]{64}$/.test(key)) {
      addLog("Invalid key. Must be 64 hex chars.", "error");
      return;
    }
    localStorage.setItem(STORAGE_KEY, key);
    updateAuthBadge();
    addLog("Auth key saved");
  });

  els.clearAuthKey.addEventListener("click", () => {
    localStorage.removeItem(STORAGE_KEY);
    els.authInput.value = "";
    updateAuthBadge();
    addLog("Auth key cleared");
  });
}

function renderCurlExamples() {
  const nonceCmd = `curl -s ${BASE_URL}/api/auth/challenge`;
  const stateCmd = [
    `nonce=$( ${nonceCmd} | jq -r .nonce )`,
    `sig=$(python - <<'PY'`,
    `import hmac, hashlib, os`,
    `key=os.environ.get("AUTH_KEY","").strip()`,
    `nonce=os.environ.get("nonce","") or "${'$'}{nonce}"`,
    `payload=""`,
    `msg=(nonce+"/api/state"+payload).encode()`,
    `print(hmac.new(bytes.fromhex(key), msg, hashlib.sha256).hexdigest())`,
    `PY`,
    `)`,
    `curl -H "X-Nonce: ${'$'}nonce" -H "X-Auth: ${'$'}sig" ${BASE_URL}/api/state`,
  ].join("\n");

  const pinCmd = [
    `nonce=$( ${nonceCmd} | jq -r .nonce )`,
    `body='{\"id\":\"GPIO4\",\"mode\":\"Output\",\"state\":1}'`,
    `sig=$(python - <<'PY'`,
    `import hmac, hashlib, os`,
    `key=os.environ.get("AUTH_KEY","").strip()`,
    `nonce=os.environ.get("nonce","") or "${'$'}{nonce}"`,
    `body='${'$'}{body}'`,
    `msg=(nonce+"/api/pin/set"+body).encode()`,
    `print(hmac.new(bytes.fromhex(key), msg, hashlib.sha256).hexdigest())`,
    `PY`,
    `)`,
    `curl -X PATCH -H "Content-Type: application/json" \\`,
    `  -H "X-Nonce: ${'$'}nonce" -H "X-Auth: ${'$'}sig" \\`,
    `  -d '${'$'}body' ${BASE_URL}/api/pin/set`,
  ].join("\n");

  const cronCmd = [
    `nonce=$( ${nonceCmd} | jq -r .nonce )`,
    `body='{\"cron\":\"*/5 * * * *\",\"action\":\"toggle\",\"pin\":\"GPIO4\"}'`,
    `sig=$(python - <<'PY'`,
    `import hmac, hashlib, os`,
    `key=os.environ.get("AUTH_KEY","").strip()`,
    `nonce=os.environ.get("nonce","") or "${'$'}{nonce}"`,
    `body='${'$'}{body}'`,
    `msg=(nonce+"/api/cron/set"+body).encode()`,
    `print(hmac.new(bytes.fromhex(key), msg, hashlib.sha256).hexdigest())`,
    `PY`,
    `)`,
    `curl -X PATCH -H "Content-Type: application/json" \\`,
    `  -H "X-Nonce: ${'$'}nonce" -H "X-Auth: ${'$'}sig" \\`,
    `  -d '${'$'}body' ${BASE_URL}/api/cron/set`,
  ].join("\n");

  els.curlNonce.textContent = nonceCmd;
  els.curlState.textContent = stateCmd;
  els.curlPin.textContent = pinCmd;
  els.curlCron.textContent = cronCmd;
}

function initEventListeners() {
  els.applySetup.addEventListener("click", applySetup);
  els.refreshState.addEventListener("click", fetchState);
  els.refreshAll.addEventListener("click", fetchState);
  els.refreshGpio.addEventListener("click", fetchState);
  els.refreshCron.addEventListener("click", fetchState);
  els.addCron.addEventListener("click", addCronJob);
}

function init() {
  els.baseUrl.textContent = BASE_URL;
  initAuthControls();
  initEventListeners();
  renderCurlExamples();
  fetchState();
}

document.addEventListener("DOMContentLoaded", init);
