#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>


#define RED_LED_GPIO_PIN 12U
#define BLUE_LED_GPIO_PIN 16U
#define YELLOW_LED_GPIO_PIN 21U

#define BUTTON_GPIO_PIN 20U
#define GPIO_OFFSET 512U
#define LED_SET_OFF 0
#define LED_SET_ON 1

#define RED_LED_IDX 0
#define BLUE_LED_IDX 1
#define YELLOW_LED_IDX 2
#define NUM_LEDS 3U

static const char *device_name = "gpio_led_controller_wq";

static const char *led_colors[NUM_LEDS] = {
    "red",
    "blue",
    "yellow"
};

static int led_gpios[NUM_LEDS] = {
    (GPIO_OFFSET + RED_LED_GPIO_PIN),
    (GPIO_OFFSET + BLUE_LED_GPIO_PIN),
    (GPIO_OFFSET + YELLOW_LED_GPIO_PIN)
};

static struct gpio_desc *led_gpio_descs[NUM_LEDS];
static struct gpio_desc *button;
static struct work_struct button_work;
static int button_gpio = (GPIO_OFFSET + BUTTON_GPIO_PIN);
static int gpio_btn_irq_num;


static inline void gpio_toggle_led(struct gpio_desc *led)
{
    gpiod_set_value(led, !gpiod_get_value(led));
}

// Bottom-half GPIO button ISR (workqueue)
static void button_work_handler(struct work_struct *work)
{
    pr_info("%s: Bottom-half, processing button press\n", device_name);

    msleep(500);
    gpio_toggle_led(led_gpio_descs[RED_LED_IDX]);
    
    msleep(500);
    gpio_toggle_led(led_gpio_descs[BLUE_LED_IDX]);
    
    msleep(500);
    gpio_toggle_led(led_gpio_descs[YELLOW_LED_IDX]);
}

// Top-half GPIO button ISR (schedule a work with workqueue)
static irqreturn_t gpio_button_isr(int irq, void *dev_id) {
    pr_info("%s: GPIO button IRQ\n", (char*)dev_id);

    schedule_work(&button_work);

    return IRQ_HANDLED;
}

static int __init my_init(void)
{
    int stat;

    pr_info("GPIO LED controller via workqueue driver Init\n");

    for (unsigned i = 0; i < NUM_LEDS; i++) {
        led_gpio_descs[i] = gpio_to_desc(led_gpios[i]);
        if (!led_gpio_descs[i]) {
            pr_err("%s: Failed to get GPIO %s led desc\n", device_name, led_colors[i]);
            return -ENODEV;
        }
    }

    button = gpio_to_desc(button_gpio);
    if (!button) {
        pr_err("%s: Failed to get GPIO button desc\n", device_name);
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
    
    gpio_btn_irq_num = gpiod_to_irq(button);

    stat = request_irq(gpio_btn_irq_num, gpio_button_isr, IRQF_TRIGGER_FALLING, "gpio_button_irq", NULL);
    if (stat) {
        pr_err("%s: Failed to request IRQ for GPIO button %u\n", device_name, BUTTON_GPIO_PIN);
        return stat;
    }    

    INIT_WORK(&button_work, button_work_handler);
    pr_info("%s: GPIO controll ex driver, Button IRQ num=%d \n", device_name, gpio_btn_irq_num);
    pr_info("%s: GPIO controll ex driver loaded\n", device_name);

	return 0;
}

static void __exit my_exit(void)
{
    for (unsigned i = 0; i < NUM_LEDS; i++)
        gpiod_set_value(led_gpio_descs[i], LED_SET_OFF);

    free_irq(gpio_btn_irq_num, NULL);
    cancel_work_sync(&button_work); //cancel all scheduled work before exiting
    pr_info("%s: GPIO dummy detached\n", device_name);
}

module_init(my_init);
module_exit(my_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("GPIO Kernel Module for Managing Three LEDs Using Kernel Workqueues");
