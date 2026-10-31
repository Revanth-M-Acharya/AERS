/**
 * @file VoiceGuidanceHUD.tsx
 * @brief Voice Navigation Controller & Speech Waveform Annunciator.
 * Connects to Web Speech API and Web Audio chime synthesizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { voiceNav } from '../../services/voiceNavigationService';
import { Volume2, VolumeX, Mic, Radio, Play } from 'lucide-react';

export const VoiceGuidanceHUD: React.FC = () => {
  const [isMuted, setIsMuted] = useState<boolean>(voiceNav.getIsMuted());
  const [isSpeaking, setIsSpeaking] = useState<boolean>(false);
  const [currentText, setCurrentText] = useState<string>('Voice guidance standby.');

  useEffect(() => {
    const unsub = voiceNav.subscribe((text, speaking) => {
      setIsSpeaking(speaking);
      if (text) {
        setCurrentText(text);
      }
    });
    return unsub;
  }, []);

  const handleToggleMute = () => {
    const muted = voiceNav.toggleMute();
    setIsMuted(muted);
  };

  const testPhrases = [
    { label: 'Turn Left', text: 'Turn Left onto MG Road in 200 meters.' },
    { label: 'Turn Right', text: 'Turn Right at Residency Road intersection.' },
    { label: 'Continue Straight', text: 'Continue Straight on primary corridor.' },
    { label: 'Corridor Active', text: 'Green Corridor Active. All signals preempted.' },
    { label: 'Intersection Cleared', text: 'Intersection 2 cleared. Safe transit confirmed.' },
    { label: 'Reduce Speed', text: 'Caution: Congestion ahead. Reduce speed to 40 km/h.' },
    { label: 'Road Block Ahead', text: 'Alert: Road Block Ahead at Intersection 2. Re-routing.' },
    { label: 'Alternative Route', text: 'Alternative Route Found. Queens Road Bypass engaged.' },
  ];

  return (
    <div className="bg-[#0A0F1D] border border-slate-800 rounded-2xl p-4 shadow-xl space-y-3 font-mono">
      {/* Header */}
      <div className="flex items-center justify-between pb-2 border-b border-slate-800 text-xs">
        <div className="flex items-center space-x-2 text-cyan-400">
          <Radio size={16} className={isSpeaking ? 'animate-bounce text-emerald-400' : ''} />
          <span className="font-bold uppercase tracking-wider text-white">Voice Navigation HUD</span>
        </div>

        <button
          onClick={handleToggleMute}
          className={`flex items-center space-x-1.5 px-2.5 py-1 rounded text-[11px] font-bold border transition ${
            isMuted
              ? 'bg-rose-500/20 text-rose-300 border-rose-500/50'
              : 'bg-emerald-500/20 text-emerald-300 border-emerald-500/50'
          }`}
        >
          {isMuted ? <VolumeX size={13} /> : <Volume2 size={13} />}
          <span>{isMuted ? 'MUTED' : 'ACTIVE'}</span>
        </button>
      </div>

      {/* Active Annunciation Banner with Animated Voice Waves */}
      <div className="bg-slate-900/90 border border-cyan-500/30 rounded-xl p-3 flex items-center justify-between">
        <div className="flex items-center space-x-3 overflow-hidden">
          <div className="flex items-center space-x-1 flex-shrink-0">
            <span
              className={`w-1 bg-cyan-400 rounded-full transition-all duration-150 ${
                isSpeaking ? 'h-5 animate-pulse' : 'h-1.5'
              }`}
            />
            <span
              className={`w-1 bg-emerald-400 rounded-full transition-all duration-200 ${
                isSpeaking ? 'h-7 animate-pulse' : 'h-2'
              }`}
            />
            <span
              className={`w-1 bg-cyan-400 rounded-full transition-all duration-100 ${
                isSpeaking ? 'h-4 animate-pulse' : 'h-1'
              }`}
            />
          </div>

          <div className="text-xs text-gray-200 truncate font-semibold">
            {currentText}
          </div>
        </div>

        <span className="text-[10px] text-gray-400 bg-slate-950 px-2 py-0.5 rounded border border-slate-800 flex-shrink-0">
          AUDIO CHIME + TTS
        </span>
      </div>

      {/* Quick Voice Simulation Buttons */}
      <div>
        <div className="text-[10px] text-gray-400 uppercase tracking-widest mb-1.5">
          Simulate Tactical Audio Prompts:
        </div>
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-1.5">
          {testPhrases.map((phrase) => (
            <button
              key={phrase.label}
              onClick={() => voiceNav.speak(phrase.text, phrase.label.includes('Block') ? 'HIGH' : 'NORMAL')}
              className="px-2 py-1.5 rounded bg-slate-900 border border-slate-800 hover:border-cyan-500/50 hover:bg-cyan-950/30 text-gray-300 hover:text-white text-[11px] text-left truncate flex items-center space-x-1 transition"
            >
              <Play size={10} className="text-cyan-400 flex-shrink-0" />
              <span className="truncate">{phrase.label}</span>
            </button>
          ))}
        </div>
      </div>
    </div>
  );
};
