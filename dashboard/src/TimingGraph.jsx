import React, { useMemo } from 'react';
import * as d3 from 'd3';

export function TimingGraph({ samples }) {
  const bars = useMemo(() => {
    const width = 520;
    const height = 260;
    const jitterExtent = d3.extent(samples, (sample) => Math.abs(sample.jitter_ms));
    const x = d3.scaleBand()
      .domain(samples.map((sample, index) => `${sample.actuator}-${index}`))
      .range([0, width])
      .padding(0.28);
    const y = d3.scaleLinear()
      .domain([0, Math.max(1, jitterExtent[1] ?? 1)])
      .nice()
      .range([height, 0]);

    return { width, height, x, y };
  }, [samples]);

  return (
    <section className="panel timing-panel" aria-label="Timing telemetry">
      <div className="panel-title">
        <span>Timing</span>
        <strong>{samples.length} samples</strong>
      </div>
      <svg className="chart" viewBox={`0 0 ${bars.width} ${bars.height}`} role="img">
        {samples.map((sample, index) => {
          const key = `${sample.actuator}-${index}`;
          const value = Math.abs(sample.jitter_ms);
          const x = bars.x(key);
          const y = bars.y(value);
          return (
            <g key={key}>
              <rect
                x={x}
                y={y}
                width={bars.x.bandwidth()}
                height={bars.height - y}
                rx="3"
              />
              <text x={x + bars.x.bandwidth() / 2} y={bars.height - 8}>
                {sample.scheduled_ms}
              </text>
            </g>
          );
        })}
      </svg>
      <div className="sample-list">
        {samples.slice(-5).map((sample, index) => (
          <div className="sample-row" key={`${sample.actuator}-${index}`}>
            <span>{sample.actuator}</span>
            <strong>{Number(sample.jitter_ms).toFixed(3)} ms</strong>
          </div>
        ))}
      </div>
    </section>
  );
}
