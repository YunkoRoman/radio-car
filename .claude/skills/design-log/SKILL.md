---
name: design-log
description: Create a new numbered design-log entry and register it in the index. Use after an architectural change — hardware abstractions, runtime settings, communication protocols, storage patterns, public interfaces, module boundaries, or cross-cutting application patterns — as mandated by AGENTS.md.
---

Record an architectural decision in `design-log/`. This enforces the standing
rule in `AGENTS.md`: every architectural change gets a log entry.

## Steps

1. **Find the next number.** List `design-log/` and take the highest
   `NNN-*.md` + 1. Zero-pad to three digits (for example, `007`).
2. **Read a recent entry.** Match its exact section structure and tone; do not
   invent a new format.
3. **Create `design-log/NNN-short-slug.md`** with these sections:
   - **Background** — the relevant existing context.
   - **Problem** — what required the change.
   - **Design** — the decision and how it works, with concrete files or
     symbols.
   - **Questions and Answers** — questions raised during the design and their
     resolutions.
   - **Trade-offs** — alternatives and their consequences.
   - **Implementation Results** — what was actually delivered. For later bug
     fixes that do not change the design, append here instead of making a new
     log.
4. **Append a row to `design-log/index.md`** using its existing columns: log
   number, linked title, status, and a one-line description.

## When not to create a new entry

- Bug fixes that do not change the design: update **Implementation Results** of
  the relevant log.
- Cosmetic or style-only changes, and renames without behavioural changes.

## Checklist

- [ ] Next number computed with no collision
- [ ] New entry uses all six sections with concrete details
- [ ] `design-log/index.md` contains a correct linked row and status
