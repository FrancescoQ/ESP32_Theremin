import { useState } from "preact/hooks";
import { ToggleSwitch } from "./ToggleSwitch";
import { CommandSlider } from "./CommandSlider";

export function Effect({ effectName }) {
  // Effect-specific parameter configurations
  const effectConfigs = {
    delay: {
      displayName: "Delay",
      parameters: [
        {
          name: "time",
          label: "Delay Time",
          min: 10,
          max: 2000,
          step: 10,
          unit: "ms",
          defaultValue: 300
        },
        {
          name: "feedback",
          label: "Feedback",
          min: 0,
          max: 0.95,
          step: 0.01,
          unit: "",
          defaultValue: 0.5
        },
        {
          name: "mix",
          label: "Mix",
          min: 0,
          max: 1,
          step: 0.01,
          unit: "",
          defaultValue: 0.3
        }
      ]
    },
    chorus: {
      displayName: "Chorus",
      parameters: [
        {
          name: "rate",
          label: "Rate",
          min: 0.1,
          max: 10,
          step: 0.1,
          unit: "Hz",
          defaultValue: 2.0
        },
        {
          name: "depth",
          label: "Depth",
          min: 1,
          max: 50,
          step: 1,
          unit: "ms",
          defaultValue: 15
        },
        {
          name: "mix",
          label: "Mix",
          min: 0,
          max: 1,
          step: 0.01,
          unit: "",
          defaultValue: 0.5
        }
      ]
    },
    reverb: {
      displayName: "Reverb",
      parameters: [
        {
          name: "roomSize",
          label: "Room Size",
          min: 0,
          max: 1,
          step: 0.01,
          unit: "",
          defaultValue: 0.5
        },
        {
          name: "damping",
          label: "Damping",
          min: 0,
          max: 1,
          step: 0.01,
          unit: "",
          defaultValue: 0.5
        },
        {
          name: "mix",
          label: "Mix",
          min: 0,
          max: 1,
          step: 0.01,
          unit: "",
          defaultValue: 0.3
        }
      ]
    }
  };

  const config = effectConfigs[effectName];

  // Initialize state for each parameter
  const [paramValues, setParamValues] = useState(
    config.parameters.reduce((acc, param) => {
      acc[param.name] = param.defaultValue;
      return acc;
    }, {})
  );

  const updateParamValue = (paramName, value) => {
    setParamValues(prev => ({ ...prev, [paramName]: value }));
  };

  return (
    <div class="space-y-4">
      <ToggleSwitch
        label="Enable"
        onCommand={{
          cmd: "enableEffect",
          effect: effectName,
          value: true
        }}
        offCommand={{
          cmd: "enableEffect",
          effect: effectName,
          value: false
        }}
      />

      {config.parameters.map(param => (
        <CommandSlider
          key={param.name}
          label={param.label}
          value={paramValues[param.name]}
          onChange={(value) => updateParamValue(param.name, value)}
          min={param.min}
          max={param.max}
          step={param.step}
          unit={param.unit}
          commandGenerator={(value) => ({
            cmd: "setEffectParam",
            effect: effectName,
            param: param.name,
            value: value
          })}
        />
      ))}
    </div>
  );
}
