import { useState } from 'preact/hooks';
import { WebSocketProvider, useWebSocket } from '../hooks/WebSocketProvider';

/**
 * Header with connection status and mobile-responsive navigation
 */
export function Header({ setView, views, defaultView, currentView }) {
  const { connected } = useWebSocket();
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  const handleNavClick = (viewId) => {
    setView(viewId);
    setMobileMenuOpen(false); // Close mobile menu after selection
  };

  return (
    <header class="bg-white shadow-md">
      <div class="max-w-7xl mx-auto px-4 py-4 sm:px-6 lg:px-8">
        <div class="flex items-center justify-between">
          <h1
            class="text-2xl font-bold text-gray-900 cursor-pointer hover:text-gray-700 transition-colors"
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
                        ? 'bg-blue-100 text-blue-700 hover:bg-blue-200'
                        : 'text-gray-700 hover:text-gray-900 hover:bg-gray-100'
                    }`}
                  >
                    {item.label}
                  </button>
                );
              })}
            </nav>

            {/* Mobile Hamburger Button - Hidden on desktop */}
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              class="md:hidden p-2 text-gray-700 hover:text-gray-900 hover:bg-gray-100 rounded-md transition-colors"
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
              <span class="text-sm text-gray-600 hidden sm:inline">
                {connected ? 'Connected' : 'Disconnected'}
              </span>
            </div>
          </div>
        </div>

        {/* Mobile Menu Dropdown - Shown when hamburger is clicked */}
        {mobileMenuOpen && (
          <nav class="md:hidden mt-4 pb-2 border-t border-gray-200 pt-4">
            <div class="flex flex-col gap-2">
              {views.map(item => {
                const isActive = item.id === currentView;
                return (
                  <button
                    key={item.id}
                    onClick={() => handleNavClick(item.id)}
                    class={`w-full text-left px-4 py-3 text-sm font-medium rounded-md transition-colors ${
                      isActive
                        ? 'bg-blue-100 text-blue-700 hover:bg-blue-200'
                        : 'text-gray-700 hover:text-gray-900 hover:bg-gray-100'
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
