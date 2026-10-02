# AI contribution rules

These instructions apply to every AI agent working anywhere in this repository.

## Design log

After every architectural change, update `design-log/` before completing the
task:

1. Create a numbered log file: `design-log/NNN-short-slug.md`. Determine the
   next zero-padded number from the existing files and follow the structure of a
   recent entry exactly: **Background**, **Problem**, **Design**, **Questions and
   Answers**, **Trade-offs**, **Implementation Results**.
2. Add a row to `design-log/index.md` with the number, linked title, status, and
   a one-line description.

Architectural changes include new or changed hardware abstractions, runtime
settings, communication protocols, storage patterns, public interfaces, module
boundaries, or cross-cutting application patterns.

Do not create a new log for a bug fix that does not change the design; append
the result to the relevant entry instead. Cosmetic-only changes and renames with
no behavioural change need no design-log entry. Never rewrite or delete existing
log history without explicit user approval.

Run `/design-log` after an architectural change when the command is available;
otherwise perform its documented procedure manually before the final response.
