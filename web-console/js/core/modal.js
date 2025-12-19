/**
 * @brief Lightweight JSON modal viewer.
 */

let overlay = null;
let titleEl = null;
let bodyEl = null;
let closeBtn = null;

function ensureElements() {
  if (overlay) return;

  overlay = document.getElementById("json-modal");
  titleEl = document.getElementById("json-modal-title");
  bodyEl = document.getElementById("json-modal-body");
  closeBtn = document.getElementById("json-modal-close");

  if (!overlay || !titleEl || !bodyEl || !closeBtn) {
    console.warn("[modal] missing modal elements");
    return;
  }

  closeBtn.addEventListener("click", hideModal);
  overlay.addEventListener("click", (evt) => {
    if (evt.target === overlay) hideModal();
  });

  document.addEventListener("keydown", (evt) => {
    if (evt.key === "Escape" && !overlay.classList.contains("hidden")) {
      hideModal();
    }
  });
}

/**
 * @brief Show JSON content inside the modal overlay.
 * @param {string} title
 * @param {object|string|null} payload
 */
export function showJsonModal(title, payload) {
  ensureElements();
  if (!overlay || !titleEl || !bodyEl) return;

  titleEl.textContent = title || "JSON";

  if (payload === null || payload === undefined) {
    bodyEl.textContent = "No JSON captured yet.";
  } else if (typeof payload === "string") {
    bodyEl.textContent = payload;
  } else {
    bodyEl.textContent = JSON.stringify(payload, null, 2);
  }

  overlay.classList.remove("hidden");
  document.body.classList.add("modal-open");
}

export function hideModal() {
  if (!overlay) return;
  overlay.classList.add("hidden");
  document.body.classList.remove("modal-open");
}

export default { showJsonModal, hideModal };
