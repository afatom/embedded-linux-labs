---

# Linux Kernel Driver: Blocking I/O & Wait Queues

This repository demonstrates how a Linux character device driver handles **Blocking I/O**. It explores the mechanisms used to manage scenarios where a hardware resource is not immediately available, such as reading from an empty buffer or writing to a busy device.

## Table of Contents

* [Objective](https://www.google.com/search?q=%23objective)
* [Core Concepts](https://www.google.com/search?q=%23core-concepts)
* [Wait Queues: The Mechanics of Sleeping](https://www.google.com/search?q=%23wait-queues-the-mechanics-of-sleeping)
* [Included Modules](https://www.google.com/search?q=%23included-modules)
* [Test Flow: sleepy_basic](https://www.google.com/search?q=%23test-flow-sleepy_basic)

---

## Objective

The goal of this chapter is to cover how a character device driver should respond to read/write requests that cannot be immediately satisfied.

## Core Concepts

* 
**Blocking I/O:** The driver is expected to block or put the calling process into a sleep state.


* 
**Non-Blocking I/O:** The driver returns immediately to the user with the respected `errno` (e.g., `-EAGAIN`).


* 
**Sleep State:** A process is removed from the scheduler run queue and will not be scheduled on any CPU until its state is changed.



### Rules for Sleeping

1. 
**No Atomic Context:** A process should not sleep if it is running in an atomic context (e.g., holding RCU, spin, or seq locks).


2. 
**Post-Wake Assumptions:** You must always check the condition you were waiting for after waking up, as you cannot make assumptions about the system state.


3. 
**The Guarantee:** You cannot sleep unless it is assured that "somebody" else will wake you up later and that the process can be found to be awakened.



## Wait Queues: The Mechanics of Sleeping

Making it possible for a sleeping process to be found is accomplished through a data structure called a **wait queue**.

Implementation Details 

**Declaring a Wait Queue:**

* **Static:** `DECLARE_WAIT_QUEUE_HEAD(name);`
* **Dynamic:** ```c
wait_queue_head_t my_queue;
init_waitqueue_head(&my_queue);
```


```



**Sleeping Macros:**

* `wait_event(queue, condition)`
* `wait_event_interruptible(queue, condition)`
* `wait_event_timeout(queue, condition, timeout)`
* `wait_event_interruptible_timeout(queue, condition, timeout)`

**Waking Functions:**

* `void wake_up(wait_queue_head_t *queue);`
* 
`void wake_up_interruptible(wait_queue_head_t *queue);` 



---

## Included Modules

### 1. `sleepy_basic` (`sleepy_cdev`)

Implements a simple behavior where any process attempting to read is put to sleep. When a process writes to the device, all sleeping processes are awakened.

### 2. `sleepy_advanced`

Implements advanced behavior where a process is put to sleep if it attempts to read while the device buffer is empty. Conversely, an attempt to write when there is no space left in the buffer will put the writing process to sleep.

---

## Test Flow: `sleepy_basic`

### Step 1: Building and Loading

```bash
# Build the module
$ make all

# Load the module
$ sudo insmod sleepy_basic.ko

```

**Kernel Log Confirmation:**

```text
[769968.158348] sleepy_cdev: Character device registered, Major number: 236 Minor number: 0
[769968.158363] sleepy_cdev: Created device number under /sys/class/my_class
[769968.158367] sleepy_cdev: Created new device node /dev/my_cdev0

```

### Step 2: Running the Test Application

```bash
# Compile and run the synchronization test
$ gcc test_sleepy_basic.c -o ksync_test && sudo ./ksync_test

```

**Console Output:**

```text
[Parent] Starting the Kernel Wait Queue test on /dev/my_cdev0
[Reader 1] PID 102883: Calling read() and entering Wait Queue...
[Parent] All children dispatched. [cite_start]Waiting for synchronization... [cite: 19]
[Waker] PID 102910: Writing to device to trigger wake_up_interruptible()...
[Reader 1] PID 102883: Woke up and exiting!
[Parent] Test complete. [cite_start]All processes terminated. [cite: 20]

```

**Kernel Log (`dmesg`):**

```text
[769978.184273] sleepy_cdev: device opened
[769978.184361] sleepy_cdev: process 102883 (ksync_test) going to sleep
[769980.184589] sleepy_cdev: device opened
[769980.184695] sleepy_cdev: process 102910 (ksync_test) awakening the readers...
[769980.184715] sleepy_cdev: device closed
[769980.184733] sleepy_cdev: awoken 102883 (ksync_test)
[769980.184833] sleepy_cdev: device closed

```

---

**Would you like me to help you draft the `sleepy_advanced` section with a similar test flow for the buffer-handling logic?**
