import { useState } from 'preact/hooks';
import { WebSocketProvider, useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Header with connection status and mobile-responsive navigation
 */
export function Header({ setView, views, defaultView, currentView, darkMode, setDarkMode }) {
  const { connected } = useWebSocket();
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  const handleNavClick = (viewId) => {
    setView(viewId);
    setMobileMenuOpen(false); // Close mobile menu after selection
  };

  return (
    <header class="bg-white dark:bg-gray-800 shadow-md">
      <div class="max-w-7xl mx-auto px-4 py-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between">
          <h1
            class="text-2xl font-bold text-gray-900 dark:text-white cursor-pointer hover:text-gray-700 dark:hover:text-gray-300 transition-colors"
            onClick={() => setView(defaultView)}
          >
            Therem[AI]n Control Panel
          </h1>

          <div class="flex items-center gap-4">
            {/* Desktop Navigation - Hidden on mobile */}
            <nav class="hidden md:flex gap-2">
              {views.map(item => {
                const isActive = item.id === currentView;
                return (
                  <button
                    key={item.id}
                    onClick={() => setView(item.id)}
                    class={`px-4 py-2 text-sm font-medium rounded-md transition-colors ${
                      isActive
                        ? 'bg-blue-100 dark:bg-blue-900 text-blue-700 dark:text-blue-200 hover:bg-blue-200 dark:hover:bg-blue-800'
                        : 'text-gray-700 dark:text-gray-300 hover:text-gray-900 dark:hover:text-white hover:bg-gray-100 dark:hover:bg-gray-700'
                    }`}
                  >
                    {item.label}
                  </button>
                );
              })}
            </nav>

            {/* Dark Mode Toggle */}
            <button
              onClick={() => setDarkMode(!darkMode)}
              class="p-2 text-gray-700 dark:text-gray-300 hover:text-gray-900 dark:hover:text-white hover:bg-gray-100 dark:hover:bg-gray-700 rounded-md transition-colors"
              aria-label="Toggle dark mode"
            >
              <svg
                class="w-5 h-5"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
              >
                {darkMode ? (
                  // Sun icon (light mode)
                  <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M12 3v1m0 16v1m9-9h-1M4 12H3m15.364 6.364l-.707-.707M6.343 6.343l-.707-.707m12.728 0l-.707.707M6.343 17.657l-.707.707M16 12a4 4 0 11-8 0 4 4 0 018 0z"
                  />
                ) : (
                  // Moon icon (dark mode)
                  <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M20.354 15.354A9 9 0 018.646 3.646 9.003 9.003 0 0012 21a9.003 9.003 0 008.354-5.646z"
                  />
                )}
              </svg>
            </button>

            {/* Mobile Hamburger Button - Hidden on desktop */}
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              class="md:hidden p-2 text-gray-700 dark:text-gray-300 hover:text-gray-900 dark:hover:text-white hover:bg-gray-100 dark:hover:bg-gray-700 rounded-md transition-colors"
              aria-label="Toggle menu"
            >
              <svg
                class="w-6 h-6"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
              >
                {mobileMenuOpen ? (
                  // X icon when menu is open
                  <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M6 18L18 6M6 6l12 12"
                  />
                ) : (
                  // Hamburger icon when menu is closed
                  <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth={2}
                    d="M4 6h16M4 12h16M4 18h16"
                  />
                )}
              </svg>
            </button>

            {/* Connection Status */}
            <div class="flex items-center gap-2">
              <div class={`w-3 h-3 rounded-full ${connected ? 'bg-green-500' : 'bg-red-500'}`} />
              <span class="text-sm text-gray-600 dark:text-gray-400 hidden sm:inline">
                {connected ? 'Connected' : 'Disconnected'}
              </span>
            </div>
          </div>
        </div>

        {/* Mobile Menu Dropdown - Shown when hamburger is clicked */}
        {mobileMenuOpen && (
          <nav class="md:hidden mt-4 pb-2 border-t border-gray-200 dark:border-gray-700 pt-4">
            <div class="flex flex-col gap-2">
              {views.map(item => {
                const isActive = item.id === currentView;
                return (
                  <button
                    key={item.id}
                    onClick={() => handleNavClick(item.id)}
                    class={`w-full text-left px-4 py-3 text-sm font-medium rounded-md transition-colors ${
                      isActive
                        ? 'bg-blue-100 dark:bg-blue-900 text-blue-700 dark:text-blue-200 hover:bg-blue-200 dark:hover:bg-blue-800'
                        : 'text-gray-700 dark:text-gray-300 hover:text-gray-900 dark:hover:text-white hover:bg-gray-100 dark:hover:bg-gray-700'
                    }`}
                  >
                    {item.label}
                  </button>
                );
              })}
            </div>
          </nav>
        )}
      </div>
    </header>
  );
}
