import React, { useEffect, useMemo, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { RigView } from './RigView.jsx';
import { TimingGraph } from './TimingGraph.jsx';
import { connectTelemetry } from './wsClient.js';
import './styles.css';

const demoSamples = [
  { actuator: 'dome_rotation', scheduled_ms: 0, jitter_ms: 0.4 },
  { actuator: 'front_logic_lights', scheduled_ms: 0, jitter_ms: 0.7 },
  { actuator: 'primary_speaker', scheduled_ms: 50, jitter_ms: 1.1 },
  { actuator: 'front_logic_lights', scheduled_ms: 800, jitter_ms: 0.9 }
];

function App() {
  const [snapshot, setSnapshot] = useState({
    events_dispatched: 4,
    deadline_misses: 0,
    mean_jitter_ms: 0.775,
    p99_jitter_ms: 1.1,
    max_jitter_ms: 1.1,
    samples: demoSamples,
    states: [
      { actuator: 'dome_rotation', type: 'servo', action: 'move_to', params: { angle_deg: '90' } },
      { actuator: 'front_logic_lights', type: 'light', action: 'off', params: { state: 'off' } },
      { actuator: 'primary_speaker', type: 'audio', action: 'play', params: { file: 'beep_03.wav' } }
    ]
  });
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const client = connectTelemetry({
      url: 'ws://localhost:8765',
      onOpen: () => setConnected(true),
      onClose: () => setConnected(false),
      onSnapshot: setSnapshot
    });
    return () => client.close();
  }, []);

  const stats = useMemo(() => [
    ['Events', snapshot.events_dispatched],
    ['Misses', snapshot.deadline_misses],
    ['Mean jitter', `${Number(snapshot.mean_jitter_ms).toFixed(2)} ms`],
    ['P99 jitter', `${Number(snapshot.p99_jitter_ms).toFixed(2)} ms`]
  ], [snapshot]);

  return (
    <main className="app-shell">
      <header className="topbar">
        <div>
          <p className="eyebrow">Astromech Control Engine</p>
          <h1>ACE Droid Console</h1>
        </div>
        <div className={connected ? 'status online' : 'status'}>{connected ? 'Live' : 'Sim'}</div>
      </header>

      <section className="stat-grid">
        {stats.map(([label, value]) => (
          <div className="stat" key={label}>
            <span>{label}</span>
            <strong>{value}</strong>
          </div>
        ))}
      </section>

      <section className="workspace">
        <RigView states={snapshot.states ?? []} />
        <TimingGraph samples={snapshot.samples ?? []} />
      </section>
    </main>
  );
}

createRoot(document.getElementById('root')).render(<App />);
