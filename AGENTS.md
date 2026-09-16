# AGENTS.md

## Code Review Rules

### Scope and correctness

- Analyze the PR diff, relevant repository context, and relevant external-library behavior, but report findings only when the defect is introduced or exposed by the PR's changed code.
- Verify that the change solves the PR's stated problem and does not regress existing behavior, performance, compatibility, or other modules. Flag changes that are disproportionate to the stated topic or touch unrelated modules without a demonstrated need.

### Tests and CI

- Require tests that validate the changed behavior and protect against foreseeable regressions caused by later work in interacting modules. Treat missing test coverage as blocking when either code coverage or functional coverage for the changed behavior is below 80%.
- Treat as blocking when a test can detect a defect but does not make CI or the build fail, allowing the defective change to merge.
- Do not flag multiple tests exercising the same feature unless they are exact duplicates. Do not use review findings for formatting, lint, or other deterministic CI checks.

### Architecture, documentation, and security

- For architecture or API changes, require the corresponding documentation in the PR or evidence that the documentation is already updated and remains accurate.
- Flag security vulnerabilities and potential backdoors. For a credible backdoor concern, explicitly include `@myungjoo-ham` in the PR comment.

### Review reporting and commit structure

- Every finding must state severity and why it matters, and must clearly say that the comment is an AI-agent review finding. For a non-blocking (NIT) finding, include a single short sentence assessing impact.
- Treat an excessive number of commits relative to the change's complexity, unrelated topics combined in one commit, or a fix for a particular commit split into a later corrective commit as a review concern. Separate feature and test commits are acceptable, and a single topic may span multiple coherent commits.
