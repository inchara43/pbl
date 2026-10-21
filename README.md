# Week 4: Multi-Process Simulator & IPC

## Project Overview
This project implements a multi-process simulator in C to demonstrate Inter-Process Communication (IPC) in Linux.

The simulator supports CPU operations, memory storage, stack operations, queue operations, and status monitoring.

## Team Roles
- **UI Process:** Accepts user input and sends commands using a POSIX message queue.
- **Core Process:** Processes commands, manages simulator operations, communicates through TCP sockets, and sends log events through a named pipe.
- **Logger Process:** Receives log events through a named pipe and records them in `simulator.log`.
- **Team Leader:** Integrates the components, tests the simulator, and prepares documentation and benchmarks.

## IPC Mechanisms
1. **POSIX Message Queue:** Transfers commands from the UI to the Core.
2. **TCP Sockets:** Return responses from the Core to the UI.
3. **Named Pipe (FIFO):** Transfers log messages from the Core to the Logger.

## Features
- CPU operation counter
- Memory value storage
- Stack push and pop
- Queue enqueue and dequeue
- Simulator status display
- Logging of commands and results
- Graceful shutdown

## Requirements
- Linux or WSL (Windows Subsystem for Linux)
- GCC compiler# pbl
