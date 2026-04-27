

# myShell
![C](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20WSL-lightgrey)

A simple Unix-style shell implemented in C as a project for  CS280: Computer Systems and Architecture .



## Team Members
- Abdulla Alshaikh
- Fatema Albaqlawa
- Fatema Althawadi
- Malek Albaroudi
- Wejdan Alsalam

## Built-in Commands

| Command | Description |
|---|---|
| `cd` | Change directory to `HOME` or to a specified path. |
| `getpath` | Print the current `PATH`. |
| `setpath <path>` | Set the `PATH` to a new value. |
| `history` | Display the stored command history. |
| `!!` | Re-run the most recent command. |
| `!n` | Run command number `n` from history. |
| `!-n` | Run the command that is `n` steps back from the current command. |
| `alias` | List all aliases. |
| `alias name command` | Create or update an alias. |
| `unalias name` | Remove an alias. |
| `exit` | Exit the shell. |


## File Persistence

The shell saves and loads:
- history in `.hist_list`
- aliases in `.aliases`

These files are stored in the user’s home directory.

## Installation & Compilation
**Requirements:** GCC, Linux/WSL

```bash
# Clone the repository
git clone https://github.com/alshaikh-exe/myShell.git
cd myShell

# Compile
gcc -Wall -o myShell myShell.c

# Run
./myShell


