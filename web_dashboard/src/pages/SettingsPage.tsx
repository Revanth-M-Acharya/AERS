import React, { useState } from 'react';
import { Settings, Wifi, Server, Sliders, ShieldCheck, Save, RotateCcw } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';
import { wsService } from '../services/websocketService';

export const SettingsPage: React.FC = () => {
  const { mode, setMode, wsStatus } = useSimulation();
  const [wsUrl, setWsUrl] = useState<string>(wsService.getUrl());
  const [savedNotice, setSavedNotice] = useState<boolean>(false);

  const handleSave = () => {
    wsService.setUrl(wsUrl);
    setSavedNotice(true);
    setTimeout(() => setSavedNotice(false), 3000);
  };

  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <Settings className="w-5 h-5 text-cyan-400" />
          <span>DIGITAL TWIN OPERATIONS CENTER CONFIGURATION</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Networking, WebSocket Bridge Endpoints & QNX Cluster Address Configuration
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* WebSocket Bridge Configuration */}
        <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-4">
          <div className="flex items-center space-x-2 text-sm font-bold text-white border-b border-gray-800 pb-3">
            <Wifi className="w-4 h-4 text-cyan-400" />
            <span>WebSocket Live Hardware Bridge</span>
          </div>

          <div className="space-y-2">
            <label className="text-xs text-gray-400 font-mono block">
              Coordinator WebSocket Bridge URL:
            </label>
            <input
              type="text"
              value={wsUrl}
              onChange={(e) => setWsUrl(e.target.value)}
              className="w-full bg-gray-900 border border-gray-800 rounded-lg px-3 py-2 text-xs font-mono text-cyan-300 focus:outline-none focus:border-cyan-500"
              placeholder="ws://10.204.179.4:9001 or ws://localhost:9001"
            />
            <p className="text-[11px] text-gray-500 font-mono">
              Used when switching to "Live Mode" to stream real Raspberry Pi hardware frames.
            </p>
          </div>

          <div className="flex items-center justify-between pt-2">
            <div className="text-xs font-mono">
              <span className="text-gray-400">Socket Status: </span>
              <span
                className={`font-bold ${
                  wsStatus === 'CONNECTED'
                    ? 'text-emerald-400'
                    : wsStatus === 'CONNECTING'
                    ? 'text-amber-400'
                    : 'text-gray-500'
                }`}
              >
                {wsStatus}
              </span>
            </div>

            <button
              onClick={handleSave}
              className="flex items-center space-x-1.5 bg-cyan-600 hover:bg-cyan-500 text-gray-950 font-bold px-3.5 py-1.5 rounded-lg text-xs shadow-md transition"
            >
              <Save className="w-3.5 h-3.5" />
              <span>Apply Changes</span>
            </button>
          </div>

          {savedNotice && (
            <div className="p-2.5 rounded bg-emerald-950/60 border border-emerald-500/40 text-emerald-300 text-xs font-mono text-center">
              ✓ WebSocket endpoint updated successfully!
            </div>
          )}
        </div>

        {/* Physical Raspberry Pi Cluster IP Registry */}
        <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-4">
          <div className="flex items-center space-x-2 text-sm font-bold text-white border-b border-gray-800 pb-3">
            <Server className="w-4 h-4 text-emerald-400" />
            <span>Target QNX SDP 8.0 Cluster Network</span>
          </div>

          <div className="space-y-3 text-xs font-mono text-gray-300">
            <div className="flex justify-between items-center p-2 rounded bg-gray-900/60 border border-gray-800">
              <span>Raspberry Pi 1 (Coordinator):</span>
              <span className="text-cyan-300 font-bold">10.204.179.4 (Master)</span>
            </div>
            <div className="flex justify-between items-center p-2 rounded bg-gray-900/60 border border-gray-800">
              <span>Raspberry Pi 2 (Intersection 1):</span>
              <span className="text-cyan-300 font-bold">10.204.179.245:5001</span>
            </div>
            <div className="flex justify-between items-center p-2 rounded bg-gray-900/60 border border-gray-800">
              <span>Raspberry Pi 3 (Intersection 2):</span>
              <span className="text-cyan-300 font-bold">10.204.179.130:5002</span>
            </div>
            <div className="flex justify-between items-center p-2 rounded bg-gray-900/60 border border-gray-800">
              <span>Raspberry Pi 4 (Intersection 3):</span>
              <span className="text-amber-300 font-bold">10.204.179.144:5003 (Blue Only)</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
