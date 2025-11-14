import { h } from 'preact';

/**
 * Card per mostrare uno status/metrica
 * Componente riutilizzabile tipo "Drupal block" :)
 */
export function StatusCard({ title, value, unit, icon, color = 'blue' }) {
  const colorClasses = {
    blue: 'bg-blue-500',
    green: 'bg-green-500',
    red: 'bg-red-500',
    yellow: 'bg-yellow-500',
    gray: 'bg-gray-500'
  };

  return (
    <div class="bg-white rounded-lg shadow-md p-6 hover:shadow-lg transition-shadow">
      <div class="flex items-center justify-between mb-4">
        <h3 class="text-sm font-medium text-gray-600 uppercase tracking-wide">
          {title}
        </h3>
        {icon && (
          <div class={`w-10 h-10 rounded-full ${colorClasses[color]} flex items-center justify-center text-white text-xl`}>
            {icon}
          </div>
        )}
      </div>
      
      <div class="flex items-baseline">
        <span class="text-3xl font-bold text-gray-900">
          {value !== null && value !== undefined ? value : '--'}
        </span>
        {unit && (
          <span class="ml-2 text-sm text-gray-500">
            {unit}
          </span>
        )}
      </div>
    </div>
  );
}
