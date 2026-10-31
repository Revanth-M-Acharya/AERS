import React from 'react';
import { BarChart3, TrendingDown, Clock, Activity, Zap } from 'lucide-react';
import { MetricGraphs } from '../components/Analytics/MetricGraphs';

export const AnalyticsPage: React.FC = () => {
  return (
    <div className="space-y-6">
      <div className="glass-panel p-6 rounded-xl border border-gray-800">
        <h3 className="font-heading-cyber font-bold text-white text-lg flex items-center space-x-2">
          <BarChart3 className="w-5 h-5 text-cyan-400" />
          <span>MISSION PERFORMANCE ANALYTICS & STATISTICAL INTELLIGENCE</span>
        </h3>
        <p className="text-xs text-gray-400 font-mono mt-1">
          Empirical Evaluation of Emergency Route Optimization & Preemption Latency
        </p>
      </div>

      <MetricGraphs />
    </div>
  );
};
