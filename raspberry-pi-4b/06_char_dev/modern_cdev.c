#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/slab.h>


static const char *my_cdev_name = "my_cdev";
static const char *my_cls_name = "my_cdev_class";

// static dev_t device_nr; //Holds the device major and minor
// static struct cdev my_cdev;
// struct device *my_cdev_dev;
// struct class *my_cls;
// static unsigned int device_baseminor = 0;
// static unsigned int num_devices = MINORMASK + 1;

static struct file_operations my_cdev_ops = {

};

struct cdev_private_data {
    struct cdev cdev;
    struct device *dev;
    struct class *cls;
    struct file_operations *cdev_fops;
    dev_t device_nr;
    unsigned int device_baseminor;
    unsigned int num_devices;
};

struct cdev_private_data *cdev_privdata;


static int __init my_cdev_init(void)
{
    int stat;

    cdev_privdata = kmalloc(sizeof(*cdev_privdata), GFP_KERNEL);
    if (!cdev_privdata) {
        pr_err("%s: char device init err (kmalloc)\n", my_cdev_name);
        return -ENOMEM;
    }

    cdev_privdata->cdev_fops = &my_cdev_ops;
    // Registers a range of char device numbers (dynamically allocated by the Kernel)
    stat = alloc_chrdev_region(&cdev_privdata->device_nr, cdev_privdata->device_baseminor,
                               cdev_privdata->num_devices, my_cdev_name);
    if (stat) {
        pr_err("%s: char device registeration failed with err %d\n", my_cdev_name, stat);
        return stat;
    }

    cdev_init(&cdev_privdata->cdev, cdev_privdata->cdev_fops);
    cdev_privdata->cdev.owner = THIS_MODULE; //prevent module from unloading while a device is open
    
    stat = cdev_add(&cdev_privdata->cdev, cdev_privdata->device_nr, cdev_privdata->num_devices);
    if (stat) {
        pr_err("%s: char device add failed with err %d\n", my_cdev_name, stat);
        goto cdev_add_err;
    }

    /**
     * Use udev kernel subsystem to add a device node in the /dev rather than adding it manually
     * in userland with mknod command. It creates a device and registers it with sysfs
     */
    cdev_privdata->cls = class_create(my_cls_name);
    if (IS_ERR_OR_NULL(cdev_privdata->cls)) {
        stat = PTR_ERR_OR_ZERO(cdev_privdata->cls);
        pr_err("%s: class creation failed with err %d\n", my_cdev_name, stat);
        goto class_create_err;
    }

    // No parent device for my_cdev device, Hence pass NULL
    cdev_privdata->dev = device_create(cdev_privdata->cls, NULL, cdev_privdata->device_nr,
                                       cdev_privdata, "my_cdev%d", 0);
    if (IS_ERR_OR_NULL(cdev_privdata->dev)) {
        stat = PTR_ERR_OR_ZERO(cdev_privdata->dev);
        pr_err("%s: device creation failed with err %d\n", my_cdev_name, stat);
        goto device_create_err;
    }

	pr_info("%s: char device registeration success, major %d, minor %d\n", my_cdev_name,
            MAJOR(cdev_privdata->device_nr), MINOR(cdev_privdata->device_nr));

    return 0;


device_create_err:
    class_destroy(cdev_privdata->cls);

class_create_err:
    cdev_del(&cdev_privdata->cdev);

cdev_add_err:
    unregister_chrdev_region(cdev_privdata->device_nr, cdev_privdata->num_devices); // unregister range of @count device numbers
    kfree(cdev_privdata);

    return stat;
}

static void __exit my_cdev_exit(void)
{
    device_destroy(cdev_privdata->cls, cdev_privdata->device_nr);
    class_destroy(cdev_privdata->cls);
    // remove a char device from the system
    cdev_del(&cdev_privdata->cdev);
    unregister_chrdev_region(cdev_privdata->device_nr, cdev_privdata->num_devices);
    kfree(cdev_privdata);
	pr_info("%s: cdev removed and unregistered\n", my_cdev_name);
}


module_init(my_cdev_init);
module_exit(my_cdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("My dummy Char device");
