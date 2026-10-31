import React from 'react';
import { InteractiveCityMap } from '../components/CityMap/InteractiveCityMap';
import { ControlPanel } from '../components/SimulationControls/ControlPanel';

export const CityMapPage: React.FC = () => {
  return (
    <div className="space-y-6">
      <div className="h-[620px]">
        <InteractiveCityMap />
      </div>
      <ControlPanel />
    </div>
  );
};
