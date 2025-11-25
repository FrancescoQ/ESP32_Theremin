import { h } from 'preact';

/**
 * Card to display a status/metric
 * Reusable component like a "Drupal block" :)
 */
export function StatusCard({ title, value, unit, color = 'blue' }) {
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
    </div>
  );
}
