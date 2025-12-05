# AI Agent Instructions

> **Purpose**: Behavioral rules for AI agents (Claude, Copilot, etc.) working on this codebase.  
> **Last Updated**: 2025-12-05  
> **Version**: 2.1

---

## Before Starting Any Work

### 1. Read Project Documents First

**BEFORE doing any work, read these documents:**

1. [DESIGN_PHILOSOPHY.md](file:///home/siddu/astra/DESIGN_PHILOSOPHY.md) - Core principles
2. [ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md) - Current system state
3. [DESIGN_GAPS.md](file:///home/siddu/astra/DESIGN_GAPS.md) - Known gaps

---

## Core Rules

### 2. Host Environment Restrictions

**CRITICAL**: Do NOT run arbitrary commands on the host machine.

- ✅ **ALLOWED**: `docker exec astra <command>`
- ❌ **FORBIDDEN**: Any other command execution on the host

**Why**: All development happens inside the container.

### 2.5 Resource Constraints

**Use only 2 cores** for compile and tests (aligned with 2-core pod design).

```bash
# Build
ninja -C build/gcc-debug -j2

# Tests
ctest --test-dir build/gcc-debug
```

### 3. Ask Permission Before Coding

**ALWAYS ask permission before making changes.**

- Discuss design/approach FIRST
- Wait for explicit approval
- Then implement

**Exception**: Minor fixes (typos, formatting) can proceed without explicit approval.

### 4. Design Discussion First

**"Coding is the last thing we do."**

1. Understand the requirement
2. Discuss architecture implications
3. Get approval on approach
4. Only then implement

---

## Token-Saving Workflow

**For build/test cycles**: User runs commands in separate terminal, pastes errors.

- AI provides the fix steps
- User runs commands
- User pastes only errors (if any)
- AI fixes errors

This avoids wasting tokens on long build output.

---

## Communication Protocol

1. **Before starting**: Explain what you plan to do
2. **Ask questions**: Clarify requirements before assuming
3. **Present options**: Offer alternatives when there are trade-offs
4. **Report blockers**: Immediately flag issues that prevent progress
5. **Summarize**: After completion, summarize what was done

---

## Related Documents

- [DESIGN_PHILOSOPHY.md](file:///home/siddu/astra/DESIGN_PHILOSOPHY.md) - Design principles
- [ARCHITECTURE.md](file:///home/siddu/astra/ARCHITECTURE.md) - System architecture
- [CODE_REVIEW.md](file:///home/siddu/astra/CODE_REVIEW.md) - Code review checklist
- [README.md](file:///home/siddu/astra/README.md) - Build commands

---


## Token-Saving Workflow

**For build/test cycles**: User runs commands in separate terminal, pastes errors.

- AI provides the fix steps
- User runs commands
- User pastes only errors (if any)
- AI fixes errors

This avoids wasting tokens on long build output.
---
**End of Document**
