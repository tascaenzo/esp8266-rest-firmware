const Modal = (() => {
  let modalEl = null;

  function open(content) {
    if (modalEl) return;

    modalEl = document.createElement("div");
    modalEl.className = `
      fixed inset-0 z-50 flex items-center justify-center
      bg-black/30 backdrop-blur-sm
    `;

    modalEl.innerHTML = `
      <div
        class="bg-white rounded-xl shadow-xl
               w-full max-w-md mx-4
               animate-fade-in"
      >
        ${content}
      </div>
    `;

    modalEl.addEventListener("click", (e) => {
      if (e.target === modalEl) close();
    });

    document.addEventListener("keydown", onKeyDown);
    document.body.appendChild(modalEl);
  }

  function close() {
    if (!modalEl) return;

    modalEl.remove();
    modalEl = null;
    document.removeEventListener("keydown", onKeyDown);
  }

  function onKeyDown(e) {
    if (e.key === "Escape") close();
  }

  return { open, close };
})();

export default Modal;
