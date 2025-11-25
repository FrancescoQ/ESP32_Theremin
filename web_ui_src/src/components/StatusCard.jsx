import { h } from 'preact';
import { useState } from 'preact/hooks';

/**
 * Card to display a status/metric
 * Reusable component like a "Drupal block" :)
 */
export function StatusCard({ title, value, unit, color = 'blue', description = '' }) {
  const [showDescription, setShowDescription] = useState(false);

  const colorClasses = {
    blue: 'bg-blue-500',
    green: 'bg-green-500',
    red: 'bg-red-500',
    yellow: 'bg-yellow-500',
    gray: 'bg-gray-500'
  };

  return (
    <div class="bg-white dark:bg-gray-800 rounded-lg shadow-md p-6 hover:shadow-lg transition-shadow">
      <div class="flex items-center justify-between mb-4">
        <h3 class="text-sm font-medium text-gray-600 dark:text-gray-400 uppercase tracking-wide">
          {title}
        </h3>
        {description && (
          <button
            onClick={() => setShowDescription(!showDescription)}
            class="w-6 h-6 rounded-full bg-blue-500 hover:bg-blue-600 text-white flex items-center justify-center text-sm font-bold transition-colors cursor-pointer"
            aria-label="Toggle description"
          >
            i
          </button>
        )}
      </div>

      <div class="flex items-baseline">
        <span class="text-3xl font-bold text-gray-900 dark:text-white">
          {value !== null && value !== undefined ? value : '--'}
        </span>
        {unit && (
          <span class="ml-2 text-sm text-gray-500 dark:text-gray-400">
            {unit}
          </span>
        )}
      </div>
      {description && showDescription && (
        <div class="text-sm text-blue-800 dark:text-blue-200 mt-4 p-4 bg-blue-50 dark:bg-blue-900 rounded-md">
          {description}
        </div>
      )}
    </div>
  );
}
