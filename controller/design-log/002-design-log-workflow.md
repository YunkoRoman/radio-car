# 002 — Numbered architectural design-log workflow

## Background

The initial repository journal used one `DESIGN_LOG.md` file containing a short
entry for every AI file update. It did not distinguish between architectural
decisions and routine edits, and records could become difficult to navigate as
the project grows.

## Problem

The project needs the established `scraper` pattern: durable, separately
addressable architecture records, a discoverable index, and clear rules for when
a new entry is warranted.

## Design

`design-log/` is the canonical record:

- Each architectural decision receives an immutable numbered file named
  `NNN-short-slug.md`.
- `design-log/index.md` catalogs the number, linked title, status, and summary.
- Every entry contains Background, Problem, Design, Questions and Answers,
  Trade-offs, and Implementation Results.
- `AGENTS.md` mandates this flow for all AI agents. The command definition in
  `.claude/skills/design-log/SKILL.md` documents the `/design-log` procedure.

Routine bug fixes append their outcome to the relevant architectural record.
Cosmetic changes and behaviour-neutral renames require no record.

## Questions and Answers

**Q: Should every AI edit make a separate design-log file?**

**A:** No. Design logs capture architectural decisions, not a raw change
history. This avoids noise while preserving the rationale that future work needs.

**Q: How is the next entry chosen?**

**A:** The agent derives the next three-digit number from existing files before
creating an entry, then updates the index in the same task.

## Trade-offs

- Maintaining an index and individual files costs more than appending one line
  to a journal, but makes decisions linkable and readable at scale.
- Small routine changes may not be represented in the log; version control
  remains the detailed source of change history.

## Implementation Results

- Replaced the flat journal with the numbered `design-log/` directory.
- Added the index and preserved the original PlatformIO decision as entry 001.
- Added entry 002 for this workflow and a `/design-log` skill definition.
- Updated `AGENTS.md` and `README.md` to reference the new canonical flow.
