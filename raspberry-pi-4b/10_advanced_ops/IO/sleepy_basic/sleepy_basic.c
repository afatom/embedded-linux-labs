#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/wait.h>

static const char *my_dev_name = "sleepy_cdev";

static dev_t dev_nr;
static struct cdev my_cdev;
static struct class *my_class;
static int flag = 0;

static DECLARE_WAIT_QUEUE_HEAD(wq);

static ssize_t my_read(struct file *filep, char __user *buf, size_t count,
		       loff_t *pos)
{
	pr_info("%s: process %i (%s) going to sleep\n", my_dev_name,
		current->pid, current->comm);
	wait_event_interruptible(wq, flag != 0);
	flag = 0;
	pr_info("%s: awoken %i (%s)\n", my_dev_name, current->pid,
		current->comm);
	return 0; /* EOF */
}

static ssize_t my_write(struct file *filep, const char __user *buf,
			size_t count, loff_t *pos)
{
	pr_info("%s: process %i (%s) awakening the readers...\n", my_dev_name,
		current->pid, current->comm);
	flag = 1;
	wake_up_interruptible(&wq);
	return count; /* succeed, to avoid retrial */
}

static int my_open(struct inode *inode, struct file *file)
{
	pr_info("%s: device opened\n", my_dev_name);
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	pr_info("%s: device closed\n", my_dev_name);
	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.read = my_read,
	.write = my_write,
};

static int __init my_init(void)
{
	int status;
	status = alloc_chrdev_region(&dev_nr, 0, MINORMASK + 1, my_dev_name);
	if (status) {
		pr_err("%s: Character device registration failed\n",
		       my_dev_name);
		return status;
	}

	cdev_init(&my_cdev, &fops);

	status = cdev_add(&my_cdev, dev_nr, MINORMASK + 1);
	if (status) {
		pr_err("%s: cdev_add failed\n", my_dev_name);
		goto err_free_device_nr;
	}

	my_class = class_create("my_class");
	if (!my_class) {
		pr_err("%s:  class_create failed\n", my_dev_name);
		status = ENOMEM;
		goto err_delete_cdev;
	}

	if (!device_create(my_class, NULL, dev_nr, NULL, "%s%d", my_dev_name,
			   0)) {
		pr_err("%s: device_create failed\n", my_dev_name);
		status = ENOMEM;
		goto err_delete_class;
	}

	pr_info("%s: Caracter device registerd, Major number: %d Minor number: %d\n",
		my_dev_name, MAJOR(dev_nr), MINOR(dev_nr));
	pr_info("%s: Created device number under /sys/class/my_class\n",
		my_dev_name);
	pr_info("%s: Created new device node /dev/my_cdev0\n", my_dev_name);

	return 0;

err_delete_class:
	class_destroy(my_class);
err_delete_cdev:
	cdev_del(&my_cdev);
err_free_device_nr:
	unregister_chrdev_region(dev_nr, MINORMASK + 1);
	return status;
}

static void __exit my_exit(void)
{
	device_destroy(my_class, dev_nr);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_nr, MINORMASK + 1);
	pr_info("%s: Module unloaded\n", my_dev_name);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION(
	"Sleepy Basic, Blocking/Non-blocking IO: read and wakeup on write module");
