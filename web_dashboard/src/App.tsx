import React from 'react';
import { SimulationProvider } from './contexts/SimulationContext';
import { MainLayout } from './layouts/MainLayout';

export function App() {
  return (
    <SimulationProvider>
      <MainLayout />
    </SimulationProvider>
  );
}

export default App;
