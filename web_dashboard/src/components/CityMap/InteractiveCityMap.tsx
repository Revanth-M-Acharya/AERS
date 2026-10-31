import React, { useState, useRef, useMemo } from 'react';
import {
  ZoomIn,
  ZoomOut,
  Maximize2,
  RotateCcw,
  Navigation,
  Activity,
  Radio,
  Building,
  Home,
  CheckCircle2,
  AlertTriangle,
  AlertOctagon,
  ShieldAlert,
  Cpu,
  Video,
  Eye,
  Siren,
  Clock,
  Zap,
  X,
  Gauge,
  Thermometer,
  Wifi,
  Sparkles,
} from 'lucide-react';
import { useSimulation } from '../../hooks/useSimulation';
import { IntersectionState, RoadData, Landmark } from '../../types';

export const InteractiveCityMap: React.FC = () => {
  const {
    state,
    selectRoute,
    toggleNodeFailure,
    toggleRoadBlocked,
    toggleAccident,
    toggleFlood,
  } = useSimulation();

  const [zoom, setZoom] = useState<number>(1);
  const [pan, setPan] = useState<{ x: number; y: number }>({ x: 0, y: 0 });
  const [isDragging, setIsDragging] = useState<boolean>(false);
  const [dragStart, setDragStart] = useState<{ x: number; y: number }>({ x: 0, y: 0 });

  // Inspection states for interactive popups
  const [selectedNode, setSelectedNode] = useState<IntersectionState | null>(null);
  const [selectedRoad, setSelectedRoad] = useState<RoadData | null>(null);

  // Active route definition
  const activeRoute = useMemo(() => {
    if (state.activeRouteId === 'ALT_A') return state.routes.altRouteA;
    if (state.activeRouteId === 'ALT_B') return state.routes.altRouteB;
    return state.routes.primaryRoute;
  }, [state.activeRouteId, state.routes]);

  // Map coordinate lookup for nodes
  const nodePositionMap = useMemo(() => {
    const map = new Map<number, { x: number; y: number }>();
    state.intersections.forEach((n) => map.set(n.id, { x: n.x, y: n.y }));
    return map;
  }, [state.intersections]);

  // Compute smooth ambulance coordinates along the active corridor
  const ambulanceCoords = useMemo(() => {
    const percent = state.ambulance.positionPercent;
    const startPoint = { x: 90, y: 280 }; // Station Alpha
    const targetHospital = state.landmarks.find((l) => l.id === state.hospital.id) || { x: 720, y: 350 };

    // Build ordered route coordinates
    const waypoints: { x: number; y: number }[] = [startPoint];
    activeRoute.nodeIds.forEach((nid) => {
      const pos = nodePositionMap.get(nid);
      if (pos) waypoints.push(pos);
    });
    waypoints.push({ x: targetHospital.x, y: targetHospital.y });

    if (waypoints.length < 2) {
      return { x: startPoint.x, y: startPoint.y, angle: 0 };
    }

    // Segment-based interpolation
    const numSegments = waypoints.length - 1;
    const normalizedProgress = (percent / 100) * numSegments;
    const segmentIndex = Math.min(Math.floor(normalizedProgress), numSegments - 1);
    const segmentT = normalizedProgress - segmentIndex;

    const p1 = waypoints[segmentIndex];
    const p2 = waypoints[segmentIndex + 1];

    const currentX = p1.x + (p2.x - p1.x) * segmentT;
    const currentY = p1.y + (p2.y - p1.y) * segmentT;
    const angle = (Math.atan2(p2.y - p1.y, p2.x - p1.x) * 180) / Math.PI;

    return { x: currentX, y: currentY, angle };
  }, [state.ambulance.positionPercent, activeRoute, nodePositionMap, state.landmarks, state.hospital.id]);

  // Police cruiser position (patrols between node 1 and 3)
  const policeCoords = useMemo(() => {
    if (!state.policeEscort.active) return null;
    const p1 = nodePositionMap.get(2) || { x: 450, y: 280 };
    const p2 = nodePositionMap.get(3) || { x: 720, y: 280 };
    const t = 0.5 + 0.3 * Math.sin(Date.now() / 1500);
    return {
      x: p1.x + (p2.x - p1.x) * t,
      y: p1.y + (p2.y - p1.y) * t,
      angle: 0,
    };
  }, [state.policeEscort.active, nodePositionMap]);

  // Mouse drag handlers for panning
  const handleMouseDown = (e: React.MouseEvent) => {
    if ((e.target as HTMLElement).tagName === 'BUTTON') return;
    setIsDragging(true);
    setDragStart({ x: e.clientX - pan.x, y: e.clientY - pan.y });
  };

  const handleMouseMove = (e: React.MouseEvent) => {
    if (!isDragging) return;
    setPan({ x: e.clientX - dragStart.x, y: e.clientY - dragStart.y });
  };

  const handleMouseUp = () => setIsDragging(false);

  const resetView = () => {
    setZoom(1);
    setPan({ x: 0, y: 0 });
    setSelectedNode(null);
    setSelectedRoad(null);
  };

  // Road density color helper
  const getRoadColor = (road: RoadData) => {
    if (road.isBlocked || road.hasAccident || road.isBridgeClosed) return '#EF4444';
    if (road.isFlooded || road.isUnderConstruction) return '#F97316';
    switch (road.trafficDensity) {
      case 'GREEN':
        return '#10B981';
      case 'YELLOW':
        return '#F59E0B';
      case 'ORANGE':
        return '#F97316';
      case 'RED':
        return '#EF4444';
      default:
        return '#10B981';
    }
  };

  // Landmark icon helper
  const renderLandmarkIcon = (landmark: Landmark) => {
    switch (landmark.type) {
      case 'HOSPITAL':
        return (
          <g>
            <circle cx="0" cy="0" r="16" fill="#1E293B" stroke="#EF4444" strokeWidth="2" filter="url(#node-glow)" />
            <rect x="-8" y="-3" width="16" height="6" fill="#EF4444" rx="1" />
            <rect x="-3" y="-8" width="6" height="16" fill="#EF4444" rx="1" />
          </g>
        );
      case 'AMBULANCE_DEPOT':
        return (
          <g>
            <circle cx="0" cy="0" r="14" fill="#0F172A" stroke="#00F0FF" strokeWidth="2" />
            <circle cx="0" cy="0" r="4" fill="#00F0FF" />
          </g>
        );
      case 'POLICE_PRECINCT':
        return (
          <g>
            <circle cx="0" cy="0" r="13" fill="#0F172A" stroke="#3B82F6" strokeWidth="2" />
            <polygon points="0,-7 6,-3 6,5 0,8 -6,5 -6,-3" fill="#3B82F6" opacity="0.8" />
          </g>
        );
      case 'FIRE_STATION':
        return (
          <g>
            <circle cx="0" cy="0" r="13" fill="#0F172A" stroke="#F97316" strokeWidth="2" />
            <circle cx="0" cy="0" r="5" fill="#F97316" />
          </g>
        );
      case 'COORDINATOR':
        return (
          <g>
            <circle cx="0" cy="0" r="18" fill="#0B132B" stroke="#00F0FF" strokeWidth="2.5" filter="url(#node-glow)" />
            <circle cx="0" cy="0" r="28" fill="none" stroke="#00F0FF" strokeWidth="1" opacity="0.4" className="animate-ping" />
            <circle cx="0" cy="0" r="6" fill="#00F0FF" />
          </g>
        );
      default:
        return null;
    }
  };

  return (
    <div className="relative w-full h-full bg-[#070A10] rounded-xl border border-gray-800 overflow-hidden shadow-2xl select-none flex flex-col">
      {/* Top Map Action Toolbar */}
      <div className="absolute top-4 left-4 z-10 flex items-center space-x-2 bg-gray-900/90 backdrop-blur-md p-1.5 rounded-lg border border-gray-800 shadow-xl">
        <button
          onClick={() => setZoom((z) => Math.min(z + 0.15, 2.5))}
          className="p-1.5 text-gray-400 hover:text-cyan-400 hover:bg-gray-800 rounded transition"
          title="Zoom In"
        >
          <ZoomIn size={16} />
        </button>
        <button
          onClick={() => setZoom((z) => Math.max(z - 0.15, 0.6))}
          className="p-1.5 text-gray-400 hover:text-cyan-400 hover:bg-gray-800 rounded transition"
          title="Zoom Out"
        >
          <ZoomOut size={16} />
        </button>
        <div className="w-[1px] h-4 bg-gray-700" />
        <button
          onClick={resetView}
          className="p-1.5 text-gray-400 hover:text-cyan-400 hover:bg-gray-800 rounded transition"
          title="Reset View"
        >
          <RotateCcw size={16} />
        </button>
        <button
          onClick={() => setZoom(1.35)}
          className="p-1.5 text-gray-400 hover:text-cyan-400 hover:bg-gray-800 rounded transition"
          title="Focus Corridor"
        >
          <Maximize2 size={16} />
        </button>
      </div>

      {/* Top Right Active Route Selector Pill */}
      <div className="absolute top-4 right-4 z-10 flex items-center space-x-2 bg-gray-900/90 backdrop-blur-md px-3 py-1.5 rounded-lg border border-gray-800 shadow-xl">
        <span className="text-[11px] font-mono text-gray-400 flex items-center mr-1">
          <Navigation size={12} className="mr-1 text-cyan-400" /> CORRIDOR:
        </span>
        <button
          onClick={() => selectRoute('PRIMARY')}
          className={`px-2.5 py-1 text-xs font-mono font-bold rounded transition ${
            state.activeRouteId === 'PRIMARY'
              ? 'bg-cyan-500/20 text-cyan-400 border border-cyan-500/40 shadow-sm'
              : 'text-gray-400 hover:text-gray-200'
          }`}
        >
          PRIMARY ({state.routes.primaryRoute.etaSeconds}s)
        </button>
        <button
          onClick={() => selectRoute('ALT_A')}
          className={`px-2.5 py-1 text-xs font-mono font-bold rounded transition ${
            state.activeRouteId === 'ALT_A'
              ? 'bg-amber-500/20 text-amber-400 border border-amber-500/40 shadow-sm'
              : 'text-gray-400 hover:text-gray-200'
          }`}
        >
          ALT A ({state.routes.altRouteA.etaSeconds}s)
        </button>
        <button
          onClick={() => selectRoute('ALT_B')}
          className={`px-2.5 py-1 text-xs font-mono font-bold rounded transition ${
            state.activeRouteId === 'ALT_B'
              ? 'bg-purple-500/20 text-purple-400 border border-purple-500/40 shadow-sm'
              : 'text-gray-400 hover:text-gray-200'
          }`}
        >
          ALT B ({state.routes.altRouteB.etaSeconds}s)
        </button>
      </div>

      {/* Interactive SVG Canvas */}
      <div
        className="w-full h-full cursor-grab active:cursor-grabbing relative overflow-hidden"
        onMouseDown={handleMouseDown}
        onMouseMove={handleMouseMove}
        onMouseUp={handleMouseUp}
        onMouseLeave={handleMouseUp}
      >
        <svg
          viewBox="0 0 1150 640"
          className="w-full h-full transition-transform duration-75"
          style={{
            transform: `translate(${pan.x}px, ${pan.y}px) scale(${zoom})`,
            transformOrigin: 'center center',
          }}
        >
          <defs>
            {/* Cyber City Blueprint Grid */}
            <pattern id="cyber-grid-dense" width="30" height="30" patternUnits="userSpaceOnUse">
              <path d="M 30 0 L 0 0 0 30" fill="none" stroke="rgba(30, 41, 59, 0.35)" strokeWidth="0.8" />
            </pattern>
            <pattern id="cyber-grid-major" width="150" height="150" patternUnits="userSpaceOnUse">
              <rect width="150" height="150" fill="url(#cyber-grid-dense)" />
              <path d="M 150 0 L 0 0 0 150" fill="none" stroke="rgba(56, 189, 248, 0.08)" strokeWidth="1.5" />
            </pattern>

            {/* Glowing Corridor Green Wave Filter */}
            <filter id="corridor-glow" x="-30%" y="-30%" width="160%" height="160%">
              <feGaussianBlur stdDeviation="6" result="blur" />
              <feMerge>
                <feMergeNode in="blur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>

            {/* Emergency Siren Glow Filter */}
            <filter id="siren-glow" x="-60%" y="-60%" width="220%" height="220%">
              <feGaussianBlur stdDeviation="8" result="blur" />
              <feMerge>
                <feMergeNode in="blur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>

            {/* High-tech Node Glow */}
            <filter id="node-glow" x="-40%" y="-40%" width="180%" height="180%">
              <feGaussianBlur stdDeviation="4" result="blur" />
              <feMerge>
                <feMergeNode in="blur" />
                <feMergeNode in="SourceGraphic" />
              </feMerge>
            </filter>

            {/* Road Pulse Animation Dash */}
            <linearGradient id="active-corridor-grad" x1="0%" y1="0%" x2="100%" y2="0%">
              <stop offset="0%" stopColor="#10B981" stopOpacity="0.8" />
              <stop offset="50%" stopColor="#00F0FF" stopOpacity="1" />
              <stop offset="100%" stopColor="#10B981" stopOpacity="0.8" />
            </linearGradient>
          </defs>

          {/* Background Grid Layer */}
          <rect width="1150" height="640" fill="#080C16" />
          <rect width="1150" height="640" fill="url(#cyber-grid-major)" />

          {/* City District Blocks & Zoning */}
          <g opacity="0.6">
            <rect x="130" y="30" width="130" height="55" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="195" y="60" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Central IT Park</text>

            <rect x="360" y="30" width="150" height="55" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="435" y="60" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Financial Tech Center</text>

            <rect x="800" y="30" width="130" height="55" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="865" y="60" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Diplomatic Enclave</text>

            <rect x="130" y="350" width="180" height="80" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="220" y="395" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Victoria Medical Belt</text>

            <rect x="520" y="350" width="160" height="80" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="600" y="395" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Koramangala Commercial</text>

            <rect x="800" y="350" width="140" height="80" rx="6" fill="#0C1527" stroke="#1E293B" strokeWidth="1" />
            <text x="870" y="395" textAnchor="middle" fill="#64748B" fontSize="9" fontFamily="sans-serif">Old Airport Tech Hub</text>
          </g>

          {/* 1. ALL 22 ROADS */}
          <g id="roads-layer">
            {state.roads.map((road) => {
              const fromPos = nodePositionMap.get(road.fromNodeId);
              const toPos = nodePositionMap.get(road.toNodeId);
              if (!fromPos || !toPos) return null;

              const isCorridorActiveRoad = activeRoute.roadIds.includes(road.id) && state.isRunning;
              const isSelected = selectedRoad?.id === road.id;
              const roadColor = getRoadColor(road);

              const midX = (fromPos.x + toPos.x) / 2;
              const midY = (fromPos.y + toPos.y) / 2;

              return (
                <g
                  key={road.id}
                  className="cursor-pointer group"
                  onClick={() => {
                    setSelectedRoad(road);
                    setSelectedNode(null);
                  }}
                >
                  {/* Asphalt Underlay */}
                  <line
                    x1={fromPos.x}
                    y1={fromPos.y}
                    x2={toPos.x}
                    y2={toPos.y}
                    stroke="#1E293B"
                    strokeWidth={isSelected ? 22 : 18}
                    strokeLinecap="round"
                  />

                  {/* Density Status Color Line */}
                  <line
                    x1={fromPos.x}
                    y1={fromPos.y}
                    x2={toPos.x}
                    y2={toPos.y}
                    stroke={roadColor}
                    strokeWidth={isSelected ? 10 : 8}
                    strokeOpacity={0.65}
                    strokeLinecap="round"
                  />

                  {/* Pulsing Green Preemption Aura for Active Corridor */}
                  {isCorridorActiveRoad && (
                    <line
                      x1={fromPos.x}
                      y1={fromPos.y}
                      x2={toPos.x}
                      y2={toPos.y}
                      stroke="url(#active-corridor-grad)"
                      strokeWidth={14}
                      strokeLinecap="round"
                      filter="url(#corridor-glow)"
                      strokeOpacity={0.9}
                    />
                  )}

                  {/* Centerline Dashes */}
                  <line
                    x1={fromPos.x}
                    y1={fromPos.y}
                    x2={toPos.x}
                    y2={toPos.y}
                    stroke={isCorridorActiveRoad ? '#FFFFFF' : '#64748B'}
                    strokeWidth={1.5}
                    strokeDasharray={isCorridorActiveRoad ? '10,8' : '6,6'}
                  />

                  {/* Road Condition / Incident Icon Marker at midpoint */}
                  {(road.isBlocked || road.hasAccident || road.isFlooded || road.isUnderConstruction || road.isBridgeClosed) && (
                    <g transform={`translate(${midX}, ${midY})`}>
                      <circle cx="0" cy="0" r="10" fill="#EF4444" filter="url(#node-glow)" />
                      {road.hasAccident && <text x="0" y="4" textAnchor="middle" fill="#FFFFFF" fontSize="10">💥</text>}
                      {road.isFlooded && <text x="0" y="4" textAnchor="middle" fill="#FFFFFF" fontSize="10">🌊</text>}
                      {(road.isBlocked || road.isUnderConstruction) && <text x="0" y="4" textAnchor="middle" fill="#FFFFFF" fontSize="10">🚧</text>}
                      {road.isBridgeClosed && <text x="0" y="4" textAnchor="middle" fill="#FFFFFF" fontSize="10">🌉</text>}
                    </g>
                  )}
                </g>
              );
            })}
          </g>

          {/* 2. LANDMARKS LAYER */}
          <g id="landmarks-layer">
            {state.landmarks.map((landmark) => (
              <g
                key={landmark.id}
                transform={`translate(${landmark.x}, ${landmark.y})`}
                className="cursor-pointer"
              >
                {renderLandmarkIcon(landmark)}
                <text
                  x="0"
                  y="26"
                  textAnchor="middle"
                  fill="#94A3B8"
                  fontSize="8.5"
                  fontWeight="bold"
                  fontFamily="sans-serif"
                >
                  {landmark.name.split(' (')[0]}
                </text>
              </g>
            ))}
          </g>

          {/* 3. ALL 12 SMART INTERSECTIONS */}
          <g id="intersections-layer">
            {state.intersections.map((node) => {
              const isSelected = selectedNode?.id === node.id;
              const isOffline = node.health === 'OFFLINE';
              const isNodeInCorridor = activeRoute.nodeIds.includes(node.id) && state.isRunning;

              return (
                <g
                  key={node.id}
                  transform={`translate(${node.x}, ${node.y})`}
                  className="cursor-pointer"
                  onClick={(e) => {
                    e.stopPropagation();
                    setSelectedNode(node);
                    setSelectedRoad(null);
                  }}
                >
                  {/* Outer Radar / Preemption Ring */}
                  {isOffline ? (
                    <circle
                      cx="0"
                      cy="0"
                      r="26"
                      fill="#EF4444"
                      fillOpacity="0.2"
                      stroke="#EF4444"
                      strokeWidth="2"
                      strokeDasharray="4,4"
                      className="animate-pulse"
                    />
                  ) : isNodeInCorridor ? (
                    <circle
                      cx="0"
                      cy="0"
                      r="28"
                      fill="#10B981"
                      fillOpacity="0.15"
                      stroke="#10B981"
                      strokeWidth="2.5"
                      filter="url(#corridor-glow)"
                    />
                  ) : null}

                  {/* Junction Base Box */}
                  <circle
                    cx="0"
                    cy="0"
                    r={isSelected ? 20 : 17}
                    fill="#0F172A"
                    stroke={
                      isOffline
                        ? '#EF4444'
                        : isNodeInCorridor
                        ? '#00F0FF'
                        : isSelected
                        ? '#38BDF8'
                        : '#334155'
                    }
                    strokeWidth={isSelected ? 2.5 : 1.5}
                  />

                  {/* Node ID Badge in center */}
                  <text
                    x="0"
                    y="4"
                    textAnchor="middle"
                    fill={isOffline ? '#EF4444' : isNodeInCorridor ? '#00F0FF' : '#E2E8F0'}
                    fontSize="10"
                    fontWeight="bold"
                    fontFamily="monospace"
                  >
                    #{node.id}
                  </text>

                  {/* Traffic Signal Mini-Stack (Top Right) */}
                  <g transform="translate(18, -24)">
                    <rect x="0" y="0" width="12" height="26" rx="3" fill="#020617" stroke="#334155" strokeWidth="1" />
                    <circle
                      cx="6"
                      cy="5"
                      r="3"
                      fill={node.signalColor === 'RED' ? '#EF4444' : '#450A0A'}
                    />
                    <circle
                      cx="6"
                      cy="13"
                      r="3"
                      fill={node.signalColor === 'YELLOW' ? '#F59E0B' : '#451A03'}
                    />
                    <circle
                      cx="6"
                      cy="21"
                      r="3"
                      fill={node.signalColor === 'GREEN' ? '#10B981' : '#022C22'}
                    />
                  </g>

                  {/* Real Raspberry Pi Hardware LED Box (Nodes 1, 2, 3) */}
                  {node.id <= 3 && (
                    <g transform="translate(-32, -24)">
                      <rect x="0" y="0" width="14" height="26" rx="3" fill="#020617" stroke="#475569" strokeWidth="1" />
                      {/* Red LED (Nodes 1 & 2 have it; Node 3 is virtualized due to hardware shortage) */}
                      <circle
                        cx="7"
                        cy="6"
                        r="3.5"
                        fill={
                          node.hasRedHardware
                            ? node.redLed
                              ? '#EF4444'
                              : '#311010'
                            : '#1E293B' // Hardware shortage indicator
                        }
                        filter={node.redLed ? 'url(#siren-glow)' : undefined}
                      />
                      {/* Blue Siren LED (BCM GPIO6, Pin 31 on all 3 Pis) */}
                      <circle
                        cx="7"
                        cy="18"
                        r="3.5"
                        fill={node.blueLed ? '#00F0FF' : '#083344'}
                        filter={node.blueLed ? 'url(#siren-glow)' : undefined}
                      />
                    </g>
                  )}

                  {/* AI Camera Marker Icon */}
                  <g transform="translate(14, 10)">
                    <circle cx="6" cy="6" r="6" fill="#0F172A" stroke="#00F0FF" strokeWidth="1" />
                    <circle cx="6" cy="6" r="2.5" fill={node.camera?.health === 'ONLINE' ? '#00F0FF' : '#EF4444'} />
                  </g>

                  {/* Node Name & Telemetry under node */}
                  <text
                    x="0"
                    y="32"
                    textAnchor="middle"
                    fill={isOffline ? '#EF4444' : '#94A3B8'}
                    fontSize="8"
                    fontWeight="bold"
                    fontFamily="sans-serif"
                  >
                    {node.name.length > 18 ? node.name.substring(0, 16) + '..' : node.name}
                  </text>
                  <text
                    x="0"
                    y="42"
                    textAnchor="middle"
                    fill="#64748B"
                    fontSize="7"
                    fontFamily="monospace"
                  >
                    {isOffline ? 'OFFLINE (FAILED)' : `${node.cpuPercent}% CPU | ${node.queueLength} veh`}
                  </text>

                  {/* Emergency Preemption Banner */}
                  {node.preempted && (
                    <g transform="translate(-45, -42)">
                      <rect x="0" y="0" width="90" height="14" rx="2" fill="#00F0FF" />
                      <text x="45" y="10" textAnchor="middle" fill="#020617" fontSize="7.5" fontWeight="bold">
                        GREEN CORRIDOR
                      </text>
                    </g>
                  )}
                </g>
              );
            })}
          </g>

          {/* 4. POLICE ESCORT CRUISER (If Active) */}
          {policeCoords && (
            <g transform={`translate(${policeCoords.x}, ${policeCoords.y})`}>
              <circle cx="0" cy="0" r="16" fill="none" stroke="#3B82F6" strokeWidth="1" className="animate-ping" opacity="0.6" />
              <rect x="-14" y="-8" width="28" height="16" rx="3" fill="#1E3A8A" stroke="#60A5FA" strokeWidth="1.5" />
              {/* Flashing Police Light Bar */}
              <rect x="-6" y="-3" width="5" height="6" fill="#EF4444" className="animate-pulse" />
              <rect x="1" y="-3" width="5" height="6" fill="#3B82F6" className="animate-pulse" />
              <text x="0" y="-12" textAnchor="middle" fill="#93C5FD" fontSize="7" fontWeight="bold">
                PATROL-01
              </text>
            </g>
          )}

          {/* 5. AMBULANCE AMB-101 (Smoothly Animated Position) */}
          {state.ambulance.isActive && (
            <g transform={`translate(${ambulanceCoords.x}, ${ambulanceCoords.y}) rotate(${ambulanceCoords.angle})`}>
              {/* Outer Siren Pulse Wave */}
              <circle cx="0" cy="0" r="24" fill="none" stroke="#00F0FF" strokeWidth="2" className="animate-ping" opacity="0.7" />

              {/* Vehicle Body Chassis */}
              <rect x="-18" y="-10" width="36" height="20" rx="4" fill="#FFFFFF" stroke="#EF4444" strokeWidth="2" />
              {/* Red Medical Cross */}
              <rect x="-6" y="-2.5" width="12" height="5" fill="#EF4444" />
              <rect x="-2.5" y="-6" width="5" height="12" fill="#EF4444" />
              {/* Blue Roof Siren Dome */}
              <circle cx="3" cy="0" r="4" fill="#00F0FF" filter="url(#siren-glow)" className="animate-pulse" />

              {/* Floating Ambulance Telemetry Tag */}
              <g transform={`rotate(${-ambulanceCoords.angle}) translate(0, -20)`}>
                <rect x="-42" y="-12" width="84" height="15" rx="3" fill="#090E17" stroke="#00F0FF" strokeWidth="1" />
                <text x="0" y="-2" textAnchor="middle" fill="#00F0FF" fontSize="8" fontWeight="bold">
                  AMB-101 ({state.ambulance.speedKmh} km/h)
                </text>
              </g>
            </g>
          )}
        </svg>
      </div>

      {/* Floating Modal: Intersection Node Inspector */}
      {selectedNode && (
        <div className="absolute top-16 right-4 w-80 bg-gray-950/95 backdrop-blur-xl border border-gray-800 rounded-xl p-4 shadow-2xl z-20 animate-fade-in text-xs">
          <div className="flex items-center justify-between pb-3 border-b border-gray-800">
            <div className="flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse" />
              <h4 className="font-bold text-gray-200">Node #{selectedNode.id}: {selectedNode.name}</h4>
            </div>
            <button
              onClick={() => setSelectedNode(null)}
              className="text-gray-400 hover:text-gray-200 p-1 rounded"
            >
              <X size={14} />
            </button>
          </div>

          <div className="mt-3 space-y-2.5">
            <div className="flex justify-between items-center bg-gray-900/60 p-2 rounded">
              <span className="text-gray-400">Network Host:</span>
              <span className="font-mono text-cyan-400">{selectedNode.ip}:{selectedNode.port}</span>
            </div>

            <div className="flex justify-between items-center bg-gray-900/60 p-2 rounded">
              <span className="text-gray-400">Node Health:</span>
              <span className={`font-mono font-bold ${selectedNode.health === 'ONLINE' ? 'text-emerald-400' : 'text-rose-400'}`}>
                {selectedNode.health}
              </span>
            </div>

            <div className="grid grid-cols-2 gap-2">
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">CPU UTIL</span>
                <span className="font-mono font-bold text-gray-200">{selectedNode.cpuPercent}%</span>
              </div>
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">CORE TEMP</span>
                <span className="font-mono font-bold text-amber-400">{selectedNode.temperatureC}°C</span>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-2">
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">NETWORK RTT</span>
                <span className="font-mono font-bold text-cyan-400">{selectedNode.latencyMs} ms</span>
              </div>
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">QUEUE LENGTH</span>
                <span className="font-mono font-bold text-gray-200">{selectedNode.queueLength} vehicles</span>
              </div>
            </div>

            {/* Hardware LEDs details */}
            <div className="bg-gray-900/60 p-2.5 rounded border border-gray-800">
              <span className="text-gray-400 text-[10px] font-bold block mb-1.5 uppercase">Hardware Output Pins</span>
              <div className="flex items-center justify-between">
                <span className="text-gray-300">Red LED (BCM 5 Pin 29):</span>
                <span className={`font-mono font-bold ${selectedNode.hasRedHardware ? (selectedNode.redLed ? 'text-rose-400' : 'text-gray-500') : 'text-amber-500'}`}>
                  {selectedNode.hasRedHardware ? (selectedNode.redLed ? 'ON' : 'OFF') : 'VIRTUALIZED'}
                </span>
              </div>
              <div className="flex items-center justify-between mt-1">
                <span className="text-gray-300">Blue Siren (BCM 6 Pin 31):</span>
                <span className={`font-mono font-bold ${selectedNode.blueLed ? 'text-cyan-400' : 'text-gray-500'}`}>
                  {selectedNode.blueLed ? 'FLASHING' : 'OFF'}
                </span>
              </div>
            </div>

            {/* Camera Feed Telemetry */}
            {selectedNode.camera && (
              <div className="bg-gray-900/60 p-2.5 rounded border border-gray-800">
                <div className="flex items-center justify-between mb-1">
                  <span className="text-gray-400 text-[10px] font-bold flex items-center">
                    <Video size={10} className="mr-1 text-cyan-400" /> AI PERCEPTION: {selectedNode.camera.id}
                  </span>
                  <span className="text-[10px] text-emerald-400 font-mono">{selectedNode.camera.fps} FPS</span>
                </div>
                <div className="text-gray-300 text-[11px] flex justify-between">
                  <span>Vehicles Detected:</span>
                  <span className="font-bold text-cyan-400">{selectedNode.camera.vehicleCount}</span>
                </div>
                <div className="text-gray-300 text-[11px] flex justify-between mt-0.5">
                  <span>Lane Occupancy:</span>
                  <span className="font-bold text-amber-400">{selectedNode.camera.laneOccupancyPercent}%</span>
                </div>
              </div>
            )}

            {/* Quick Fault Injection Action */}
            <button
              onClick={() => toggleNodeFailure(selectedNode.id)}
              className={`w-full py-2 rounded text-xs font-mono font-bold transition ${
                selectedNode.health === 'ONLINE'
                  ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40 hover:bg-rose-500/30'
                  : 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/40 hover:bg-emerald-500/30'
              }`}
            >
              {selectedNode.health === 'ONLINE' ? 'SIMULATE NODE HARDWARE FAILURE' : 'RESTORE NODE POWER'}
            </button>
          </div>
        </div>
      )}

      {/* Floating Modal: Road Segment Inspector */}
      {selectedRoad && (
        <div className="absolute top-16 right-4 w-80 bg-gray-950/95 backdrop-blur-xl border border-gray-800 rounded-xl p-4 shadow-2xl z-20 animate-fade-in text-xs">
          <div className="flex items-center justify-between pb-3 border-b border-gray-800">
            <div className="flex items-center space-x-2">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400" />
              <h4 className="font-bold text-gray-200">{selectedRoad.name}</h4>
            </div>
            <button
              onClick={() => setSelectedRoad(null)}
              className="text-gray-400 hover:text-gray-200 p-1 rounded"
            >
              <X size={14} />
            </button>
          </div>

          <div className="mt-3 space-y-2.5">
            <div className="flex justify-between items-center bg-gray-900/60 p-2 rounded">
              <span className="text-gray-400">Road Corridor ID:</span>
              <span className="font-mono text-cyan-400">{selectedRoad.id} (N{selectedRoad.fromNodeId} ↔ N{selectedRoad.toNodeId})</span>
            </div>

            <div className="grid grid-cols-2 gap-2">
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">CORRIDOR LENGTH</span>
                <span className="font-mono font-bold text-gray-200">{selectedRoad.lengthKm} km</span>
              </div>
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">SPEED LIMIT</span>
                <span className="font-mono font-bold text-cyan-400">{selectedRoad.speedLimitKmh} km/h</span>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-2">
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">CONGESTION LEVEL</span>
                <span className="font-mono font-bold text-amber-400">{selectedRoad.congestionPercent}% ({selectedRoad.trafficDensity})</span>
              </div>
              <div className="bg-gray-900/60 p-2 rounded">
                <span className="text-gray-400 text-[10px] block">TRAVEL TIME</span>
                <span className="font-mono font-bold text-emerald-400">{selectedRoad.travelTimeSeconds}s</span>
              </div>
            </div>

            <div className="bg-gray-900/60 p-2.5 rounded border border-gray-800">
              <div className="flex justify-between text-gray-300">
                <span>IRSA Suitability Score:</span>
                <span className="font-mono font-bold text-cyan-400">{selectedRoad.suitabilityScore}/100</span>
              </div>
              <div className="flex justify-between text-gray-300 mt-1">
                <span>Prediction Confidence:</span>
                <span className="font-mono font-bold text-purple-400">{selectedRoad.confidenceScore}%</span>
              </div>
              <div className="flex justify-between text-gray-300 mt-1">
                <span>Road Surface Condition:</span>
                <span className="font-mono font-bold text-emerald-400">{selectedRoad.roadCondition}</span>
              </div>
            </div>

            {/* Quick Road Fault Injections */}
            <div className="grid grid-cols-3 gap-1.5 pt-1">
              <button
                onClick={() => toggleRoadBlocked(selectedRoad.id)}
                className={`p-1.5 rounded text-[10px] font-mono font-bold transition text-center ${
                  selectedRoad.isBlocked
                    ? 'bg-rose-500/20 text-rose-400 border border-rose-500/40'
                    : 'bg-gray-800 text-gray-300 hover:bg-gray-700'
                }`}
              >
                {selectedRoad.isBlocked ? 'UNBLOCK' : 'BLOCK'}
              </button>
              <button
                onClick={() => toggleAccident(selectedRoad.id)}
                className={`p-1.5 rounded text-[10px] font-mono font-bold transition text-center ${
                  selectedRoad.hasAccident
                    ? 'bg-amber-500/20 text-amber-400 border border-amber-500/40'
                    : 'bg-gray-800 text-gray-300 hover:bg-gray-700'
                }`}
              >
                {selectedRoad.hasAccident ? 'CLEAR CRASH' : 'CRASH'}
              </button>
              <button
                onClick={() => toggleFlood(selectedRoad.id)}
                className={`p-1.5 rounded text-[10px] font-mono font-bold transition text-center ${
                  selectedRoad.isFlooded
                    ? 'bg-blue-500/20 text-blue-400 border border-blue-500/40'
                    : 'bg-gray-800 text-gray-300 hover:bg-gray-700'
                }`}
              >
                {selectedRoad.isFlooded ? 'DRAIN' : 'FLOOD'}
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Bottom Map Status Strip */}
      <div className="absolute bottom-3 left-4 right-4 z-10 bg-gray-950/85 backdrop-blur-md px-4 py-2.5 rounded-lg border border-gray-800 flex items-center justify-between text-xs font-mono shadow-xl">
        <div className="flex items-center space-x-6">
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Corridor Route:</span>
            <span className="text-cyan-400 font-bold">{activeRoute.name}</span>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Current Position:</span>
            <span className="text-gray-200 font-semibold">{state.ambulance.currentRoad}</span>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Vehicle Speed:</span>
            <span className="text-emerald-400 font-bold">{state.ambulance.speedKmh} km/h</span>
          </div>
        </div>

        <div className="flex items-center space-x-5">
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Target ETA:</span>
            <span className="text-amber-400 font-bold">{state.ambulance.etaSeconds}s</span>
          </div>
          <span className="text-gray-700">|</span>
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Preemption State:</span>
            <span className={state.isRunning ? 'text-cyan-400 font-bold' : 'text-gray-500'}>
              {state.isRunning ? 'DYNAMIC PREEMPTION ACTIVE' : 'CYCLE STANDBY'}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
};
