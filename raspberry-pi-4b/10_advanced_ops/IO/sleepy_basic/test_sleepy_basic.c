#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define DEVICE_PATH "/dev/sleepy_cdev0"
#define NUM_READERS 1
#define NUM_WRITERS 1
#define NUM_TOTAL_CHILDREN (NUM_READERS + NUM_WRITERS)

int main(int argc, char *argv[])
{
	int i;

	printf("[Parent] Starting the Kernel Wait Queue test on %s\n",
	       DEVICE_PATH);

	// 1. Create 2 Reader Processes
	for (i = 1; i <= NUM_READERS; i++) {
		pid_t pid = fork();

		if (pid < 0) {
			perror("Fork failed");
			return 1;
		}

		if (pid == 0) {
			// CHILD PROCESS: READER
			int fd = open(DEVICE_PATH, O_RDONLY);
			if (fd < 0) {
				perror("Child failed to open device for reading");
				exit(1);
			}

			printf("[Reader %d] PID %d: Calling read() and entering Wait Queue...\n",
			       i, getpid());

			char buffer[1];
			// This call will BLOCK until the writer process writes to the device and wakes it up'
			ssize_t bytes = read(fd, buffer, 0);

			printf("[Reader %d] PID %d: Woke up and exiting!\n", i,
			       getpid());

			close(fd);
			exit(0);
		}
	}

	// Give the readers a moment to ensure they reach the 'wait_queue' in the kernel
	sleep(2);

	// 2. Create the Writer Process (The Waker)
	pid_t pid_waker = fork();
	if (pid_waker == 0) {
		// CHILD PROCESS: WRITER
		int fd = open(DEVICE_PATH, O_WRONLY);
		if (fd < 0) {
			perror("Waker failed to open device for writing");
			exit(1);
		}

		printf("[Waker] PID %d: Writing to device to trigger wake_up_interruptible()...\n",
		       getpid());

		const char *msg = "wake";
		// This call enters your 'my_write', sets flag=1, and calls wake_up
		write(fd, msg, strlen(msg));

		close(fd);
		exit(0);
	}

	// 3. Parent waits for all 3 children to finish
	printf("[Parent] All children dispatched. Waiting for synchronization...\n");
	for (i = 0; i < NUM_TOTAL_CHILDREN; i++) {
		wait(NULL);
	}

	printf("[Parent] Test complete. All processes terminated.\n");
	return 0;
}
