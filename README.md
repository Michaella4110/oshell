# OShell - A Simplified Unix Shell

## Author

**Tesfamichael Assefa – 3630/16**

## Project Description

OShell is a simplified Unix command-line interpreter that implements core shell functionality including process management, command parsing, file I/O, and signal handling. The shell supports three invocation modes (interactive, pipe, and batch), implements built-in commands, variable expansion, and follows standard shell conventions.

## Features Implemented

### All Specification Requirements Implemented

#### 1. Invocation Modes

* **Interactive Mode**: Shows `$ ` prompt, uses `isatty()` detection
* **Pipe Mode**: Reads commands from stdin (non-interactive)
* **Batch Mode**: Executes commands from file
* **Argument Validation**: Accepts 0 or 1 argument only (more cause error)

#### 2. Parsing Features

* `;` - Sequential execution (one after another)
* `&&` - Conditional AND (run if previous succeeded)
* `||` - Conditional OR (run if previous failed)
* `&` - Parallel execution (run simultaneously, wait for all)
* `#` - Comments (ignore rest of line)
* `>` - Redirection (stdout+stderr to file, one per command)

#### 3. Built-in Commands (No Forking/Exec)

* `exit [status]` - Exit shell with optional numeric status code
* `cd [dir]` - Change directory (supports `-`, `--`, `$HOME`, `$OLDPWD`)
* `env` - Display all environment variables
* `setenv NAME VALUE` - Set environment variable
* `unsetenv NAME` - Remove environment variable
* `alias` - Create, display, or manage command aliases
* `path` - Set internal search path for external commands

#### 4. Variable Expansion

* `$VAR` - Expands to environment variable value
* `$?` - Expands to last command exit status
* `$$` - Expands to shell's process ID
* `$UNDEFINED` - Expands to empty string (bash-like)

#### 5. PATH Search Behavior

* **Internal path list** (not inherited from environment)
* **Default**: `/bin`
* Searches with `access(path, X_OK)`
* Supports absolute (`/usr/bin/ls`) and relative (`./program`) paths
* Exit codes: 127 (not found), 126 (not executable)

#### 6. Exit Status

* Returns exit status of last executed command
* 0 = success, non-zero = failure
* Builtins return 0 on success, non-zero on incorrect usage

#### 7. Error Handling

* **Unified Error Message**: "An error has occurred"
* Continues after most errors (except invalid batch file/argument count)
* Invalid batch file → exit 1, bad argument count at startup → exit

#### 8. Signal Handling

* **Ctrl+C** at prompt → ignored (shell continues)
* **Ctrl+D** → exits shell (EOF handling)

#### 9. Whitespace Handling

* Arbitrary spaces/tabs allowed around commands
* Operators work without spaces: `ls>out.txt`, `echo hi&&echo bye`

#### 10. Man Pages

* Complete man pages for all 7 built-in commands + main shell + builtins overview
* Files: `exit.1`, `cd.1`, `env.1`, `setenv.1`, `unsetenv.1`, `alias.1`, `path.1`, `oshell.1`, `builtins.1`

## Project Structure

```
oshell/
├── README.md
├── Makefile
├── man/
│   ├── alias.1
│   ├── builtins.1
│   ├── cd.1
│   ├── env.1
│   ├── exit.1
│   ├── oshell.1
│   ├── path.1
│   ├── setenv.1
│   └── unsetenv.1
├── src/
│   ├── main.c
│   ├── include/
│   │   ├── errors.h
│   │   ├── shell.h
│   │   └── utils.h
│   ├── core/
│   │   ├── builtins.c
│   │   ├── errors.c
│   │   ├── executor.c
│   │   ├── expander.c
│   │   ├── parser.c
│   │   ├── state.c
│   │   └── utils.c
│   └── modes/
│       ├── batch.c
│       ├── determine.c
│       ├── interactive.c
│       ├── pipe.c
│       └── modes.h
└── oshell
```

## Compilation Instructions

### Method 1: Using Makefile

```bash
cd oshell
make clean
make
```

### Method 2: Manual Compilation

```bash
gcc -Wall -Wextra -Werror -Isrc/include \
src/main.c \
src/core/builtins.c \
src/core/errors.c \
src/core/executor.c \
src/core/expander.c \
src/core/parser.c \
src/core/state.c \
src/core/utils.c \
src/modes/batch.c \
src/modes/determine.c \
src/modes/interactive.c \
src/modes/pipe.c \
-o oshell
```

## Execution Instructions

### Interactive Mode

```bash
./oshell
```

### Pipe Mode

```bash
echo "echo hello" | ./oshell
```

### Batch Mode

```bash
./oshell script.txt
```

## Testing Examples

### Operators

```bash
echo first ; echo second
echo a && echo b
cat /nonexistent || echo "Error handled"
sleep 1 & echo "Immediate"
echo visible # invisible
ls -la > output.txt
```

### Built-ins

```bash
cd /tmp && pwd
env
setenv TESTVAR value
unsetenv TESTVAR
alias ll='ls -l'
path /bin /usr/bin
exit 0
```

### Variable Expansion

```bash
echo "$HOME"
echo "$$"
echo "$?"
```

## Limitations

* No pipe (`|`) operator
* No input redirection (`<`)
* No append redirection (`>>`)
* No command substitution ($(cmd) or backticks)
* No job control (jobs, fg, bg)

## Attribution & Acknowledgement

This project was developed for educational purposes as part of an Operating Systems course. All code presented herein is the sole work of **Tesfamichael Assefa**.

We extend sincere gratitude to our instructor, **Mr. Abebayehu**, for the detailed project specification and guidance throughout the development process.

---

*Project Completed: December 2025 | Operating Systems Course Project*
