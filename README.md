# STDISCM-P2

**Author:** Atasha Dominique C. Pidlaoan - S17

## Overview
This project simulates a synchronized dungeon queue system for an MMORPG, managing multiple concurrent dungeon instances and ensuring fair party formation. The system prevents deadlock and starvation by controlling access to player resources and dungeon slots.

## Configuration
Edit `config.txt` to set:
- `n` – Maximum concurrent dungeon instances
- `t` – Number of Tank players
- `h` – Number of Healer players
- `d` – Number of DPS players
- `t1` – Minimum dungeon completion time (seconds)
- `t2` – Maximum dungeon completion time (seconds, ≤ 15 for testing)

## Output
- Displays the status (`active` or `empty`) of each dungeon instance.
- Shows a summary: parties served and total time spent per instance.
- Lists leftover players after all parties are formed.

## Build & Run Instructions

### macOS / Linux
1. Open a terminal and navigate to the project folder.
2. Compile the program:
   ```
   g++ -std=c++11 -o P2.out P2.cpp
   ```
3. Run the program:
   ```
   ./P2.out
   ```

### Windows
1. Install MinGW and add it to your system PATH.
2. Open Command Prompt and navigate to the project folder.
3. Compile:
   ```
   g++ -std=c++11 -o P2.exe P2.cpp
   ```
4. Run:
   ```
   P2.exe
   ```

### Visual Studio Code
1. Open the project folder in VS Code.
2. Open `P2.cpp`.
3. Use the built-in terminal for compilation and running (see above).
4. Alternatively, use the "Run" button or press `Ctrl + F5` if configured.

## Troubleshooting
- **GCC not found:** Install GCC and ensure it is in your PATH.
- **MinGW issues (Windows):** Confirm MinGW is installed and added to PATH.
- **Config errors:** Ensure all values in `config.txt` are valid and present.

---