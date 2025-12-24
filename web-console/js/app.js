import Router from "./core/router.js";
import { enableDevReload } from "./core/dev-mode.js";
import openSettingsModal from "./components/settings-modal.js";

document.addEventListener("DOMContentLoaded", () => {
  bindComponentsUi();
  enableDevReload();
  Router.init();

  console.log("[app] loaded");
});

function bindComponentsUi() {
  /* Navigation */
  document.querySelectorAll("[data-view]").forEach((btn) => {
    btn.addEventListener("click", () => {
      const view = btn.dataset.view;
      if (view) {
        location.hash = view;
      }
    });
  });

  /* Settings */
  document
    .getElementById("open-settings")
    .addEventListener("click", openSettingsModal);
}
