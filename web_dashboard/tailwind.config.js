/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        cyber: {
          dark: '#0A0E17',
          card: '#111827',
          border: '#1F2937',
          accent: '#00F0FF',
          green: '#10B981',
          red: '#EF4444',
          yellow: '#F59E0B',
          blue: '#3B82F6',
          purple: '#8B5CF6',
        }
      },
      animation: {
        'pulse-fast': 'pulse 1s cubic-bezier(0.4, 0, 0.6, 1) infinite',
        'glow': 'glow 2s ease-in-out infinite alternate',
      },
      keyframes: {
        glow: {
          '0%': { boxShadow: '0 0 5px rgba(0, 240, 255, 0.4), 0 0 10px rgba(0, 240, 255, 0.2)' },
          '100%': { boxShadow: '0 0 15px rgba(0, 240, 255, 0.8), 0 0 25px rgba(0, 240, 255, 0.4)' },
        }
      }
    },
  },
  plugins: [],
}
