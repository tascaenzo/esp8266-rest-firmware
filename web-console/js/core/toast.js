/**
 * @brief Lightweight toast notifications for the console.
 *
 * Designed to surface API failures without cluttering the views with
 * inline banners. Toasts auto-dismiss but can also be dismissed manually.
 */

const STACK_ID = "toast-stack";
const LIFETIME_MS = 4200;

/**
 * @brief Spawn a toast message.
 *
 * @param {string} message - Human-readable text
 * @param {"info"|"success"|"error"} [level="info"]
 */
export function showToast(message, level = "info") {
  const stack = ensureStack();
  const toast = document.createElement("div");
  toast.className = `toast toast-${level}`;
  toast.textContent = message;

  // Allow manual dismissal
  toast.addEventListener("click", () => stack.removeChild(toast));

  stack.appendChild(toast);

  setTimeout(() => {
    if (toast.parentElement === stack) {
      stack.removeChild(toast);
    }
  }, LIFETIME_MS);
}

function ensureStack() {
  let stack = document.getElementById(STACK_ID);
  if (!stack) {
    stack = document.createElement("div");
    stack.id = STACK_ID;
    document.body.appendChild(stack);
  }
  return stack;
}

export default { showToast };
