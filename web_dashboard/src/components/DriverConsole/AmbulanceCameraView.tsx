/**
 * @file AmbulanceCameraView.tsx
 * @brief 4-Channel Ambulance Camera Simulation HUD.
 * Simulates Front, Rear, Cabin, and Patient feeds with tactical HUD overlay and Night-Vision/Thermal mode.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useState, useEffect, useRef } from 'react';
import { CameraFeedId } from '../../types';
import { Video, Eye, Radio, Maximize2, ShieldCheck, Sun, Moon } from 'lucide-react';

interface AmbulanceCameraViewProps {
  speedKmh: number;
}

export const AmbulanceCameraView: React.FC<AmbulanceCameraViewProps> = ({ speedKmh }) => {
  const [activeCam, setActiveCam] = useState<CameraFeedId>('FRONT');
  const [isThermal, setIsThermal] = useState<boolean>(false);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    let frameId: number;
    let offset = 0;
    const width = canvas.width;
    const height = canvas.height;

    const render = () => {
      // Background tone based on camera and thermal mode
      if (isThermal) {
        ctx.fillStyle = '#061325';
      } else if (activeCam === 'FRONT') {
        ctx.fillStyle = '#0A1220';
      } else if (activeCam === 'REAR') {
        ctx.fillStyle = '#0F1626';
      } else if (activeCam === 'CABIN') {
        ctx.fillStyle = '#121824';
      } else {
        ctx.fillStyle = '#081820';
      }
      ctx.fillRect(0, 0, width, height);

      // Camera specific simulated elements
      if (activeCam === 'FRONT' || activeCam === 'REAR') {
        // Perspective road lines moving with speed
        offset = (offset + Math.max(1, speedKmh * 0.15)) % 40;

        ctx.strokeStyle = isThermal ? '#38BDF8' : '#334155';
        ctx.lineWidth = 2;

        // Road edges
        ctx.beginPath();
        ctx.moveTo(width * 0.1, height);
        ctx.lineTo(width * 0.42, height * 0.35);
        ctx.lineTo(width * 0.58, height * 0.35);
        ctx.lineTo(width * 0.9, height);
        ctx.stroke();

        // Center dashed lines
        ctx.strokeStyle = isThermal ? '#F43F5E' : '#F59E0B';
        ctx.lineWidth = 3;
        for (let y = height * 0.4 + offset; y < height; y += 40) {
          const progress = (y - height * 0.4) / (height * 0.6);
          const cx = width / 2;
          ctx.beginPath();
          ctx.moveTo(cx, y);
          ctx.lineTo(cx, y + 16 * progress);
          ctx.stroke();
        }

        // Simulated AI bounding box for vehicle ahead
        if (activeCam === 'FRONT') {
          ctx.strokeStyle = '#00F0FF';
          ctx.lineWidth = 1.5;
          ctx.strokeRect(width * 0.46, height * 0.45, 45, 30);
          ctx.fillStyle = '#00F0FF';
          ctx.font = '9px monospace';
          ctx.fillText('LEAD VEH: 28m [CLEARED]', width * 0.42, height * 0.42);
        }
      } else if (activeCam === 'CABIN') {
        // Driver Cockpit HUD lines
        ctx.strokeStyle = '#1E293B';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(width * 0.3, height * 0.7, 50, 0, Math.PI * 2);
        ctx.stroke();
        ctx.fillStyle = '#38BDF8';
        ctx.font = '10px monospace';
        ctx.fillText('COCKPIT CONSOLE ACTIVE • QNX RTOS OS 8.0', width * 0.15, height * 0.3);
      } else {
        // Patient Chamber HUD
        ctx.strokeStyle = '#10B981';
        ctx.lineWidth = 1;
        ctx.strokeRect(width * 0.25, height * 0.3, width * 0.5, height * 0.5);
        ctx.fillStyle = '#10B981';
        ctx.font = '10px monospace';
        ctx.fillText('PATIENT BED CAM: TRIAGE MONITORED', width * 0.28, height * 0.26);
      }

      // Subtle CRT / Cam scanline overlay
      ctx.fillStyle = 'rgba(255, 255, 255, 0.02)';
      for (let sl = 0; sl < height; sl += 4) {
        ctx.fillRect(0, sl, width, 1.5);
      }

      frameId = requestAnimationFrame(render);
    };

    render();

    return () => {
      cancelAnimationFrame(frameId);
    };
  }, [activeCam, speedKmh, isThermal]);

  const cams: Array<{ id: CameraFeedId; label: string }> = [
    { id: 'FRONT', label: 'CAM 1 (Front Road)' },
    { id: 'REAR', label: 'CAM 2 (Rear View)' },
    { id: 'CABIN', label: 'CAM 3 (Cabin/Cockpit)' },
    { id: 'PATIENT', label: 'CAM 4 (Patient Bay)' },
  ];

  return (
    <div className="bg-[#0A0F1D] border border-slate-800 rounded-2xl p-4 shadow-xl space-y-3 font-mono">
      {/* Cam Bar Controls */}
      <div className="flex items-center justify-between pb-2 border-b border-slate-800 text-xs">
        <div className="flex items-center space-x-2 text-cyan-400">
          <Video size={16} />
          <span className="font-bold uppercase tracking-wider text-white">4-Channel Vehicle Cams</span>
        </div>

        <div className="flex items-center space-x-2">
          <button
            onClick={() => setIsThermal(!isThermal)}
            className={`px-2 py-1 rounded text-[10px] font-bold border transition ${
              isThermal
                ? 'bg-rose-500/20 text-rose-300 border-rose-500'
                : 'bg-slate-800 text-gray-400 border-slate-700 hover:text-white'
            }`}
          >
            {isThermal ? 'THERMAL ON' : 'OPTICAL HUD'}
          </button>
        </div>
      </div>

      {/* Video Viewport Canvas */}
      <div className="relative rounded-xl overflow-hidden border border-cyan-500/30 bg-black aspect-video max-h-56">
        <canvas
          ref={canvasRef}
          width={480}
          height={270}
          className="w-full h-full block object-cover"
        />

        {/* Tactical On-Screen Display (OSD) Overlay */}
        <div className="absolute top-2 left-2 flex items-center space-x-2 text-[10px] text-white bg-black/60 px-2 py-0.5 rounded backdrop-blur">
          <span className="w-2 h-2 rounded-full bg-rose-500 animate-ping" />
          <span className="font-bold text-rose-400">REC ●</span>
          <span>{activeCam} CH</span>
          <span className="text-gray-400">1080p60</span>
        </div>

        <div className="absolute top-2 right-2 text-[10px] text-cyan-300 font-mono bg-black/60 px-2 py-0.5 rounded backdrop-blur">
          AI PERCEPTION ACTIVE
        </div>

        <div className="absolute bottom-2 left-2 text-[10px] text-gray-300 font-mono bg-black/60 px-2 py-0.5 rounded backdrop-blur">
          SPEED: {speedKmh} km/h • IRSA SYNC
        </div>
      </div>

      {/* Cam Switcher Buttons */}
      <div className="grid grid-cols-4 gap-2 text-xs">
        {cams.map((c) => (
          <button
            key={c.id}
            onClick={() => setActiveCam(c.id)}
            className={`py-1.5 px-2 rounded-lg border text-center transition ${
              activeCam === c.id
                ? 'bg-cyan-500/20 text-cyan-300 border-cyan-500 font-bold shadow-[0_0_10px_rgba(0,240,255,0.2)]'
                : 'bg-slate-900 border-slate-800 text-gray-400 hover:text-white'
            }`}
          >
            {c.label}
          </button>
        ))}
      </div>
    </div>
  );
};
