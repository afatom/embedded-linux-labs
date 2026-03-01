/**
 * This kernel module demonstrates the implementation and utilization of kernel threads
 * for concurrent processing. In this module, the threads do not perform any functional
 * tasks; they simply sleep and output messages to the ring buffer.
 * 
 * Author: fares.adham@gmail.com   
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>


#define NUM_KTHRDS  (2U)

struct kthrd_data {
    unsigned int thrd_idx;
    unsigned int sleep_time;
};

static const char *module_name="my_kthread_module";
static struct task_struct *kthreads_ts[NUM_KTHRDS];
static struct kthrd_data *kthrds_data;

static int thread_function(void *kthrd_data)
{
    struct kthrd_data *data = (struct kthrd_data *)kthrd_data;

    pr_info("%s: Kthread num %u started\n", module_name, data->thrd_idx);
    
    while (!kthread_should_stop()) {
        pr_info("%s: Kthread num %u sleeping for %u sec\n", module_name, data->thrd_idx,
            data->sleep_time);
            ssleep(data->sleep_time);
        }
        
    pr_info("%s: Kthread num %u stopped\n", module_name, data->thrd_idx);
    
    return 0;
}

static void __exit my_exit(void)
{    
    for (unsigned int i = 0; i < NUM_KTHRDS; i++) {
        if (kthreads_ts[i]) {
            kthread_stop(kthreads_ts[i]);
        }
    }

    kfree(kthrds_data);

    pr_info("%s: Module unloaded successfully\n", module_name);
}

static int __init my_init(void)
{
    int stat;
    unsigned int i;

    kthrds_data = kmalloc(NUM_KTHRDS * sizeof(*kthrds_data), GFP_KERNEL);
    if (!kthrds_data) {
        pr_err("%s: Failed to allocate threads data\n", module_name);
        return -ENOMEM;
    }

    for (unsigned int i = 0; i < NUM_KTHRDS; i++) {
        kthrds_data[i].thrd_idx = i + 1;
        kthrds_data[i].sleep_time = i + 1;
    }

    for (i = 0; i < NUM_KTHRDS; i++) {
        kthreads_ts[i] = kthread_run(thread_function, &kthrds_data[i], "my_kthread_num%u", i);
        if (IS_ERR(kthreads_ts[i])) {
            pr_err("%s: Failed to create kthread num %u\n", module_name, i);
            stat = PTR_ERR(kthreads_ts[i]);
            goto err_thrd_create;
        }
    }

    pr_info("%s: Module loaded successfully\n", module_name);

    return 0;

err_thrd_create:
    for (unsigned int j = 0; j < i; j++) {
        kthread_stop(kthreads_ts[j]);
    }
    kfree(kthrds_data);
    return stat;
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adham Faris <fares.adham@gmail.com>");
MODULE_DESCRIPTION("Kernel threads demonstration Example");