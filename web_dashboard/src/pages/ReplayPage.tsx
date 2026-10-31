import React from 'react';
import { RotateCcw, Clock, Film } from 'lucide-react';
import { MissionReplayPlayer } from '../components/Replay/MissionReplayPlayer';

export const ReplayPage: React.FC = () => {
  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <RotateCcw className="w-5 h-5 text-cyan-400" />
          <span>MISSION POST-MORTEM & TIMELINE SCRUBBER REPLAY</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Blackbox Event Trace Reconstruction with Variable Speed Multipliers (1x, 2x, 5x)
        </p>
      </div>

      <MissionReplayPlayer />
    </div>
  );
};
