/**
 * This project is a classic "Kernel Concurrency Lab." It uses a button to trigger different
 * ways of handling work in the kernel, visualising them with your three LEDs.
 * 
 * This tiny Project: "The Interrupt Multiplier"
 * The goal is to see how different kernel mechanisms handle the same event (a button press)
 * with different constraints.
 * 
 * 1) GPIO 20   (Button): Acts as the Top Half. It triggers a hardware interrupt.
 * 2) GPIO 21   (LED 1 - Tasklet): Lights up immediately when the Bottom Half (Tasklet) starts.
 *              It proves that small, atomic tasks can run very fast after an interrupt.
 * 3) GPIO 16   (LED 2 - Workqueue): Lights up after the Tasklet, demonstrating work that is
 *              allowed to sleep or wait for the system to be ready.
 * 4) GPIO 12   (LED 3 - Kernel Thread): This LED will blink continuously in the background,
 *              showing a persistent "daemon" process that lives only in the kernel.
 * 
 * Author: fares.adham@gmail.com   
 */

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>


#define RED_LED_GPIO_PIN        12U
#define BLUE_LED_GPIO_PIN       16U
#define YELLOW_LED_GPIO_PIN     21U
#define BUTTON_GPIO_PIN         20U
#define GPIO_OFFSET             512U
#define LED_SET_OFF             0
#define LED_SET_ON              1
#define RED_LED_IDX             0
#define BLUE_LED_IDX            1
#define YELLOW_LED_IDX          2
#define NUM_LEDS                3U
#define LED_1_IDX               0
#define LED_2_IDX               1
#define LED_3_IDX               2
#define BTN_DEBOUNCE            30 //msec


struct kthread_data {
    unsigned int thread_uid;
    unsigned int sleep_time;
};

static const char *led_colors[NUM_LEDS] = {
    "red",
    "blue",
    "yellow"
};

static const char *module_name="interrupt_multiplier";
static const char *wq_name="int_multiplier_wq";

static struct gpio_desc *led_gpio_descs[NUM_LEDS];
static struct gpio_desc *button;
static int button_gpio = (GPIO_OFFSET + BUTTON_GPIO_PIN);
static struct task_struct *thread_ts;
static int gpio_btn_irq_num;
static struct workqueue_struct *wq;
static struct work_struct button_work;
static struct kthread_data my_thread_data = {
    .thread_uid = 1,
    .sleep_time = 2
};

static int led_gpios[NUM_LEDS] = {
    (GPIO_OFFSET + RED_LED_GPIO_PIN),
    (GPIO_OFFSET + BLUE_LED_GPIO_PIN),
    (GPIO_OFFSET + YELLOW_LED_GPIO_PIN)
};
int gpiod_set_debounce(struct gpio_desc *desc, unsigned int debounce);
static inline void gpio_toggle_led(struct gpio_desc *led);
static int thread_function(void *data);
void my_tasklet_handler(struct tasklet_struct *t);

static int thread_function(void *data)
{
    struct kthread_data *thrd_data = (struct kthread_data *)data;

    pr_info("%s: LED toggler thread (UID %u) started\n", module_name, thrd_data->thread_uid);
    
    while (!kthread_should_stop()) {
        gpio_toggle_led(led_gpio_descs[LED_3_IDX]);
        ssleep(thrd_data->sleep_time);
    }
    
    pr_info("%s: LED toggler thread (UID %u) Stopped\n", module_name, thrd_data->thread_uid);

    return 0;
}

// Bottom-half GPIO button ISR (workqueue)
static void button_work_handler(struct work_struct *work)
{
    pr_info("%s: GPIO button 'ISR' (Bottom-half), processing button press\n", module_name);

    msleep(250);
    gpio_toggle_led(led_gpio_descs[LED_2_IDX]);
}

/* 1. TASKLET (Atomic Bottom Half) */
void my_tasklet_handler(struct tasklet_struct *t) {
    pr_info("%s: Task let, Toggle led 1\n", module_name);
    gpio_toggle_led(led_gpio_descs[LED_1_IDX]);
    mdelay(50); // Small delay to visualize, but normally tasklets must be FAST
    gpio_toggle_led(led_gpio_descs[LED_1_IDX]);
}
DECLARE_TASKLET(my_tasklet, my_tasklet_handler);

