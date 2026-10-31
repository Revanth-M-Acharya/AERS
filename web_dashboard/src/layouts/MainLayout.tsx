import React, { useState } from 'react';
import { Sidebar, PageId } from '../components/Sidebar';
import { TopHeader } from '../components/TopHeader';
import { DashboardPage } from '../pages/DashboardPage';
import { CityMapPage } from '../pages/CityMapPage';
import { CommandCenterPage } from '../pages/CommandCenterPage';
import { EmergencyCallsPage } from '../pages/EmergencyCallsPage';
import { AmbulancesPage } from '../pages/AmbulancesPage';
import { TrafficSignalsPage } from '../pages/TrafficSignalsPage';
import { HospitalsPage } from '../pages/HospitalsPage';
import { DigitalTwinPage } from '../pages/DigitalTwinPage';
import { AnalyticsPage } from '../pages/AnalyticsPage';
import { ReplayPage } from '../pages/ReplayPage';
import { LogsPage } from '../pages/LogsPage';
import { SettingsPage } from '../pages/SettingsPage';
import { IRSAPage } from '../pages/IRSAPage';
import { PrioritySchedulerPage } from '../pages/PrioritySchedulerPage';
import { DriverConsolePage } from '../pages/DriverConsolePage';
import { AlertTriangle, ServerCrash, X } from 'lucide-react';
import { useSimulation } from '../hooks/useSimulation';

export const MainLayout: React.FC = () => {
  const [activePage, setActivePage] = useState<PageId>('dashboard');
  const [dismissedAlert, setDismissedAlert] = useState<string | null>(null);
  const { state } = useSimulation();

  const renderPage = () => {
    switch (activePage) {
      case 'dashboard':
        return <DashboardPage />;
      case 'priority-scheduler':
        return <PrioritySchedulerPage />;
      case 'driver-console':
        return <DriverConsolePage />;
      case 'irsa':
        return <IRSAPage />;
      case 'city-map':
        return <CityMapPage />;
      case 'command-center':
        return <CommandCenterPage />;
      case 'emergency-calls':
        return <EmergencyCallsPage />;
      case 'ambulances':
        return <AmbulancesPage />;
      case 'traffic-signals':
        return <TrafficSignalsPage />;
      case 'hospitals':
        return <HospitalsPage />;
      case 'digital-twin':
        return <DigitalTwinPage />;
      case 'analytics':
        return <AnalyticsPage />;
      case 'replay':
        return <ReplayPage />;
      case 'logs':
        return <LogsPage />;
      case 'settings':
        return <SettingsPage />;
      default:
        return <DashboardPage />;
    }
  };

  return (
    <div className="flex h-screen w-screen overflow-hidden bg-[#0A0E17]">
      {/* Navigation Sidebar */}
      <Sidebar activePage={activePage} onSelectPage={setActivePage} />

      {/* Main Operations Content Area */}
      <div className="flex-1 flex flex-col h-screen overflow-hidden relative">
        <TopHeader />

        {/* Node Failure Recovery Critical Banner Alert (Part 4) */}
        {state.nodeFailureAlert && dismissedAlert !== state.nodeFailureAlert && (
          <div className="bg-gradient-to-r from-rose-950/90 via-red-900/90 to-rose-950/90 border-b border-rose-500/60 px-6 py-2.5 flex items-center justify-between shadow-2xl animate-pulse z-20">
            <div className="flex items-center space-x-3 text-rose-100 font-mono text-xs">
              <ServerCrash size={18} className="text-rose-400 flex-shrink-0 animate-bounce" />
              <span className="font-extrabold tracking-wide text-rose-200">
                {state.nodeFailureAlert}
              </span>
            </div>
            <button
              onClick={() => setDismissedAlert(state.nodeFailureAlert)}
              className="text-rose-300 hover:text-white p-1 rounded hover:bg-rose-800/40 transition"
              title="Acknowledge & Dismiss Alert"
            >
              <X size={16} />
            </button>
          </div>
        )}

        {/* Weather Overlays */}
        {state.weather.mode === 'NIGHT' && (
          <div className="pointer-events-none absolute inset-0 bg-blue-950/20 mix-blend-multiply z-10" />
        )}
        {state.weather.mode === 'FOG' && (
          <div className="pointer-events-none absolute inset-0 bg-gray-500/10 backdrop-blur-[1px] z-10" />
        )}

        <main className="flex-1 p-6 overflow-y-auto z-0">{renderPage()}</main>
      </div>
    </div>
  );
};
