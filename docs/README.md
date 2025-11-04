# ESP32 Theremin Documentation

> **Quick Links:** [Project Overview](../README.md) | [Product Brief](../productbrief.md) | [Memory Bank](../memory-bank/)

## Documentation Structure

This directory organizes all technical documentation for the ESP32 Theremin project.

---

## 📐 Architecture

System design and technical architecture documentation.

- **[ARCHITECTURE.md](architecture/ARCHITECTURE.md)** - Complete system architecture overview
  - Class structure and responsibilities
  - Component interaction patterns
  - Design decisions and rationale
  - Future extensibility considerations

---

## 📘 Guides

Step-by-step guides for setup, development, and debugging.

- **[OTA_SETUP.md](guides/OTA_SETUP.md)** - Over-The-Air firmware update setup
  - WiFi Access Point configuration
  - ElegantOTA web interface usage
  - Button activation feature
  - Security and authentication

- **[DEBUG_GUIDE.md](guides/DEBUG_GUIDE.md)** - Debugging and troubleshooting
  - Serial output interpretation
  - Common issues and solutions
  - Development workflow tips

- **[WOKWI_SIMULATION_PLAN.md](guides/WOKWI_SIMULATION_PLAN.md)** - Virtual prototyping guide
  - Wokwi simulator setup
  - Circuit configuration
  - Testing strategies

---

## 🚀 Improvements & Implementation Notes

Detailed documentation of major improvements, optimizations, and implementation decisions.

### Audio System

- **[CONTINUOUS_AUDIO_IMPLEMENTATION.md](improvements/CONTINUOUS_AUDIO_IMPLEMENTATION.md)**
  - FreeRTOS audio task implementation
  - Thread-safe parameter updates
  - I2S DAC configuration
  - Performance measurements

- **[WAVEFORM_IMPLEMENTATION.md](improvements/WAVEFORM_IMPLEMENTATION.md)**
  - Multiple waveform types (square, sine, triangle, sawtooth)
  - Lookup table vs mathematical generation
  - Sound characteristics comparison
  - Usage and testing guide

- **[DAC_FORMAT_FIX.md](improvements/DAC_FORMAT_FIX.md)**
  - ESP32 built-in DAC sample format requirements
  - Signed 16-bit to unsigned 8-bit conversion
  - Distortion elimination fix
  - Technical explanation and testing guide

- **[DAC_MIGRATION_PCM5102.md](improvements/DAC_MIGRATION_PCM5102.md)** ⭐ NEW
  - Migration from ESP32 built-in 8-bit DAC to external 16-bit PCM5102 I2S DAC
  - Hardware specifications, pin connections, and software implementation
  - 256x resolution improvement (8-bit → 16-bit), true stereo output
  - Dramatically improved audio quality (THD+N < -93 dB) with zero performance penalty
  - Minor low-volume effect glitches documented with future solutions

- **[EFFECTS_IMPLEMENTATION.md](improvements/EFFECTS_IMPLEMENTATION.md)** ⭐
  - Complete audio effects system (Delay, Chorus, Reverb)
  - Modular architecture with EffectsChain coordinator
  - Stack allocation (RAII pattern) for zero heap fragmentation
  - Oscillator-based LFO (~100x faster than sin() calls)
  - Noise gate pattern for clean reverb decay
  - Performance: 14.5% CPU with all 3 effects (85% headroom!)
  - Phase-by-phase implementation details
  - Design patterns and lessons learned

### Sensor System

- **[PITCH_SMOOTHING_IMPROVEMENTS.md](improvements/PITCH_SMOOTHING_IMPROVEMENTS.md)**
  - Exponential smoothing (EWMA) implementation
  - Floating-point frequency mapping
  - High-speed sensor timing optimization
  - Optimized reading architecture
  - Tuning guide and parameters

- **[PITCH_STEPPING_ISSUE.md](improvements/PITCH_STEPPING_ISSUE.md)**
  - Issue analysis and root causes
  - Proposed solutions comparison
  - Implementation recommendations

