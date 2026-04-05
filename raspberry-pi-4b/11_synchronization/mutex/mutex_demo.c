#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#define HIGH_PRIO_NICE_VAL -20
#define LOW_PRIO_NICE_VAL -19

static const char *module_name = "mutex_demo";

static struct task_struct *hiprio_thrd;
static struct task_struct *loprio_thrd;
static char buf[64];

static struct mutex mtx;

struct task_data {
	int value;
	char name[10];
	char text[32];
};

struct task_data hiprio_thrd_data = {
	.value = 5,
	.name = "HIPRIO",
	.text = "Hello from High Priority thread"
};

struct task_data loprio_thrd_data = { .value = 2,
				      .name = "LOPRIO",
				      .text = "Hello from Low Priority thread" };

static int thread_fn(void *data)
{
	struct task_data *t_data = (struct task_data *)data;

	while (!kthread_should_stop()) {
		mutex_lock(&mtx);

		pr_info("%s: Running %s Priority Task, Copying: %s \n",
			module_name, t_data->name, t_data->text);
		// Write to shared buffer
		snprintf(buf, sizeof(buf), "Active: %s", t_data->text);
		pr_info("%s: Enter %s Shared Buffer: %s\n", module_name,
			t_data->name, buf);

		ssleep(t_data->value);

		pr_info("%s: Exit %s Shared Buffer: %s\n", module_name,
			t_data->name, buf);

		mutex_unlock(&mtx);
	}
	return 0;
}

static int __init my_init(void)
{
	pr_info("%s: Initializing threads in SCHED_NORMAL\n", module_name);

	mutex_init(&mtx);

	hiprio_thrd =
		kthread_run(thread_fn, &hiprio_thrd_data, "high_prio_kthread");
	if (IS_ERR(hiprio_thrd)) {
		pr_err("%s: Failed to create hiprio_thrd\n", module_name);
		return PTR_ERR(hiprio_thrd);
	}

	set_user_nice(hiprio_thrd, HIGH_PRIO_NICE_VAL);

	loprio_thrd =
		kthread_run(thread_fn, &loprio_thrd_data, "low_prio_kthread");
	if (IS_ERR(loprio_thrd)) {
		pr_err("%s: Failed to create loprio_thrd\n", module_name);
		kthread_stop(hiprio_thrd);
		return PTR_ERR(loprio_thrd);
	}

	set_user_nice(loprio_thrd, LOW_PRIO_NICE_VAL);

	return 0;
}

static void __exit my_exit(void)
{
	if (hiprio_thrd)
		kthread_stop(hiprio_thrd);

	if (loprio_thrd)
		kthread_stop(loprio_thrd);

	mutex_destroy(&mtx);

	pr_info("%s: Threads stopped\n", module_name);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION(
	"Kernel Synchronization: Mutex Demo - Two threads with different priorities accessing shared resource");
