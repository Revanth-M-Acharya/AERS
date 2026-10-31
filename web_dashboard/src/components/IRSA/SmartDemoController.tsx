import React from 'react';
import {
  Play,
  RotateCcw,
  Sparkles,
  CheckCircle2,
  Clock,
  Radio,
  ShieldAlert,
  ArrowRight,
  Activity,
  AlertTriangle,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';

export const SmartDemoController: React.FC = () => {
  const { state, startSmartDemo, stopSmartDemo, resetSimulation } = useSimulation();

  const steps = [
    { num: 1, title: 'Dispatch Baseline', desc: 'AMB-101 departs from Station Alpha' },
    { num: 2, title: 'Traffic Spike', desc: 'Residency road congestion surges to 92%' },
    { num: 3, title: 'Road Obstruction', desc: 'MG Road blocked; dynamic Dijkstra reroute' },
    { num: 4, title: 'Trauma Diversion', desc: 'City General full; redirected to Apollo Trauma' },
    { num: 5, title: 'Critical Node Drop', desc: 'Node #2 goes offline; bypass corridor engaged' },
    { num: 6, title: 'Heavy Rainfall', desc: 'Adverse weather friction & speed cap applied' },
    { num: 7, title: 'Expressway Preempt', desc: 'Final green wave cleared at Airport Corridor' },
    { num: 8, title: 'Safe Arrival', desc: 'Mission accomplished with 0 stops' },
  ];

  return (
    <div className="bg-[#0B101D] border border-cyan-500/40 rounded-xl p-5 shadow-2xl shadow-cyan-950/20 flex flex-col space-y-4 relative overflow-hidden">
      {/* Background Cyber Accent Glow */}
      <div className="absolute top-0 right-0 w-80 h-32 bg-cyan-500/5 blur-3xl pointer-events-none" />

      {/* Header & Controls */}
      <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between border-b border-gray-800 pb-3 gap-3">
        <div className="flex items-center space-x-3">
          <div className="p-2.5 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600 text-black shadow-lg shadow-cyan-500/20">
            <Sparkles size={20} className="text-black font-bold" />
          </div>
          <div>
            <div className="flex items-center space-x-2">
              <h3 className="font-extrabold text-white text-base tracking-wide">
                JUDGE SMART DEMO ORCHESTRATOR
              </h3>
              <span className="px-2 py-0.5 text-[10px] font-mono font-bold bg-cyan-500/20 text-cyan-300 rounded border border-cyan-500/30">
                1-CLICK AUTOMATED FLOW
              </span>
            </div>
            <p className="text-xs text-gray-400 mt-0.5">
              Automated 8-step live demonstration showcasing IRSA multi-criteria rerouting and QNX failover
            </p>
          </div>
        </div>

        <div className="flex items-center space-x-2 w-full sm:w-auto">
          {state.smartDemoActive ? (
            <button
              onClick={stopSmartDemo}
              className="px-4 py-2 bg-rose-500/20 hover:bg-rose-500/30 text-rose-300 border border-rose-500/40 rounded-lg text-xs font-mono font-bold flex items-center justify-center space-x-1.5 transition shadow-lg w-full sm:w-auto"
            >
              <RotateCcw size={14} />
              <span>STOP DEMO</span>
            </button>
          ) : (
            <button
              onClick={startSmartDemo}
              className="px-5 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-black font-extrabold rounded-lg text-xs font-mono tracking-wider flex items-center justify-center space-x-2 transition shadow-lg shadow-cyan-500/30 w-full sm:w-auto"
            >
              <Play size={14} fill="currentColor" />
              <span>EXECUTE SMART DEMO</span>
            </button>
          )}
        </div>
      </div>

      {/* Live Narration Banner */}
      {state.smartDemoNarration && (
        <div className="bg-gradient-to-r from-cyan-950/60 to-blue-950/60 border border-cyan-500/40 rounded-lg p-3 flex items-center space-x-3 text-cyan-200 text-xs font-mono animate-fade-in shadow-inner">
          <Radio size={16} className="text-cyan-400 animate-pulse flex-shrink-0" />
          <div className="flex-1 font-semibold">{state.smartDemoNarration}</div>
        </div>
      )}

      {/* 8-Step Timeline Horizontal Tracker */}
      <div className="grid grid-cols-2 sm:grid-cols-4 lg:grid-cols-8 gap-2 pt-1">
        {steps.map((st) => {
          const isDone = state.smartDemoStep > st.num;
          const isCurrent = state.smartDemoStep === st.num;

          return (
            <div
              key={st.num}
              className={`p-2.5 rounded-lg border text-left flex flex-col justify-between transition-all duration-300 ${
                isCurrent
                  ? 'bg-cyan-950/50 border-cyan-400 shadow-md shadow-cyan-950/50 scale-[1.02]'
                  : isDone
                  ? 'bg-emerald-950/20 border-emerald-500/30 text-emerald-400'
                  : 'bg-gray-900/40 border-gray-800 text-gray-500'
              }`}
            >
              <div className="flex items-center justify-between mb-1.5">
                <span
                  className={`text-[10px] font-mono font-bold px-1.5 py-0.5 rounded ${
                    isCurrent
                      ? 'bg-cyan-500 text-black font-extrabold'
                      : isDone
                      ? 'bg-emerald-500/20 text-emerald-300'
                      : 'bg-gray-800 text-gray-500'
                  }`}
                >
                  STEP {st.num}
                </span>
                {isDone && <CheckCircle2 size={12} className="text-emerald-400" />}
                {isCurrent && (
                  <span className="w-2 h-2 rounded-full bg-cyan-400 animate-ping" />
                )}
              </div>

              <div>
                <span
                  className={`text-[11px] font-bold block leading-tight ${
                    isCurrent ? 'text-cyan-200' : isDone ? 'text-gray-200' : 'text-gray-400'
                  }`}
                >
                  {st.title}
                </span>
                <span className="text-[9px] text-gray-500 block mt-1 line-clamp-2 leading-tight">
                  {st.desc}
                </span>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};
