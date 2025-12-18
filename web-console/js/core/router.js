/**
 * @brief Simple hash-based router for the Firmware Console.
 *
 * This router is responsible for:
 *  - mapping URL hash fragments to HTML templates
 *  - loading documentation views dynamically
 *  - initializing and tearing down optional view-specific logic
 *
 * It is intentionally minimal and framework-free, to mirror
 * the embedded firmware philosophy.
 */
const Router = (() => {
  /**
   * @brief Reference to the currently active view controller.
   *
   * A view controller is an optional JS module that can expose:
   *  - init()     → called after the template is loaded
   *  - destroy()  → called before switching to another view
   */
  let currentView = null;

  /**
   * @brief Load an HTML template for a given view.
   *
   * Templates are fetched via HTTP from the /templates directory.
   * The view name must match the template filename.
   *
   * @param {string} view - View identifier (e.g. "auth", "gpio")
   * @returns {Promise<string>} HTML content of the template
   * @throws Error if the template cannot be loaded
   */
  async function loadTemplate(view) {
    const res = await fetch(`templates/${view}.html`);
    if (!res.ok) {
      throw new Error(`Template not found: ${view}`);
    }
    return res.text();
  }

  /**
   * @brief Load and activate a view.
   *
   * This function:
   *  1. Tears down the previous view (if any)
   *  2. Loads the HTML template
   *  3. Injects it into the main container
   *  4. Dynamically loads the optional JS controller
   *  5. Marks the active navigation entry
   *
   * @param {string} view - View identifier
   */
  async function loadView(view) {
    const container = document.getElementById("view-container");

    // Tear down previously active view, if it defines a destroy hook
    if (currentView?.destroy) {
      currentView.destroy();
    }

    // Load and inject HTML documentation template
    const html = await loadTemplate(view);
    container.innerHTML = html;

    // Attempt to load the view-specific JS controller (optional)
    try {
      const module = await import(`../views/${view}.js`);
      currentView = module.default || module;

      // Initialize the view if an init hook is provided
      if (currentView.init) {
        currentView.init();
      }
    } catch (err) {
      // No JS controller found → documentation-only view
      currentView = null;
    }

    // Update sidebar navigation state
    setActiveNav(view);
  }

  /**
   * @brief Update sidebar navigation highlighting.
   *
   * Marks the active view based on the current route.
   *
   * @param {string} view - Active view identifier
   */
  function setActiveNav(view) {
    document.querySelectorAll(".sidebar li").forEach((li) => {
      li.classList.toggle("active", li.dataset.view === view);
    });
  }

  /**
   * @brief Extract the view name from the URL hash.
   *
   * If no hash is present, defaults to "overview".
   *
   * @returns {string} View identifier
   */
  function getViewFromHash() {
    return location.hash.replace("#", "") || "overview";
  }

  /**
   * @brief Handle route changes triggered by hash updates.
   *
   * This function resolves the target view and attempts to load it.
   * On failure, it falls back to the overview page.
   */
  function onRouteChange() {
    const view = getViewFromHash();
    loadView(view).catch((err) => {
      console.error(err);
      loadView("overview");
    });
  }

  /**
   * @brief Initialize the router.
   *
   * Registers the hashchange listener and loads the initial view.
   * This should be called once during application bootstrap.
   */
  function init() {
    window.addEventListener("hashchange", onRouteChange);
    onRouteChange();
  }

  // Public API
  return { init };
})();

export default Router;
