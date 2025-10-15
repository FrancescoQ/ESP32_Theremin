# Memory Bank - ESP32 Theremin

This directory contains Cline's Memory Bank for the ESP32 Theremin project. The Memory Bank is a structured documentation system that enables Cline to maintain context across sessions.

## Purpose

Cline's memory resets completely between sessions. The Memory Bank serves as the **sole source of truth** for understanding project state, decisions, and progress. Every file here is critical for continuing work effectively.

## File Structure

### Core Files (Always Read First)

1. **projectbrief.md** - Foundation document
   - Core project identity and goals
   - Must-have requirements
   - Success criteria
   - Technical constraints

2. **productContext.md** - The "Why" and "How"
   - Problem being solved
   - User experience goals
   - Expected behavior
   - Design principles

3. **systemPatterns.md** - Architecture and Design
   - System architecture diagrams
   - Key technical decisions with rationale
   - Design patterns in use
   - Critical implementation paths
   - Component relationships

4. **techContext.md** - Technology and Tools
   - Complete technology stack
   - Hardware specifications
   - Development environment setup
   - Library dependencies
   - Tool usage patterns

5. **activeContext.md** - Current Work State
   - What's being worked on RIGHT NOW
   - Recent changes
   - Active decisions and open questions
   - Important patterns and preferences
   - Key learnings and insights
   - **READ THIS FIRST when resuming work**

6. **progress.md** - Project Status
   - What's complete
   - What's left to build
   - Known issues
   - Evolution of decisions
   - Milestone tracking

## How to Use This Memory Bank

### When Starting a Session

1. **Always read activeContext.md first** - Get immediate context
2. **Check progress.md** - Understand current phase and status
3. **Review relevant sections** of other files as needed
4. **Never assume** - If it's not documented, it hasn't happened

### When Updating the Memory Bank

**Update triggers:**
- Discovering new project patterns or insights
- After implementing significant changes
- When user explicitly requests: **update memory bank**
- When context needs clarification

**When updating (especially when user says "update memory bank"):**
- Review ALL files, even if not all need updates
- Focus particularly on activeContext.md and progress.md
- Document what was learned
- Update "Recent Changes" section
- Clarify next steps

**What to update:**
- **activeContext.md** - Always update with current state
- **progress.md** - Update completion status and milestones
- **systemPatterns.md** - If architecture or patterns changed
- **productContext.md** - If understanding of user needs evolved
- **techContext.md** - If new technologies or constraints discovered
- **projectbrief.md** - Only if core requirements changed (rare)

## File Dependencies

```
projectbrief.md (Foundation)
    ↓
├── productContext.md (Why/How)
├── systemPatterns.md (Architecture)
└── techContext.md (Technology)
    ↓
activeContext.md (Current Focus)
    ↓
progress.md (Status Tracking)
```

## Current Project Status

**Phase:** Initial Setup - Memory Bank Initialized
**Date:** October 15, 2025

This ESP32 Theremin project is just beginning. Memory bank has been fully initialized with comprehensive documentation. Ready to begin Phase 0 (Development Environment Setup and Simulation).

## Quick Reference

- **Project Type:** DIY Electronic Musical Instrument
- **Hardware:** ESP32 + 2x VL53L0X ToF Sensors + Passive Buzzer
- **Development:** VSCode + PlatformIO + Arduino Framework
- **Approach:** Wokwi simulation first, then real hardware
- **Current Phase:** Phase 0 - Environment Setup (Not Started)

## Important Notes

- Memory Bank is the **single source of truth**
- All files must be kept in sync with reality
- Document decisions and reasoning, not just facts
- Update regularly to maintain accuracy
- Never work from memory - always verify documentation

---

For detailed information about each aspect of the project, read the corresponding file. Start with activeContext.md for immediate context.