// Top-half GPIO button ISR (schedule a work with workqueue)
static irqreturn_t gpio_button_isr(int irq, void *dev_id) {
    pr_info("%s: GPIO button ISR (Top-half)\n", (char*)dev_id);
    tasklet_schedule(&my_tasklet);
    // schedule_work(&button_work);
    queue_work(wq, &button_work);

    return IRQ_HANDLED;
}

static inline void gpio_toggle_led(struct gpio_desc *led)
{
    gpiod_set_value(led, !gpiod_get_value(led));
}


static int __init interrupt_multiplier_init(void)
{
    int stat;

    pr_info("%s: Init\n", module_name);

    for (unsigned i = 0; i < NUM_LEDS; i++) {
        led_gpio_descs[i] = gpio_to_desc(led_gpios[i]);
        if (!led_gpio_descs[i]) {
            pr_err("%s: Failed to get GPIO %s led desc\n", module_name, led_colors[i]);
            return -ENODEV;
        }
    }

    button = gpio_to_desc(button_gpio);
    if (!button) {
        pr_err("%s: Failed to get GPIO button desc\n", module_name);
        return -ENODEV;
    }

    for (unsigned i = 0; i < NUM_LEDS; i++) {
        stat = gpiod_direction_output(led_gpio_descs[i], LED_SET_OFF);
        if (stat)
            return stat;
    }

    stat = gpiod_direction_input(button);
    if (stat)
        return stat;
    
    // stat = gpiod_set_debounce(button, BTN_DEBOUNCE * 1000);
    // if (stat) {
    //     pr_err("%s: Failed to GPIO set debounce (err %d)\n", module_name, stat);
    //     return stat;
    // }

    gpio_btn_irq_num = gpiod_to_irq(button);
    
    stat = request_irq(gpio_btn_irq_num, gpio_button_isr, IRQF_TRIGGER_FALLING, "gpio_button_irq", NULL);
    if (stat) {
        pr_err("%s: Failed to request IRQ for GPIO button %u\n", module_name, BUTTON_GPIO_PIN);
        return stat;
    }
    
    INIT_WORK(&button_work, button_work_handler);
    wq = create_singlethread_workqueue(wq_name);
    if (!wq) {
        pr_err("%s: Failed to create workqueue\n", module_name);
        goto err_free_irq_request;
    }

    thread_ts = kthread_run(thread_function, &my_thread_data, "led3_toggler_kthread");
    if (IS_ERR(thread_ts)) {
        pr_err("%s: Failed to create kthread\n", module_name);
        stat = PTR_ERR(thread_ts);
        goto err_destroy_workqueue;
    }

    pr_info("%s: interrupt multiplier module, loaded\n", module_name);

	return 0;


err_destroy_workqueue:
    destroy_workqueue(wq);
err_free_irq_request:
	// cancel_work_sync(&button_work);
    free_irq(gpio_btn_irq_num, NULL);
    return -ENOMEM;
}

int gpiod_set_debounce(struct gpio_desc *desc, unsigned int debounce)
{
	unsigned long config;

	config = pinconf_to_config_packed(PIN_CONFIG_INPUT_DEBOUNCE, debounce);
	return gpiod_set_config(desc, config);
}

static void __exit interrupt_multiplier_exit(void)
{
    kthread_stop(thread_ts);
    for (unsigned i = 0; i < NUM_LEDS; i++)
        gpiod_set_value(led_gpio_descs[i], LED_SET_OFF);
    free_irq(gpio_btn_irq_num, NULL);
    cancel_work_sync(&button_work);
    destroy_workqueue(wq);
    tasklet_disable(&my_tasklet);
    tasklet_kill(&my_tasklet);
    pr_info("%s: Module unloaded successfully\n", module_name);
}

module_init(interrupt_multiplier_init);
module_exit(interrupt_multiplier_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("GPIO Kernel Module for Managing Three LEDs Using Kernel Workqueues");

