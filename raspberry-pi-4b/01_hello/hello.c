#include <linux/module.h>
#include <linux/init.h>

/* Add functions prototypes silence the gcc compiler warnings [-Wmissing-prototypes] */
static int hello_driver_init(void);
static void hello_driver_exit(void);

static int __init hello_driver_init(void)
{
	printk("Hello from a dummy char device driver\n");
	return 0;
}

static void __exit hello_driver_exit(void)
{
	printk("Goodbye from a dummy char device driver\n");
}

module_init(hello_driver_init);
module_exit(hello_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("Hello world Linux Kernel module");
