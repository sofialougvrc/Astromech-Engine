import React from 'react';

function findState(states, actuator) {
  return states.find((state) => state.actuator === actuator) ?? {};
}

export function RigView({ states }) {
  const dome = findState(states, 'dome_rotation');
  const logic = findState(states, 'front_logic_lights');
  const speaker = findState(states, 'primary_speaker');
  const domeAngle = Number(dome.params?.angle_deg ?? 18);
  const lightsOn = logic.action === 'set' && logic.params?.state !== 'off';

  return (
    <section className="panel rig-panel" aria-label="R2 rig state">
      <div className="panel-title">
        <span>Rig</span>
        <strong>{speaker.params?.file ?? 'silent'}</strong>
      </div>
      <div className="droid">
        <div className="dome" style={{ transform: `rotate(${domeAngle / 8}deg)` }}>
          <span className="eye" />
          <span className={lightsOn ? 'logic on' : 'logic'} />
        </div>
        <div className="body">
          <span className="panel-slot left" />
          <span className="panel-slot right" />
          <span className="center-light" />
        </div>
        <div className="legs">
          <span />
          <span />
        </div>
      </div>
      <div className="state-list">
        {states.map((state) => (
          <div className="state-row" key={state.actuator}>
            <span>{state.actuator}</span>
            <strong>{state.action}</strong>
          </div>
        ))}
      </div>
    </section>
  );
}
