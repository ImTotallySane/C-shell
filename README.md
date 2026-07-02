# C Shell

A modular, POSIX-compliant custom shell implementation in C. Built using the POSIX C system APIs, this shell supports execution of arbitrary system commands, background/foreground task control, multiple-stage piping, input/output redirections, sequential operators, custom interrupts/suspend signals, and persistence-backed execution history.

## Table of Contents
1. [Key Features](#key-features)
   - [Interactive Custom Prompt](#interactive-custom-prompt)
   - [Grammar & Input Parsing](#grammar--input-parsing)
   - [File Redirection & Pipes](#file-redirection--pipes)
   - [Execution Operators](#execution-operators)
   - [Shell Intrinsics & Job Control](#shell-intrinsics--job-control)
   - [Persistent Command Logging](#persistent-command-logging)
2. [Codebase Architecture](#codebase-architecture)
3. [Compilation and Setup](#compilation-and-setup)
4. [Example Usage](#example-usage)

---

## Key Features

### Interactive Custom Prompt
- **Format**: `<Username@SystemName:current_path>`
- Displays dynamically whenever the shell is waiting for user input and not executing foreground processes.
- **Dynamic Home Aliasing**: The directory in which the shell starts becomes its home directory (`~`). Any path within this home directory tree will automatically replace the absolute home path with a tilde prefix (e.g., `/home/user/shell/src` displays as `~/src`). Paths outside this hierarchy are printed as absolute.

### Grammar & Input Parsing
The parser complies with the following Context Free Grammar (CFG) rules:
- Combines sequential commands (`&` or `;`), pipes (`|`), and redirectional tokens (`<`, `>`, `>>`).
- Gracefully ignores arbitrary amounts of whitespace (spaces, tabs, newlines).
- Syntax validation checks for structural issues (e.g., stray separators, mismatched pipelines) and outputs `Invalid Syntax!` when grammar rules are violated.

### File Redirection & Pipes
- **Input Redirection (`<`)**: Redirects standard input of a command to read from a file. Performs a preflight check to ensure the file exists and is readable, printing `No such file or directory` if it is not.
- **Output Redirection (`>` and `>>`)**: Overwrites (`>`) or appends (`>>`) the standard output of a command to a file. Checks write-access permission before forking, printing `Unable to create file for writing` on failure.
- **Redirection Chaining**: In cases where multiple input or output redirections are provided, only the last redirection of each type takes effect.
- **Pipes (`|`)**: Connects the standard output of one process to the standard input of the next. Spawns appropriate pipelines, handles fork-based isolation, and ensures that the parent shell blocks and waits until all commands in the pipe sequence have finished execution.

### Execution Operators
- **Sequential Execution (`;`)**: Runs multiple commands in left-to-right order. Continues to subsequent commands even if previous ones fail.
- **Background Execution (`&`)**: Commands ending with `&` run asynchronously. The shell displays a background job report `[job_number] process_id` and immediately prints the prompt for new inputs. When a background process terminates, the shell intercepts the exit and displays whether the task `exited normally` or `exited abnormally` prior to displaying the next prompt.

### Shell Intrinsics & Job Control
- **`hop` / `cd`**: Shifts the current working directory. Resolves target arguments sequentially:
  - `~` or no arguments: Returns to the shell home directory.
  - `.`: Remains in the current directory.
  - `..`: Moves to the parent directory.
  - `-`: Steps back to the previous working directory.
  - `name`: Resolves the absolute or relative directory name.
- **`reveal`**: Lists files and folders in lexicographical order. Supports flags:
  - `-a`: Reveals hidden entries starting with `.`.
  - `-l`: Lists one entry per line.
  - Accept paths/aliases matching the `hop` conventions (e.g. `reveal -al ~/src`).
- **`activities`**: Lists all active and stopped jobs spawned by the shell in the format: `[pid] : command_name - State` (sorted lexicographically by command name).
- **`ping <pid> <signal_number>`**: Dispatches signal (`signal_number % 32`) to the target PID.
- **`fg [job_number]`**: Recovers a background or suspended job, brings it to the foreground, sends `SIGCONT` if stopped, and blocks the shell until it completes.
- **`bg [job_number]`**: Resumes a stopped job in the background (sends `SIGCONT`).
- **`exit`**: Cleanly terminates the shell sessions, sending `SIGKILL` to all active child processes.
- **Keyboard Shortcuts**:
  - `Ctrl+C`: Interrupts the current foreground process group without crashing the shell.
  - `Ctrl+D`: EOF detection. Kills all running child processes, prints `logout`, and exits with code 0.
  - `Ctrl+Z`: Suspends the active foreground process, moving it to the background job list as `Stopped`.

### Persistent Command Logging
- Saves a maximum of 15 commands.
- Automatically persists history across shell restarts by reading/writing to a `.shell_log` file in the shell's home directory.
- Avoids duplicates: consecutive identical commands are not appended.
- Avoids circular recursion: commands beginning with `log` are ignored.
- **Sub-commands**:
  - `log`: Prints the current history buffer in oldest-to-newest order.
  - `log purge`: Empties the history log and removes the `.shell_log` file.
  - `log execute <index>`: Re-runs the command at `<index>` (one-indexed, from newest to oldest).

---

## Codebase Architecture

The project is structured logically across modular header and source files:

*   **`shell/src/main.c`**: Initializes shell environment configuration, activates signal traps, initializes the log system, and runs the main read-evaluate-print loop (REPL).
*   **`shell/src/prompt.c`** / **`shell/include/prompt.h`**: Formulates user, hostname, and path resolution for the prompt display.
*   **`shell/src/parser.c`** / **`shell/include/parser.h`**: Lexes and parses command strings using strtok-based tokenizer arrays into structured representations (`ShellCommand`, `CommandGroup`, `AtomicCommand`, and `Redirect`).
*   **`shell/src/execute.c`** / **`shell/include/execute.h`**: Manages execution flow, processes file redirection descriptors via `dup2`, builds file pipes, and implements process synchronization.
*   **`shell/src/intrinsics.c`** / **`shell/include/intrinsics.h`**: Defines the handler functions and dispatcher routing for the shell's built-in commands.
*   **`shell/src/jobs.c`** / **`shell/include/jobs.h`**: Tracks child process PIDs, updates state variables, handles non-blocking child checks via `waitpid`, and manages job backgrounding commands.
*   **`shell/src/log.c`** / **`shell/include/log.h`**: Manages storage array logic, disk writing, and indexing for command history log actions.
*   **`shell/src/signals.c`** / **`shell/include/signals.h`**: Customizes sigaction attributes for SIGINT and SIGTSTP interrupts to intercept keyboard controls.

---

## Compilation and Setup

> **System Requirements**: This project relies heavily on Unix/POSIX-specific APIs (such as `fork()`, `execvp()`, `pipe()`, `dup2()`, `sigaction()`, and `waitpid()`) and standard POSIX headers. It **cannot** be built or run natively on Windows command lines (CMD/PowerShell). Windows users must compile and run this within **WSL (Windows Subsystem for Linux)** or a POSIX-compliant environment (like MSYS2 or Cygwin).

The shell compiles under strict standard specifications (`-std=c99`) and POSIX features (`_POSIX_C_SOURCE=200809L`, `_XOPEN_SOURCE=700`).

### Compiling
Navigate to the `shell` subdirectory and run the Makefile:
```bash
cd shell
make all
```
This builds and links the source files, outputting the executable `shell.out` within the `shell` folder.

### Running
Run the compiled shell:
```bash
./shell.out
```

### Cleaning Up
To clean compiled object binaries and binary targets:
```bash
make clean
```
