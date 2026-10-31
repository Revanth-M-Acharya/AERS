import React from 'react';
import { ScrollText, Search, Download } from 'lucide-react';
import { LiveEventTable } from '../components/EventLog/LiveEventTable';

export const LogsPage: React.FC = () => {
  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <ScrollText className="w-5 h-5 text-cyan-400" />
          <span>SYSTEM-WIDE EVENT AUDIT LOGS & TELEMETRY ARCHIVE</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Chronological ISO 8601 Trace Log with Multi-Level Filtering and CSV/JSON Export
        </p>
      </div>

      <LiveEventTable />
    </div>
  );
};
