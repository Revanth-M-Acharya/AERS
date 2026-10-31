/**
 * @file PatientInputForm.tsx
 * @brief Complete Patient Triage & Case Input Form for SMART AEGIS Priority Scheduler.
 * Includes all 12 quick-fill clinical scenarios, comprehensive medical vitals,
 * acute condition flags, resource needs, and environmental controls.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

import React from 'react';
import { PatientCase } from '../../types';
import { PREDEFINED_SCENARIOS } from '../../services/irsaEngine';
import {
  User,
  Heart,
  Activity,
  Thermometer,
  ShieldAlert,
  Zap,
  RotateCcw,
  Sparkles,
  AlertOctagon,
  FileText,
} from 'lucide-react';

interface PatientInputFormProps {
  patient: PatientCase;
  onChange: (updated: PatientCase) => void;
  onReset: () => void;
}

export const PatientInputForm: React.FC<PatientInputFormProps> = ({
  patient,
  onChange,
  onReset,
}) => {
  const handleTextChange = (field: keyof PatientCase, value: any) => {
    onChange({ ...patient, [field]: value });
  };

  const handleToggle = (field: keyof PatientCase) => {
    onChange({ ...patient, [field]: !patient[field] });
  };

  const applyScenario = (name: string) => {
    if (PREDEFINED_SCENARIOS[name]) {
      onChange({ ...PREDEFINED_SCENARIOS[name] });
    }
  };

  const conditionToggles: Array<{ key: keyof PatientCase; label: string; critical?: boolean }> = [
    { key: 'hasCardiacArrest', label: 'Cardiac Arrest', critical: true },
    { key: 'hasHeartAttack', label: 'Heart Attack', critical: true },
    { key: 'hasStroke', label: 'Stroke', critical: true },
    { key: 'hasTrauma', label: 'Trauma', critical: true },
    { key: 'hasBurn', label: 'Fire Burn' },
    { key: 'hasPoisoning', label: 'Poisoning' },
    { key: 'hasAccident', label: 'Road Accident' },
    { key: 'hasHeadInjury', label: 'Head Injury', critical: true },
    { key: 'hasMultipleFractures', label: 'Multiple Fractures' },
    { key: 'hasInternalBleeding', label: 'Internal Bleeding', critical: true },
    { key: 'isPregnant', label: 'Pregnant' },
    { key: 'isDiabetic', label: 'Diabetic' },
    { key: 'hasAsthma', label: 'Asthma' },
    { key: 'hasSeizure', label: 'Seizure' },
    { key: 'hasCancer', label: 'Cancer' },
  ];

  return (
    <div className="bg-[#0F172A] border border-slate-800 rounded-xl p-5 space-y-6 shadow-xl">
      {/* Predefined Scenarios Quick Bar */}
      <div>
        <div className="flex items-center justify-between mb-2.5">
          <div className="flex items-center space-x-2 text-xs font-mono font-bold text-gray-300">
            <Sparkles size={15} className="text-cyan-400" />
            <span className="uppercase tracking-wider">Predefined Quick Scenarios:</span>
          </div>
          <button
            onClick={onReset}
            className="flex items-center space-x-1 text-[11px] font-mono text-gray-400 hover:text-white px-2 py-0.5 rounded bg-slate-800 hover:bg-slate-700 transition"
          >
            <RotateCcw size={12} />
            <span>Reset Form</span>
          </button>
        </div>

        <div className="flex flex-wrap gap-1.5">
          {Object.keys(PREDEFINED_SCENARIOS).map((name) => (
            <button
              key={name}
              type="button"
              onClick={() => applyScenario(name)}
              className="px-2.5 py-1 text-xs font-mono rounded-lg bg-slate-800/90 text-cyan-300 border border-slate-700 hover:bg-cyan-950/40 hover:border-cyan-500/50 hover:text-white transition shadow-sm"
            >
              {name}
            </button>
          ))}
        </div>
      </div>

      {/* Patient Demographics */}
      <div className="space-y-3 pt-2 border-t border-slate-800">
        <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider flex items-center space-x-1.5">
          <User size={14} className="text-cyan-400" />
          <span>Patient Demographics</span>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
          <div>
            <label className="text-gray-400 text-[10px] uppercase">Patient Name</label>
            <input
              type="text"
              value={patient.patientName}
              onChange={(e) => handleTextChange('patientName', e.target.value)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono focus:border-cyan-500 outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Age</label>
            <input
              type="number"
              value={patient.age}
              onChange={(e) => handleTextChange('age', parseInt(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono focus:border-cyan-500 outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Gender</label>
            <select
              value={patient.gender}
              onChange={(e) => handleTextChange('gender', e.target.value)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono focus:border-cyan-500 outline-none"
            >
              <option value="Male">Male</option>
              <option value="Female">Female</option>
              <option value="Other">Other</option>
            </select>
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Weight (kg)</label>
            <input
              type="number"
              value={patient.weightKg}
              onChange={(e) => handleTextChange('weightKg', parseFloat(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono focus:border-cyan-500 outline-none"
            />
          </div>
        </div>
      </div>

      {/* Physiological Vital Signs */}
      <div className="space-y-3 pt-2 border-t border-slate-800">
        <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider flex items-center space-x-1.5">
          <Activity size={14} className="text-rose-400" />
          <span>Physiological Vital Signs</span>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs font-mono">
          <div>
            <label className="text-gray-400 text-[10px] uppercase flex items-center justify-between">
              <span>Heart Rate (BPM)</span>
              <Heart size={10} className="text-rose-500" />
            </label>
            <input
              type="number"
              value={patient.heartRate}
              onChange={(e) => handleTextChange('heartRate', parseInt(e.target.value) || 0)}
              className={`w-full mt-1 bg-slate-900 border rounded p-2 text-white font-mono outline-none ${
                patient.heartRate > 120 || patient.heartRate < 50
                  ? 'border-rose-500 text-rose-300'
                  : 'border-slate-700'
              }`}
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">BP Systolic (mmHg)</label>
            <input
              type="number"
              value={patient.bloodPressureSystolic}
              onChange={(e) => handleTextChange('bloodPressureSystolic', parseInt(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">BP Diastolic (mmHg)</label>
            <input
              type="number"
              value={patient.bloodPressureDiastolic}
              onChange={(e) => handleTextChange('bloodPressureDiastolic', parseInt(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase flex items-center justify-between">
              <span>SpO2 Level (%)</span>
              <span className={patient.spO2 < 90 ? 'text-rose-400' : 'text-emerald-400'}>O2</span>
            </label>
            <input
              type="number"
              value={patient.spO2}
              onChange={(e) => handleTextChange('spO2', parseInt(e.target.value) || 0)}
              className={`w-full mt-1 bg-slate-900 border rounded p-2 text-white font-mono outline-none ${
                patient.spO2 < 90 ? 'border-rose-500 text-rose-300' : 'border-slate-700'
              }`}
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Respiration Rate (/min)</label>
            <input
              type="number"
              value={patient.respirationRate}
              onChange={(e) => handleTextChange('respirationRate', parseInt(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase flex items-center justify-between">
              <span>Temperature (°C)</span>
              <Thermometer size={11} className="text-cyan-400" />
            </label>
            <input
              type="number"
              step="0.1"
              value={patient.temperatureC}
              onChange={(e) => handleTextChange('temperatureC', parseFloat(e.target.value) || 0)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">GCS Score (3-15)</label>
            <input
              type="number"
              min="3"
              max="15"
              value={patient.gcsScore}
              onChange={(e) => handleTextChange('gcsScore', parseInt(e.target.value) || 3)}
              className={`w-full mt-1 bg-slate-900 border rounded p-2 text-white font-mono outline-none ${
                patient.gcsScore <= 8 ? 'border-rose-500 text-rose-300' : 'border-slate-700'
              }`}
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Pain Level (1-10)</label>
            <input
              type="number"
              min="1"
              max="10"
              value={patient.painLevel}
              onChange={(e) => handleTextChange('painLevel', parseInt(e.target.value) || 1)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>
        </div>

        <div className="pt-1">
          <label className="text-gray-400 text-[10px] uppercase font-mono">Blood Loss Severity</label>
          <div className="grid grid-cols-5 gap-2 mt-1 font-mono text-xs">
            {(['None', 'Mild', 'Moderate', 'Severe', 'Massive'] as const).map((loss) => (
              <button
                key={loss}
                type="button"
                onClick={() => handleTextChange('bloodLoss', loss)}
                className={`py-1.5 px-2 rounded border text-center transition ${
                  patient.bloodLoss === loss
                    ? loss === 'Massive' || loss === 'Severe'
                      ? 'bg-rose-500/20 text-rose-300 border-rose-500 font-bold'
                      : 'bg-cyan-500/20 text-cyan-300 border-cyan-500 font-bold'
                    : 'bg-slate-900 border-slate-700 text-gray-400 hover:text-white'
                }`}
              >
                {loss}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Acute Clinical Conditions */}
      <div className="space-y-3 pt-2 border-t border-slate-800">
        <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider flex items-center space-x-1.5">
          <AlertOctagon size={14} className="text-amber-400" />
          <span>Acute Conditions & Diagnostic Flags</span>
        </div>

        <div className="flex flex-wrap gap-2">
          {conditionToggles.map(({ key, label, critical }) => {
            const active = !!patient[key];
            return (
              <button
                key={key}
                type="button"
                onClick={() => handleToggle(key)}
                className={`px-3 py-1.5 rounded-lg text-xs font-mono transition border ${
                  active
                    ? critical
                      ? 'bg-rose-500/25 text-rose-200 border-rose-500 font-bold shadow-[0_0_10px_rgba(244,63,94,0.3)]'
                      : 'bg-cyan-500/20 text-cyan-200 border-cyan-500 font-bold'
                    : 'bg-slate-900/80 text-gray-400 border-slate-700 hover:text-white'
                }`}
              >
                {active ? '● ' : '+ '}
                {label}
              </button>
            );
          })}
        </div>
      </div>

      {/* Resource & Equipment Needs */}
      <div className="space-y-3 pt-2 border-t border-slate-800">
        <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider flex items-center space-x-1.5">
          <FileText size={14} className="text-purple-400" />
          <span>Critical Hospital & Vehicle Resources Required</span>
        </div>

        <div className="grid grid-cols-3 gap-3">
          {[
            { key: 'ventilatorRequired' as const, label: 'Mechanical Ventilator' },
            { key: 'oxygenRequired' as const, label: 'High-Flow Oxygen' },
            { key: 'icuRequired' as const, label: 'Immediate ICU Bed' },
          ].map(({ key, label }) => {
            const active = patient[key];
            return (
              <button
                key={key}
                type="button"
                onClick={() => handleToggle(key)}
                className={`p-3 rounded-lg border text-xs font-mono text-left transition ${
                  active
                    ? 'bg-purple-950/40 text-purple-200 border-purple-500 font-bold shadow-[0_0_10px_rgba(168,85,247,0.3)]'
                    : 'bg-slate-900 border-slate-700 text-gray-400 hover:text-white'
                }`}
              >
                <div className="text-[10px] text-gray-400 uppercase">{active ? 'REQUIRED' : 'NOT REQUIRED'}</div>
                <div className="mt-0.5">{label}</div>
              </button>
            );
          })}
        </div>
      </div>

      {/* Context, Route & Traffic Environment */}
      <div className="space-y-3 pt-2 border-t border-slate-800">
        <div className="text-xs font-mono font-bold text-gray-400 uppercase tracking-wider flex items-center space-x-1.5">
          <ShieldAlert size={14} className="text-blue-400" />
          <span>Route, Traffic & Environmental Modifiers</span>
        </div>

        <div className="grid grid-cols-3 gap-3 text-xs font-mono">
          <div>
            <label className="text-gray-400 text-[10px] uppercase">Estimated Distance (km)</label>
            <input
              type="number"
              step="0.1"
              value={patient.estimatedDistanceKm}
              onChange={(e) => handleTextChange('estimatedDistanceKm', parseFloat(e.target.value) || 1)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            />
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Road Traffic Density</label>
            <select
              value={patient.trafficDensity}
              onChange={(e) => handleTextChange('trafficDensity', e.target.value)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            >
              <option value="Low">Low Density</option>
              <option value="Moderate">Moderate Density</option>
              <option value="Heavy">Heavy Traffic</option>
              <option value="Gridlock">Gridlock Jam</option>
            </select>
          </div>

          <div>
            <label className="text-gray-400 text-[10px] uppercase">Weather Condition</label>
            <select
              value={patient.weather}
              onChange={(e) => handleTextChange('weather', e.target.value)}
              className="w-full mt-1 bg-slate-900 border border-slate-700 rounded p-2 text-white font-mono outline-none"
            >
              <option value="Clear">Clear Skies</option>
              <option value="Rain">Heavy Rain</option>
              <option value="Fog">Dense Fog</option>
              <option value="Storm">Severe Storm</option>
            </select>
          </div>
        </div>

        {/* Tactical Overrides */}
        <div className="grid grid-cols-3 gap-3 pt-2">
          <button
            type="button"
            onClick={() => handleToggle('manualOverride')}
            className={`p-2.5 rounded-lg border text-xs font-mono text-center transition ${
              patient.manualOverride
                ? 'bg-rose-500/20 text-rose-300 border-rose-500 font-bold shadow-[0_0_12px_rgba(244,63,94,0.3)]'
                : 'bg-slate-900 border-slate-700 text-gray-400 hover:text-white'
            }`}
          >
            <Zap size={14} className="mx-auto mb-1" />
            <span>Manual Override (Force P1)</span>
          </button>

          <button
            type="button"
            onClick={() => handleToggle('policeEscortRequired')}
            className={`p-2.5 rounded-lg border text-xs font-mono text-center transition ${
              patient.policeEscortRequired
                ? 'bg-amber-500/20 text-amber-300 border-amber-500 font-bold'
                : 'bg-slate-900 border-slate-700 text-gray-400 hover:text-white'
            }`}
          >
            <span>Police Escort Request</span>
          </button>

          <button
            type="button"
            onClick={() => handleToggle('vipConflict')}
            className={`p-2.5 rounded-lg border text-xs font-mono text-center transition ${
              patient.vipConflict
                ? 'bg-purple-500/20 text-purple-300 border-purple-500 font-bold'
                : 'bg-slate-900 border-slate-700 text-gray-400 hover:text-white'
            }`}
          >
            <span>VIP Convoy Conflict</span>
          </button>
        </div>
      </div>
    </div>
  );
};
