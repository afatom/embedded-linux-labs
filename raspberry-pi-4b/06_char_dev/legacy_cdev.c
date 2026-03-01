#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#define MY_CDEV_MAJOR 85210U
static const char *dev_name = "my_cdev";
static unsigned int cdev_major;

static struct file_operations my_cdev_ops = {

};


static int __init cdev_init(void)
{
    int stat;

    /**
     * Its not recomended to assign manualy the char device major, since it can
     * collide with an existing char dev major leading to device error. Its recomended
     * to use alloc_chrdev_region() API, since it prevents conflicts with other devices
     * by selecting a free, dynamic major number.
     */ 
    stat = register_chrdev(0, dev_name, &my_cdev_ops);
    if (stat < 0) {
        pr_err("%s: char device registeration failed\n", dev_name);
        return stat;
    }

    cdev_major = stat;

	pr_info("%s: char device registeration success, major num %u\n", dev_name, cdev_major);
	return 0;
}

static void __exit cdev_exit(void)
{
    unregister_chrdev(cdev_major, dev_name);
	pr_info("%s: char device unregister\n", dev_name);
}


module_init(cdev_init);
module_exit(cdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("My dummy Char device");
