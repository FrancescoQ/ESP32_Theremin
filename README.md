# ESP32 Theremin

A gesture-controlled digital musical instrument built with ESP32 and Time-of-Flight sensors, creating theremin-like sounds through hand movements.

## Overview

This project transforms an ESP32 microcontroller into a playable theremin using laser distance sensors. Move your hands over two sensors to control pitch and volume independently, creating eerie, expressive electronic sounds in real-time.

**Key Features:**
- Dual sensor control: one hand for pitch, one for volume
- Real-time audio synthesis with minimal latency
- I2C sensor management with address conflict resolution
- Clean, modular architecture for future expansion

## Software

Built with PlatformIO and Arduino framework through an **AI-assisted development workflow**:

**Planning Phase (Claude AI):**
- Initial requirements definition and architecture design
- Technical specifications and problem-solving approach
- Comprehensive project brief and documentation structure

**Implementation Phase (Cline AI + Human Collaboration):**
- Code generation and implementation guided by Cline AI
- Active human review, refinement, and custom coding
- Hardware testing and real-world validation
- Iterative debugging and optimization

**Core Features:**
- Sensor input smoothing for stable control
- Real audio synthesis using ESP32 internal DAC (early iterations used basic PWM and Wokwi simulation for proof-of-concept)
- **Audio effects system** - Delay, Chorus, AND Reverb effects with excellent performance (14.5% CPU, 85% headroom!)
- **Multi-oscillator support** - 3 oscillators with 4 waveform types (Sine, Square, Triangle, Sawtooth)
- Modular component design (SensorManager, AudioEngine, EffectsChain, Theremin controller)
- OTA update capability
- Comprehensive debug logging

**Note on Simulation:** Wokwi simulator was useful for initial validation but has limited utility beyond basic proof-of-concept (Step 0).

*This was a true collaborative process - AI tools accelerated development and provided technical assistance, but all code was reviewed, tested, and refined through active human involvement and hardware validation.*

## Development Process

This project was developed through a **collaborative AI-assisted workflow**:

1. **Initial Planning**: Extensive discussion with Claude AI to define requirements, architecture, and development approach, resulting in the comprehensive project brief
2. **Iterative Refinement**: Multiple iterations with Claude to refine specifications, explore technical solutions, and plan implementation phases
3. **Code Implementation**: Cline AI agent used to write, test, and refine the actual codebase based on the Claude-generated brief
4. **Continuous Collaboration**: True human-AI partnership with the developer actively involved in code review, hardware testing, debugging, and implementation decisions - AI accelerated development while human expertise guided technical choices and validated real-world functionality

This approach demonstrates how modern AI tools can accelerate embedded systems development while maintaining human oversight and creative direction.

## Current Status

**🎉 Phase 4 COMPLETE - Three-Effect Audio Engine! 🎉**
- ✓ Dual sensor control working
- ✓ Real-time audio synthesis with I2S DAC
- ✓ **3 oscillators** with 4 waveform types
- ✓ **Delay, Chorus, AND Reverb effects** (14.5% CPU, 85% headroom!)
- ✓ Serial command control for all effects
- ✓ Stable, responsive operation
- ✓ OTA update support (with optional button activation)

**Next:** Phase 3 hardware (controls + display) when parts arrive, or optional Phase G quality polish

See `productbrief.md` for v2.0 roadmap and `memory-bank/progress.md` for detailed status.

## Documentation

Comprehensive project documentation is maintained in the `memory-bank/` directory:
- **projectbrief.md**: Complete project requirements and vision
- **ARCHITECTURE.md**: System design and component details
- **OTA_SETUP.md**: Over-the-air update configuration
- **DEBUG_GUIDE.md**: Debugging and troubleshooting

## License

Open source project for educational and experimental use.

---

*Built with ESP32, powered by curiosity, developed with AI collaboration.*
