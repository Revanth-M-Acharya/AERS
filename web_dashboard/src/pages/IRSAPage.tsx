import React from 'react';
import { SmartDemoController } from '../components/IRSA/SmartDemoController';
import { RouteRecommendationView } from '../components/IRSA/RouteRecommendationView';
import { FaultInjectionMatrix } from '../components/IRSA/FaultInjectionMatrix';
import { IRSAPanel } from '../components/IRSA/IRSAPanel';
import { InteractiveCityMap } from '../components/CityMap/InteractiveCityMap';
import { Activity, ShieldAlert, Cpu, Radio, Network } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';

export const IRSAPage: React.FC = () => {
  const { state, mode } = useSimulation();

  return (
    <div className="space-y-6 pb-12 animate-fade-in">
      {/* Top Banner / Operations Room Header */}
      <div className="flex flex-col md:flex-row items-start md:items-center justify-between bg-gradient-to-r from-[#0C1527] to-[#0A0E17] border border-gray-800 p-5 rounded-2xl shadow-2xl gap-4">
        <div className="flex items-center space-x-3.5">
          <div className="p-3 rounded-xl bg-cyan-500/10 border border-cyan-500/30 text-cyan-400">
            <Network size={24} />
          </div>
          <div>
            <div className="flex items-center space-x-2">
              <h1 className="text-xl font-extrabold text-white tracking-wide">
                INTELLIGENT ROAD SITUATION AWARENESS (IRSA) OPERATIONS CENTER
              </h1>
              <span className="px-2.5 py-0.5 rounded-full text-[10px] font-mono font-bold bg-cyan-500/20 text-cyan-300 border border-cyan-500/30">
                CITY MESH v8.0
              </span>
            </div>
            <p className="text-xs text-gray-400 mt-1">
              Autonomous multi-criteria emergency corridor evaluation • QNX SDP 8.0 Real-time Failover Engine
            </p>
          </div>
        </div>

        {/* Live Status Indicators */}
        <div className="flex items-center space-x-4 font-mono text-xs">
          <div className="bg-gray-900/80 px-3 py-1.5 rounded-lg border border-gray-800 flex items-center space-x-2">
            <span className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span className="text-gray-400">MODE:</span>
            <span className="font-bold text-cyan-300">{mode}</span>
          </div>
          <div className="bg-gray-900/80 px-3 py-1.5 rounded-lg border border-gray-800 flex items-center space-x-2">
            <span className="text-gray-400">ATMOSPHERE:</span>
            <span className="font-bold text-amber-300">{state.weather.mode}</span>
          </div>
        </div>
      </div>

      {/* 1. Judge Smart Demo Orchestrator (1-click judge demo) */}
      <SmartDemoController />

      {/* 2. Interactive City Map Preview (12 Intersections, 22 Roads) */}
      <div className="h-[480px] w-full rounded-2xl overflow-hidden border border-gray-800 shadow-2xl">
        <InteractiveCityMap />
      </div>

      {/* 3. Multi-Criteria Route Recommendation Engine */}
      <RouteRecommendationView />

      {/* 4. Chaos Engineering & Fault Injection Matrix (11 Scenarios) */}
      <FaultInjectionMatrix />

      {/* 5. Comprehensive Road Health Table & AI Camera Perception Array */}
      <IRSAPanel />
    </div>
  );
};
