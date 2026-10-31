import React from 'react';
import {
  Navigation,
  CheckCircle2,
  AlertTriangle,
  Clock,
  Milestone,
  ShieldCheck,
  Zap,
  ArrowRight,
  TrendingUp,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';
import { RouteOption } from '../../types';

export const RouteRecommendationView: React.FC = () => {
  const { state, selectRoute } = useSimulation();

  const routes = [
    { key: 'PRIMARY' as const, data: state.routes.primaryRoute, color: 'cyan', badge: 'PRIMARY RECOMMENDATION' },
    { key: 'ALT_A' as const, data: state.routes.altRouteA, color: 'amber', badge: 'ALTERNATIVE A (BYPASS)' },
    { key: 'ALT_B' as const, data: state.routes.altRouteB, color: 'purple', badge: 'ALTERNATIVE B (PERIMETER)' },
  ];

  return (
    <div className="bg-[#0B101D] border border-gray-800 rounded-xl p-5 shadow-2xl flex flex-col space-y-4">
      {/* Header */}
      <div className="flex items-center justify-between border-b border-gray-800 pb-3">
        <div className="flex items-center space-x-2.5">
          <div className="p-2 rounded-lg bg-cyan-500/10 border border-cyan-500/20 text-cyan-400">
            <Navigation size={18} />
          </div>
          <div>
            <h3 className="font-bold text-gray-100 text-sm tracking-wide">
              MULTI-CRITERIA ROUTE RECOMMENDATION ENGINE
            </h3>
            <p className="text-xs text-gray-400">
              Real-time Dijkstra path calculation weighted by congestion, incidents, weather & node health
            </p>
          </div>
        </div>

        <div className="flex items-center space-x-2">
          <span className="text-xs font-mono text-gray-400">ACTIVE CORRIDOR:</span>
          <span className="px-2.5 py-1 text-xs font-mono font-bold rounded bg-cyan-500/20 text-cyan-300 border border-cyan-500/30">
            {state.activeRouteId}
          </span>
        </div>
      </div>

      {/* 3 Side-by-Side Route Cards */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        {routes.map(({ key, data, color, badge }) => {
          const isActive = state.activeRouteId === key;
          const isSelectedBorder =
            color === 'cyan'
              ? isActive ? 'border-cyan-500 shadow-cyan-950/50 shadow-lg' : 'border-gray-800 hover:border-gray-700'
              : color === 'amber'
              ? isActive ? 'border-amber-500 shadow-amber-950/50 shadow-lg' : 'border-gray-800 hover:border-gray-700'
              : isActive ? 'border-purple-500 shadow-purple-950/50 shadow-lg' : 'border-gray-800 hover:border-gray-700';

          return (
            <div
              key={key}
              className={`bg-[#0E1526] rounded-xl p-4 border transition-all duration-200 flex flex-col justify-between ${isSelectedBorder}`}
            >
              <div>
                {/* Card Top Pill & Selection State */}
                <div className="flex items-center justify-between mb-3">
                  <span
                    className={`text-[10px] font-mono font-bold px-2 py-0.5 rounded ${
                      color === 'cyan'
                        ? 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/30'
                        : color === 'amber'
                        ? 'bg-amber-500/20 text-amber-400 border border-amber-500/30'
                        : 'bg-purple-500/20 text-purple-400 border border-purple-500/30'
                    }`}
                  >
                    {badge}
                  </span>
                  {isActive && (
                    <span className="flex items-center text-[10px] font-mono font-bold text-emerald-400">
                      <CheckCircle2 size={12} className="mr-1" /> ACTIVE
                    </span>
                  )}
                </div>

                {/* Route Name */}
                <h4 className="font-bold text-gray-100 text-sm mb-1">{data.name}</h4>

                {/* Traversed Intersections List */}
                <div className="flex items-center flex-wrap gap-1 my-2">
                  {data.nodeIds.map((nid, idx) => (
                    <React.Fragment key={nid}>
                      <span className="px-1.5 py-0.5 rounded bg-gray-800/80 text-[10px] font-mono font-bold text-gray-300">
                        Node #{nid}
                      </span>
                      {idx < data.nodeIds.length - 1 && (
                        <ArrowRight size={10} className="text-gray-600" />
                      )}
                    </React.Fragment>
                  ))}
                </div>

                {/* Metric Summary Grid */}
                <div className="grid grid-cols-2 gap-2 my-3">
                  <div className="bg-gray-900/60 p-2 rounded border border-gray-800/60">
                    <span className="text-gray-400 text-[10px] flex items-center mb-0.5">
                      <Clock size={11} className="mr-1 text-cyan-400" /> ESTIMATED ETA
                    </span>
                    <span className="font-mono text-base font-bold text-cyan-300">
                      {data.etaSeconds}s
                    </span>
                  </div>

                  <div className="bg-gray-900/60 p-2 rounded border border-gray-800/60">
                    <span className="text-gray-400 text-[10px] flex items-center mb-0.5">
                      <Milestone size={11} className="mr-1 text-emerald-400" /> DISTANCE
                    </span>
                    <span className="font-mono text-base font-bold text-gray-200">
                      {data.distanceKm} km
                    </span>
                  </div>
                </div>

                {/* Confidence Bar */}
                <div className="my-2 bg-gray-900/40 p-2 rounded border border-gray-800/50">
                  <div className="flex justify-between text-[11px] mb-1">
                    <span className="text-gray-400 flex items-center">
                      <ShieldCheck size={11} className="mr-1 text-purple-400" /> Confidence Score
                    </span>
                    <span className="font-mono font-bold text-purple-300">{data.confidenceScore}%</span>
                  </div>
                  <div className="w-full bg-gray-800 h-1.5 rounded-full overflow-hidden">
                    <div
                      className="bg-purple-500 h-full rounded-full transition-all duration-300"
                      style={{ width: `${data.confidenceScore}%` }}
                    />
                  </div>
                </div>

                {/* Decision Rationale */}
                <div className="bg-gray-900/80 p-2.5 rounded border border-gray-800/80 text-[11px] text-gray-300 leading-relaxed mt-2">
                  <span className="text-gray-400 font-bold block mb-0.5 text-[10px] uppercase tracking-wider">
                    SELECTION RATIONALE:
                  </span>
                  {data.reason}
                </div>
              </div>

              {/* Action Button */}
              <button
                onClick={() => selectRoute(key)}
                disabled={isActive}
                className={`mt-4 w-full py-2 px-3 rounded-lg text-xs font-mono font-bold flex items-center justify-center transition-all ${
                  isActive
                    ? 'bg-gray-800/60 text-gray-400 cursor-default border border-gray-700/50'
                    : 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/40 hover:bg-cyan-500/30 hover:border-cyan-400 shadow-md'
                }`}
              >
                {isActive ? (
                  <>
                    <CheckCircle2 size={13} className="mr-1.5 text-emerald-400" />
                    CORRIDOR LOCK ENGAGED
                  </>
                ) : (
                  <>
                    <Zap size={13} className="mr-1.5 text-cyan-400" />
                    ACTIVATE THIS CORRIDOR
                  </>
                )}
              </button>
            </div>
          );
        })}
      </div>
    </div>
  );
};
