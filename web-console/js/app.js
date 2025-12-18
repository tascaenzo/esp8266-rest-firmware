import Router from "./core/router.js";
import * as Settings from "./views/settings.js";
import { getAuthExpected } from "./core/config.js";
import { setAuthExpectation } from "./core/state.js";

document.addEventListener("DOMContentLoaded", () => {
  setAuthExpectation(getAuthExpected());
  bindNavigation();
  bindSettingsPanel();
  Router.init();

  console.log("[app] loaded");
});

function bindNavigation() {
  document.querySelectorAll(".sidebar li").forEach((item) => {
    item.addEventListener("click", () => {
      location.hash = item.dataset.view;
    });
  });
}

function bindSettingsPanel() {
  const overlay = document.getElementById("settings-overlay");
  const openBtn = document.getElementById("open-settings");
  const closeBtn = document.getElementById("close-settings");

  if (!overlay || !openBtn || !closeBtn) {
    console.error("[settings] missing DOM elements");
    return;
  }

  openBtn.addEventListener("click", () => {
    overlay.classList.remove("hidden");

    Settings.init();
  });

  closeBtn.addEventListener("click", () => {
    overlay.classList.add("hidden");
  });
}