### System Monitoring

- **[PERFORMANCE_MONITORING.md](improvements/PERFORMANCE_MONITORING.md)**
  - Watchdog-style threshold monitoring
  - Audio real-time deadline tracking (11ms buffer period)
  - RAM usage alerts
  - Silent when OK, alerts only on thresholds
  - Lightweight and accurate (excludes I/O blocking)

---

## 📝 Documentation Conventions

### File Naming
- Use `SCREAMING_SNAKE_CASE.md` for all documentation files
- Be descriptive but concise
- Group related docs in subdirectories

### Content Structure
Each documentation file should include:
1. **Title and brief description**
2. **Implementation date** (for improvement docs)
3. **Problem/Goal statement**
4. **Solution/Implementation details**
5. **Results and measurements**
6. **Related files and references**

### Cross-Referencing
- Use relative paths for internal links
- Reference code files explicitly
- Link to related documentation

---

## 🔍 Finding Documentation

### By Topic

**Setting up the project:**
- Start with [../README.md](../README.md)
- Review [WOKWI_SIMULATION_PLAN.md](guides/WOKWI_SIMULATION_PLAN.md) for virtual testing
- Check [OTA_SETUP.md](guides/OTA_SETUP.md) for wireless updates

**Understanding the codebase:**
- Read [ARCHITECTURE.md](architecture/ARCHITECTURE.md) for system overview
- Check [memory-bank/systemPatterns.md](../memory-bank/systemPatterns.md) for design patterns

**Debugging issues:**
- Consult [DEBUG_GUIDE.md](guides/DEBUG_GUIDE.md)
- Review improvement docs for known issues and solutions

**Learning from implementations:**
- Browse [improvements/](improvements/) directory
- Check git history for implementation context

### By Phase

**Phase 0 - Setup:**
- WOKWI_SIMULATION_PLAN.md
- DEBUG_GUIDE.md

**Phase 1 - Foundation:**
- ARCHITECTURE.md
- OTA_SETUP.md

**Phase 2 - Audio & Sensors:**
- CONTINUOUS_AUDIO_IMPLEMENTATION.md
- WAVEFORM_IMPLEMENTATION.md
- DAC_FORMAT_FIX.md
- PITCH_SMOOTHING_IMPROVEMENTS.md
- PITCH_STEPPING_ISSUE.md
- PERFORMANCE_MONITORING.md

**Phase 4 - Effects:**
- EFFECTS_IMPLEMENTATION.md ⭐ Complete!

**Future Phases:**
- Phase 3 (hardware controls + display) - pending parts
- Phase 5+ - documentation will be added as features are implemented

---

## 📊 Project Status

Current documentation covers:
- ✅ System architecture
- ✅ OTA update system
- ✅ Audio system improvements (waveforms, DAC, continuous generation)
- ✅ **Audio effects system (Delay, Chorus, Reverb)** ⭐ NEW
- ✅ Sensor optimizations
- ✅ System performance monitoring
- ✅ Debug workflows
- ✅ Virtual prototyping

**Latest Achievement:** Phase 4 complete - Three-effect audio engine running on hardware with 14.5% CPU usage!

See [memory-bank/progress.md](../memory-bank/progress.md) for detailed project status.

---

## 🤝 Contributing to Documentation

When adding new documentation:

1. **Choose the right category:**
   - `architecture/` - System design decisions
   - `guides/` - How-to and setup instructions
   - `improvements/` - Implementation notes and optimizations

2. **Follow the template structure:**
   - Clear title and date
   - Problem statement
   - Solution with code examples
   - Results and measurements
   - Related files

3. **Update this README:**
   - Add new file to appropriate section
   - Include brief description
   - Maintain alphabetical order within sections

4. **Cross-reference:**
   - Link from related docs
   - Update memory-bank if architecture changes
   - Reference in progress.md if milestone achieved

---

**Last Updated:** November 4, 2025
