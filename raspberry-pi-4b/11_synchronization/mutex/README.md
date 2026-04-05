# Kernel Mutex Synchronization Demo

## Overview

This kernel module demonstrates the usage of **mutexes** for synchronizing access to shared resources among concurrent kernel threads. It implements a practical example where two kernel threads with different priorities contend for access to a shared data structure while maintaining data integrity.

## Purpose

The `mutex_demo` module serves as an educational tool to understand:

- **Mutual Exclusion**: How mutexes prevent race conditions when multiple threads access shared resources
- **Thread Synchronization**: Kernel mechanisms for coordinating execution of concurrent threads
- **Priority Levels**: How different thread priorities interact with synchronization primitives
- **Critical Sections**: Protection of code regions that modify shared state

## How It Works

### Architecture

The module creates two kernel threads with different priorities:

| Thread | Priority (nice) | Task |
|--------|-----------------|------|
| `high_prio_kthread` | -20 (highest) | Accesses shared buffer every 5 seconds |
| `low_prio_kthread`  | -19 (lower)   | Accesses shared buffer every 2 seconds |

### Thread Behavior

Each thread executes a loop that:

1. **Acquires the mutex** (`mutex_lock()`) — only one thread can proceed
2. **Enters critical section** — logs entry message
3. **Modifies shared buffer** — writes thread-specific data using `snprintf()`
4. **Sleeps** — holds mutex for its assigned duration (HIGH: 5s, LOW: 2s)
5. **Exits critical section** — logs exit message
6. **Releases the mutex** (`mutex_unlock()`) — allows other threads to proceed

### Synchronization Guarantee

The mutex ensures that:
- Only one thread accesses the buffer at a time
- No data corruption from concurrent modifications
- Predictable, serialized access to the shared resource

## Compilation

### Build Commands

```bash
# Compile the module
make all

# Check compilation
ls -la *.ko
```

### Makefile

The accompanying `Makefile` contains standard kernel module build rules using the Linux kernel build system.

## Usage

### Loading the Module

```bash
# Install the module
sudo insmod mutex_demo.ko

# Verify it loaded
lsmod | grep mutex_demo
```

### Observing Behavior

Monitor kernel messages to track mutex contention and thread execution:

```bash
# Real-time kernel log (in separate terminal)
sudo dmesg -W

# Or check kernel ring buffer
dmesg | tail -20

# Follow specific module messages
sudo dmesg -W | grep mutex_demo
```

### Expected Output

```
[1725786.853002] mutex_demo: Initializing threads in SCHED_NORMAL
[1725786.853161] mutex_demo: Running HIPRIO Priority Task, Copying: Hello from High Priority thread 
[1725786.853169] mutex_demo: Enter HIPRIO Shared Buffer: Active: Hello from High Priority thread
[1725792.037936] mutex_demo: Exit HIPRIO Shared Buffer: Active: Hello from High Priority thread
[1725792.037962] mutex_demo: Running HIPRIO Priority Task, Copying: Hello from High Priority thread 
[1725792.037969] mutex_demo: Enter HIPRIO Shared Buffer: Active: Hello from High Priority thread
[1725797.157974] mutex_demo: Exit HIPRIO Shared Buffer: Active: Hello from High Priority thread
[1725797.158004] mutex_demo: Running LOPRIO Priority Task, Copying: Hello from Low Priority thread 
[1725797.158010] mutex_demo: Enter LOPRIO Shared Buffer: Active: Hello from Low Priority thread
[1725799.174004] mutex_demo: Exit LOPRIO Shared Buffer: Active: Hello from Low Priority thread
[1725799.174025] mutex_demo: Running LOPRIO Priority Task, Copying: Hello from Low Priority thread 
[1725799.174030] mutex_demo: Enter LOPRIO Shared Buffer: Active: Hello from Low Priority thread
```

## Key Kernel APIs Demonstrated

### Mutex Operations

| API | Purpose |
|-----|---------|
| `mutex_init()` | Initialize an uninitialized mutex |
| `mutex_lock()` | Acquire mutex (blocking if held by another thread) |
| `mutex_unlock()` | Release mutex, allow waiters to proceed |
| `mutex_destroy()` | Clean up and release mutex resources |

### Thread Management

| API | Purpose |
|-----|---------|
| `kthread_run()` | Create and start a kernel thread |
| `kthread_should_stop()` | Check if thread should terminate |
| `kthread_stop()` | Signal and wait for thread termination |
| `set_user_nice()` | Adjust thread scheduling priority |
| `ssleep()` | Sleep for `n` seconds (can be interrupted) |

## Author

Created by: fares.adham@gmail.com

## License

GPL (GNU General Public License)

---
