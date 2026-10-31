import React, { useState } from 'react';
import { Search, Filter, Download, Trash2, ShieldCheck, AlertCircle } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';
import { EventLogEntry } from '../../types';

export const LiveEventTable: React.FC = () => {
  const { state } = useSimulation();
  const [searchTerm, setSearchTerm] = useState<string>('');
  const [levelFilter, setLevelFilter] = useState<string>('ALL');

  const filteredEvents = state.events.filter((evt) => {
    const matchesSearch =
      evt.message.toLowerCase().includes(searchTerm.toLowerCase()) ||
      evt.source.toLowerCase().includes(searchTerm.toLowerCase());
    const matchesLevel = levelFilter === 'ALL' || evt.level === levelFilter;
    return matchesSearch && matchesLevel;
  });

  const exportToJson = () => {
    const blob = new Blob([JSON.stringify(state.events, null, 2)], {
      type: 'application/json',
    });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `smart_aegis_events_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const exportToCsv = () => {
    const headers = 'ID,Timestamp,Source,Level,Message\n';
    const rows = state.events
      .map(
        (e) =>
          `"${e.id}","${e.timestamp}","${e.source}","${e.level}","${e.message.replace(/"/g, '""')}"`
      )
      .join('\n');
    const blob = new Blob([headers + rows], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `smart_aegis_events_${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const getLevelBadgeClass = (level: EventLogEntry['level']) => {
    switch (level) {
      case 'EMERGENCY':
        return 'bg-rose-500/20 text-rose-400 border border-rose-500/30 font-bold animate-pulse';
      case 'ERROR':
        return 'bg-red-500/20 text-red-400 border border-red-500/30 font-bold';
      case 'WARN':
        return 'bg-amber-500/20 text-amber-300 border border-amber-500/30';
      case 'SUCCESS':
        return 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30';
      default:
        return 'bg-cyan-500/10 text-cyan-300 border border-cyan-500/20';
    }
  };

  return (
    <div className="glass-panel p-5 rounded-xl border border-gray-800 space-y-4">
      {/* Search, Filter & Export Toolbar */}
      <div className="flex flex-col sm:flex-row items-center justify-between gap-3">
        {/* Search */}
        <div className="relative w-full sm:w-72">
          <Search className="w-4 h-4 text-gray-500 absolute left-3 top-2.5" />
          <input
            type="text"
            placeholder="Search event logs or sources..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full bg-gray-900 border border-gray-800 rounded-lg pl-9 pr-3 py-1.5 text-xs text-gray-200 placeholder-gray-500 focus:outline-none focus:border-cyan-500"
          />
        </div>

        {/* Filter and Export buttons */}
        <div className="flex items-center space-x-2 w-full sm:w-auto justify-end">
          <div className="flex items-center space-x-1 bg-gray-900 border border-gray-800 rounded-lg p-1 text-xs">
            {['ALL', 'EMERGENCY', 'WARN', 'SUCCESS', 'INFO'].map((lvl) => (
              <button
                key={lvl}
                onClick={() => setLevelFilter(lvl)}
                className={`px-2 py-0.5 rounded font-mono text-[11px] transition ${
                  levelFilter === lvl
                    ? 'bg-cyan-500/20 text-cyan-300 font-bold border border-cyan-500/30'
                    : 'text-gray-400 hover:text-gray-200'
                }`}
              >
                {lvl}
              </button>
            ))}
          </div>

          <button
            onClick={exportToJson}
            className="flex items-center space-x-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 px-3 py-1.5 rounded-lg text-xs border border-gray-700 transition"
            title="Export JSON"
          >
            <Download className="w-3.5 h-3.5" />
            <span>JSON</span>
          </button>

          <button
            onClick={exportToCsv}
            className="flex items-center space-x-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 px-3 py-1.5 rounded-lg text-xs border border-gray-700 transition"
            title="Export CSV"
          >
            <Download className="w-3.5 h-3.5" />
            <span>CSV</span>
          </button>
        </div>
      </div>

      {/* Events Table */}
      <div className="overflow-x-auto max-h-96 border border-gray-800 rounded-lg bg-gray-950/40">
        <table className="w-full text-left text-xs">
          <thead className="bg-gray-900/80 text-gray-400 font-mono text-[11px] uppercase tracking-wider sticky top-0 border-b border-gray-800">
            <tr>
              <th className="py-2.5 px-3">Time</th>
              <th className="py-2.5 px-3">Source</th>
              <th className="py-2.5 px-3">Level</th>
              <th className="py-2.5 px-3">Message</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-gray-800/60 font-mono">
            {filteredEvents.length === 0 ? (
              <tr>
                <td colSpan={4} className="py-8 text-center text-gray-500 italic">
                  No matching log events recorded.
                </td>
              </tr>
            ) : (
              filteredEvents.map((evt) => (
                <tr key={evt.id} className="hover:bg-gray-900/40 transition">
                  <td className="py-2 px-3 text-gray-500 whitespace-nowrap">{evt.timestamp}</td>
                  <td className="py-2 px-3 text-cyan-300 whitespace-nowrap">{evt.source}</td>
                  <td className="py-2 px-3 whitespace-nowrap">
                    <span className={`px-2 py-0.5 rounded text-[10px] ${getLevelBadgeClass(evt.level)}`}>
                      {evt.level}
                    </span>
                  </td>
                  <td className="py-2 px-3 text-gray-200">
                    <div>{evt.message}</div>
                    {evt.details && (
                      <div className="text-[10px] text-gray-500 italic mt-0.5">{evt.details}</div>
                    )}
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
