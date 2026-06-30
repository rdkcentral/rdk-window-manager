# RDK Window Manager Copilot Instructions

## Review Comment Linking Guidelines

When writing review comments based on custom instructions located in `.github/instructions/**.instructions.md`, include a direct GitHub link to the exact violated guideline section in the corresponding instruction file.

Use this format:

```text
Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/<instruction-file>.instructions.md#guideline-section-name
```

## Examples

```text
Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#runtime-logging

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#pointer-and-handle-safety

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#external-api-error-handling

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#thread-safety-for-shared-state

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#event-emission-rules

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#client-name-canonicalization

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#environment-and-config-parsing

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#cmake-feature-gating

Refer: https://github.com/rdkcentral/rdk-window-manager/blob/develop/.github/instructions/General.instructions.md#tests-for-public-behavior
```
