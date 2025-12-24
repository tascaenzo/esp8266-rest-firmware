import Modal from "./modal.js";
import {
  getBaseUrl,
  setBaseUrl,
  isAuthEnabled,
  setAuthEnabled,
  getApiKey,
  setApiKey,
} from "../core/config.js";

function openSettingsModal() {
  const currentBaseUrl = getBaseUrl();
  const authEnabled = isAuthEnabled();
  const apiKey = getApiKey();

  Modal.open(`
    <div class="p-6">
      <h3 class="text-lg font-semibold mb-1">
        Settings
      </h3>
      <p class="text-sm text-gray-500 mb-6">
        Firmware Console configuration
      </p>

      <!-- Base URL -->
      <label class="block text-sm font-medium mb-1">
        Device Base URL
      </label>

      <input
        id="base-url-input"
        type="text"
        placeholder="http://192.168.1.42"
        value="${currentBaseUrl}"
        class="w-full px-3 py-2 border border-gray-300 rounded
               focus:outline-none focus:ring-2 focus:ring-green-500"
      />

      <p class="text-xs text-gray-500 mt-2 mb-6">
        Leave empty to use the same origin.
      </p>

      <!-- Auth toggle -->
      <div class="flex items-center justify-between mb-4">
        <div>
          <label class="text-sm font-medium">
            Enable authentication
          </label>
          <p class="text-xs text-gray-500">
            Use API key for device requests
          </p>
        </div>

        <input
          id="auth-toggle"
          type="checkbox"
          ${authEnabled ? "checked" : ""}
          class="h-5 w-5 accent-green-600"
        />
      </div>

      <!-- API Key -->
      <label class="block text-sm font-medium mb-1">
        API Key
      </label>

      <input
        id="api-key-input"
        type="password"
        placeholder="••••••••"
        value="${apiKey}"
        ${authEnabled ? "" : "disabled"}
        class="w-full px-3 py-2 border border-gray-300 rounded
               focus:outline-none focus:ring-2 focus:ring-green-500
               disabled:bg-gray-100 disabled:text-gray-400"
      />

      <div class="mt-6 flex justify-end gap-2">
        <button
          id="settings-cancel"
          class="px-4 py-2 text-sm rounded hover:bg-gray-100"
        >
          Cancel
        </button>

        <button
          id="settings-save"
          class="px-4 py-2 text-sm rounded
                 bg-green-600 text-white hover:bg-green-700"
        >
          Save
        </button>
      </div>
    </div>
  `);

  // Elements
  const authToggle = document.getElementById("auth-toggle");
  const apiKeyInput = document.getElementById("api-key-input");

  // Toggle API key input enable/disable
  authToggle.addEventListener("change", () => {
    apiKeyInput.disabled = !authToggle.checked;
    if (!authToggle.checked) {
      apiKeyInput.value = "";
    }
  });

  // Cancel
  document
    .getElementById("settings-cancel")
    .addEventListener("click", Modal.close);

  // Save
  document.getElementById("settings-save").addEventListener("click", () => {
    const baseUrlValue = document.getElementById("base-url-input").value;
    const authEnabledValue = authToggle.checked;
    const apiKeyValue = apiKeyInput.value;

    setBaseUrl(baseUrlValue);
    setAuthEnabled(authEnabledValue);

    if (authEnabledValue) {
      setApiKey(apiKeyValue);
    }

    Modal.close();
  });
}

export default openSettingsModal;
