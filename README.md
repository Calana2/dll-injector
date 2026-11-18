# dll-injector

Yet another DLL injection / ejection tool.

## Features

- **GUI** and **CLI** tools for 32-bits and 64-bits executables.
- **Injection** via the classic `CreateRemoteThread` method.
- **Ejection** via `FreeLibrary`.
- Statically compiled binaries available in *releases*.

> **Note:** Ejection may cause a crash due to the fragility of multithreading. Only a robust DLL will avoid this issue.

## Building

**Requirements:** MinGW (compiler) and `make` (build tool). [winlamb](https://github.com/rodrigocfd/winlamb) is used as a wrapper for the Win32 API.

| Command | Description |
|---|---|
| `make samples` | Build the sample DLLs |
| `make injectors` | Build all injectors |
| `make cli_injectors` | Build CLI injectors only |
| `make gui_injectors` | Build GUI injectors only |
| `make all` | Build everything |

## Usage

CLI: `injector64.exe --dll foo.dll --exe bar.exe [--action INJECT|EJECT]   (default: INJECT)`

## Limitations

- Classic injection (`CreateRemoteThread`) is detected by most AV/EDR solutions. I might add more techniques in the future.
