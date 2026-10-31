/**
 * @file PatientMonitorHUD.tsx
 * @brief Real-time animated ECG cardiac telemetry and vital signs HUD.
 * Canvas-based continuous sweeping electrocardiogram waveform.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useEffect, useRef } from 'react';
import { Heart, Activity, Wind, Thermometer, ShieldAlert } from 'lucide-react';
import { AmbulanceTelemetry } from '../../types';

interface PatientMonitorHUDProps {
  ambulance: AmbulanceTelemetry;
  respirationRate?: number;
  temperatureC?: number;
}

export const PatientMonitorHUD: React.FC<PatientMonitorHUDProps> = ({
  ambulance,
  respirationRate = 22,
  temperatureC = 37.2,
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let animationFrameId: number;
    let x = 0;
    const width = canvas.width;
    const height = canvas.height;
    const midY = height / 2;
    let lastY = midY;

    // Clear canvas
    ctx.fillStyle = '#060B13';
    ctx.fillRect(0, 0, width, height);

    // Draw subtle medical grid lines
    ctx.strokeStyle = '#0F2338';
    ctx.lineWidth = 1;
    for (let gx = 0; gx < width; gx += 20) {
      ctx.beginPath();
      ctx.moveTo(gx, 0);
      ctx.lineTo(gx, height);
      ctx.stroke();
    }
    for (let gy = 0; gy < height; gy += 20) {
      ctx.beginPath();
      ctx.moveTo(0, gy);
      ctx.lineTo(width, gy);
      ctx.stroke();
    }

    let t = 0;
    const hr = ambulance.heartRateBpm > 0 ? ambulance.heartRateBpm : 75;
    const beatInterval = Math.max(20, Math.round(1800 / hr));

    const render = () => {
      // Clear a small vertical wipe ahead of the sweep cursor
      ctx.fillStyle = '#060B13';
      ctx.fillRect(x, 0, 16, height);

      // Redraw subtle grid lines in cleared area
      ctx.strokeStyle = '#0F2338';
      ctx.beginPath();
      ctx.moveTo(x + 10, 0);
      ctx.lineTo(x + 10, height);
      ctx.stroke();

      // Compute P-Q-R-S-T cardiac waveform value
      let y = midY;
      const beatPos = t % beatInterval;

      if (beatPos >= 0 && beatPos <= 4) {
        // P-wave
        y = midY - Math.sin((beatPos / 4) * Math.PI) * 7;
      } else if (beatPos === 6) {
        // Q-dip
        y = midY + 9;
      } else if (beatPos === 8) {
        // R-peak (sharp upward spike)
        y = midY - 38;
      } else if (beatPos === 10) {
        // S-dip (sharp downward deflection)
        y = midY + 16;
      } else if (beatPos >= 14 && beatPos <= 22) {
        // T-wave (repolarization)
        y = midY - Math.sin(((beatPos - 14) / 8) * Math.PI) * 11;
      } else {
        // Isoelectric baseline with tiny physiologic noise
        y = midY + (Math.random() - 0.5) * 1.5;
      }

      // Draw ECG trace
      ctx.strokeStyle = '#10B981'; // Medical green trace
      ctx.lineWidth = 2.2;
      ctx.shadowColor = '#10B981';
      ctx.shadowBlur = 6;
      ctx.beginPath();
      ctx.moveTo(x > 0 ? x - 2 : 0, lastY);
      ctx.lineTo(x, y);
      ctx.stroke();
      ctx.shadowBlur = 0; // reset

      lastY = y;
      x += 2;
      t++;

      if (x >= width) {
        x = 0;
      }

      animationFrameId = requestAnimationFrame(render);
    };

    render();

    return () => {
      cancelAnimationFrame(animationFrameId);
    };
  }, [ambulance.heartRateBpm]);

  return (
    <div className="bg-[#0A0F1D] border border-slate-800 rounded-2xl p-4 shadow-xl space-y-3 font-mono">
      {/* Header */}
      <div className="flex items-center justify-between pb-2 border-b border-slate-800 text-xs">
        <div className="flex items-center space-x-2 text-rose-400">
          <Activity size={15} className="animate-pulse" />
          <span className="font-bold uppercase tracking-wider text-white">Live Patient Vitals HUD</span>
        </div>
        <span className="px-2 py-0.5 rounded text-[10px] bg-red-500/20 text-red-400 border border-red-500/30 font-bold">
          {ambulance.priority.replace('_', ' ')}
        </span>
      </div>

      {/* ECG Canvas Waveform */}
      <div className="relative rounded-lg overflow-hidden border border-emerald-950/80 bg-[#060B13] h-28">
        <canvas
          ref={canvasRef}
          width={600}
          height={112}
          className="w-full h-full block"
        />
        <div className="absolute top-2 left-2 text-[10px] text-emerald-400 font-bold tracking-widest bg-emerald-950/60 px-1.5 py-0.5 rounded border border-emerald-500/30">
          LEAD II • 25 mm/s • 10 mm/mV
        </div>
      </div>

      {/* Vital Metrics Grid */}
      <div className="grid grid-cols-4 gap-2 text-center text-xs">
        {/* Heart Rate */}
        <div className="bg-slate-900 p-2 rounded-lg border border-slate-800">
          <div className="text-[10px] text-gray-400 flex items-center justify-center space-x-1">
            <Heart size={10} className="text-rose-500 fill-rose-500 animate-ping" />
            <span>HR (BPM)</span>
          </div>
          <div className="text-xl font-black text-rose-400 mt-0.5">
            {ambulance.heartRateBpm}
          </div>
        </div>

        {/* SpO2 */}
        <div className="bg-slate-900 p-2 rounded-lg border border-slate-800">
          <div className="text-[10px] text-gray-400 flex items-center justify-center space-x-1">
            <Activity size={10} className="text-cyan-400" />
            <span>SpO2</span>
          </div>
          <div className="text-xl font-black text-cyan-400 mt-0.5">
            {ambulance.spO2Percent}%
          </div>
        </div>

        {/* Blood Pressure */}
        <div className="bg-slate-900 p-2 rounded-lg border border-slate-800">
          <div className="text-[10px] text-gray-400 flex items-center justify-center space-x-1">
            <ShieldAlert size={10} className="text-amber-400" />
            <span>BP (mmHg)</span>
          </div>
          <div className="text-base font-black text-white mt-1">
            {ambulance.bloodPressure}
          </div>
        </div>

        {/* Respiration */}
        <div className="bg-slate-900 p-2 rounded-lg border border-slate-800">
          <div className="text-[10px] text-gray-400 flex items-center justify-center space-x-1">
            <Wind size={10} className="text-purple-400" />
            <span>RESP (/m)</span>
          </div>
          <div className="text-xl font-black text-purple-400 mt-0.5">
            {respirationRate}
          </div>
        </div>
      </div>

      {/* Condition / Diagnostic text */}
      <div className="bg-slate-900/60 p-2 rounded border border-slate-800 text-[11px] text-gray-300 flex items-center justify-between">
        <span className="text-gray-400">Clinical Triage:</span>
        <span className="font-bold text-amber-300">{ambulance.patientCondition}</span>
      </div>
    </div>
  );
};
