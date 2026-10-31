import React, { useState, useEffect } from 'react';
import { Play, Pause, RotateCcw, FastForward, Clock, ShieldCheck, CheckCircle2 } from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const MissionReplayPlayer: React.FC = () => {
  const { state } = useSimulation();
  const [isPlaying, setIsPlaying] = useState<boolean>(false);
  const [speed, setSpeed] = useState<number>(1);
  const [scrubberValue, setScrubberValue] = useState<number>(0);

  // Playback timer
  useEffect(() => {
    if (!isPlaying) return;
    const interval = setInterval(() => {
      setScrubberValue((v) => {
        if (v >= 100) {
          setIsPlaying(false);
          return 100;
        }
        return Math.min(100, v + 1 * speed);
      });
    }, 100);
    return () => clearInterval(interval);
  }, [isPlaying, speed]);

  const stages = [
    { label: 'Incident Reported', pct: 0 },
    { label: 'AMB-101 Dispatched', pct: 15 },
    { label: 'Int 1 Preemption', pct: 35 },
    { label: 'Int 2 Preemption', pct: 65 },
    { label: 'Int 3 Preemption', pct: 85 },
    { label: 'Hospital Handover', pct: 100 },
  ];

  return (
    <div className="glass-panel p-6 rounded-xl border border-gray-800 space-y-6">
      <div className="flex items-center justify-between pb-3 border-b border-gray-800">
        <div>
          <h3 className="font-heading-cyber font-bold text-white text-base">
            MISSION AUDIT & BLACKBOX REPLAY PLAYER
          </h3>
          <p className="text-xs text-gray-400 font-mono">
            Mission ID: #MSN-2026-0819 (Priority 1 - Cardiac Trauma)
          </p>
        </div>
        <div className="flex items-center space-x-2">
          <span className="text-[10px] font-mono px-2 py-0.5 rounded bg-emerald-500/20 text-emerald-400 border border-emerald-500/30">
            VERIFIED RESOLUTION
          </span>
        </div>
      </div>

      {/* Scrubber Bar */}
      <div className="space-y-2">
        <div className="flex justify-between text-xs font-mono text-gray-400">
          <span>T+ 00:00</span>
          <span className="text-cyan-400 font-bold">
            Playback Progress: {Math.round(scrubberValue)}%
          </span>
          <span>T+ 00:38</span>
        </div>

        <input
          type="range"
          min="0"
          max="100"
          value={scrubberValue}
          onChange={(e) => setScrubberValue(Number(e.target.value))}
          className="w-full h-2 bg-gray-800 rounded-lg appearance-none cursor-pointer accent-cyan-400"
        />

        {/* Milestone Markers */}
        <div className="grid grid-cols-6 text-[10px] text-gray-500 font-mono pt-1 text-center">
          {stages.map((stg) => (
            <div key={stg.pct} className={scrubberValue >= stg.pct ? 'text-cyan-400 font-bold' : ''}>
              {stg.label}
            </div>
          ))}
        </div>
      </div>

      {/* Control Buttons */}
      <div className="flex items-center justify-between pt-2">
        <div className="flex items-center space-x-3">
          <button
            onClick={() => setIsPlaying(!isPlaying)}
            className="flex items-center space-x-2 bg-cyan-500 hover:bg-cyan-400 text-gray-950 font-bold px-4 py-2 rounded-lg text-xs shadow-lg shadow-cyan-500/20 transition-all"
          >
            {isPlaying ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4" />}
            <span>{isPlaying ? 'Pause' : 'Play Replay'}</span>
          </button>

          <button
            onClick={() => {
              setIsPlaying(false);
              setScrubberValue(0);
            }}
            className="flex items-center space-x-1.5 bg-gray-800 hover:bg-gray-700 text-gray-300 px-3 py-2 rounded-lg text-xs border border-gray-700 transition"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Rewind</span>
          </button>
        </div>

        {/* Speed Multipliers */}
        <div className="flex items-center space-x-1.5 bg-gray-900 border border-gray-800 p-1 rounded-lg text-xs">
          {[1, 2, 5].map((s) => (
            <button
              key={s}
              onClick={() => setSpeed(s)}
              className={`px-2.5 py-1 rounded font-mono font-bold text-xs transition ${
                speed === s
                  ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/30'
                  : 'text-gray-400 hover:text-gray-200'
              }`}
            >
              {s}x
            </button>
          ))}
        </div>
      </div>
    </div>
  );
};
