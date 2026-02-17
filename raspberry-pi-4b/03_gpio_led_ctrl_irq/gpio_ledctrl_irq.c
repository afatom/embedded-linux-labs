
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer_types.h>
#include <linux/module.h>
#include <linux/timer.h>

#define GPIO_CONS_INTERFACE_FRAMEWORK

#ifdef GPIO_CONS_INTERFACE_FRAMEWORK
#include <linux/gpio/consumer.h>
#else
#include <linux/gpio.h> // legacy framework
#endif


#define LED_GPIO_PIN 21U
#define BUTTON_GPIO_PIN 20U
#define GPIO_OFFSET 512U
#define LED_SET_OFF 0
#define LED_SET_ON 1
#define BTN_DEBOUNCE_DELAY 120 // 120 msec


static const char *device_name = "gpio_led_controller";
static int prev_button_state;
static struct timer_list debounce_timer;
static int led_gpio = (GPIO_OFFSET + LED_GPIO_PIN);
static int button_gpio = (GPIO_OFFSET + BUTTON_GPIO_PIN);
#ifdef GPIO_CONS_INTERFACE_FRAMEWORK
static struct gpio_desc *led, *button;
#endif
static int gpio_btn_irq_num;


static void debounce_timer_callback(struct timer_list *timer)
{
    #ifdef GPIO_CONS_INTERFACE_FRAMEWORK
    // read current GPIO button state
    int current_state = gpiod_get_value(button);
    #else
    int current_state = gpio_get_value(button_gpio);
    #endif

    // if the current state is the same as the previous state, means it is a valid press by the user
    // otherwise its a bouncing state
    if (current_state == prev_button_state) {
        // toggle the GPIO led
    #ifdef GPIO_CONS_INTERFACE_FRAMEWORK
        gpiod_set_value(led, !gpiod_get_value(led));
    #else
        gpio_set_value(led_gpio, !gpio_get_value(led_gpio));
    #endif
    }
}

static irqreturn_t gpio_button_isr(int irq, void *dev_id) {
    pr_info("%s: GPIO button IRQ\n", (char*)dev_id);
    // read the button state and save it to the prev_button_state
#ifdef GPIO_CONS_INTERFACE_FRAMEWORK
    prev_button_state = gpiod_get_value(button);
#else
    prev_button_state = gpio_get_value(button_gpio);
#endif
    // wait for button to settle and prevent false IRQs due to button bouncing
    mod_timer(&debounce_timer, jiffies + msecs_to_jiffies(BTN_DEBOUNCE_DELAY));

    return IRQ_HANDLED;
}

static int __init my_init(void)
{
    int stat;

    pr_info("GPIO controll ex driver Init\n");
#ifdef GPIO_CONS_INTERFACE_FRAMEWORK
    led = gpio_to_desc(led_gpio);
    if (!led) {
        pr_err("%s: Failed to get GPIO led desc\n", device_name);
        return -ENODEV;
    }
    
    button = gpio_to_desc(button_gpio);
    if (!button) {
        pr_err("%s: Failed to get GPIO button desc\n", device_name);
        return -ENODEV;
    }

    stat = gpiod_direction_output(led, LED_SET_OFF);
    if (stat)
        return stat;
        
    stat = gpiod_direction_input(button);
    if (stat)
        return stat;
    
    gpio_btn_irq_num = gpiod_to_irq(button);

    stat = request_irq(gpio_btn_irq_num, gpio_button_isr, IRQF_TRIGGER_FALLING, "gpio_button_irq", NULL);
    if (stat) {
        pr_err("%s: Failed to request IRQ for GPIO button %u\n", device_name, BUTTON_GPIO_PIN);
        return stat;
    }    
#else
    stat = gpio_request(led_gpio, "led_gpio");
    if (stat) {
        pr_err("%s: Failed to req led GPIO pin num %u\n", device_name, LED_GPIO_PIN);
        return stat;
    }

    gpio_direction_output(led_gpio, LED_SET_OFF);
    
    stat = gpio_request(button_gpio, "button_gpio");
    if (stat) {
        pr_err("%s: Failed to req button GPIO pin num %u\n", device_name, BUTTON_GPIO_PIN);
        goto err_free_led_gpio;
    }
    
    gpio_direction_input(button_gpio);
    
    gpio_btn_irq_num = gpio_to_irq(button_gpio);

    stat = request_irq(gpio_btn_irq_num, gpio_button_isr, IRQF_TRIGGER_FALLING, "gpio_button_irq", NULL);
    if (stat) {
        pr_err("%s: Failed to request IRQ for GPIO button %u\n", device_name, BUTTON_GPIO_PIN);
        goto err_free_button_gpio;
    }
#endif

    timer_setup(&debounce_timer, debounce_timer_callback, 0);
    pr_info("%s: GPIO controll ex driver, Button IRQ num=%d \n", device_name, gpio_btn_irq_num);
    pr_info("%s: GPIO controll ex driver loaded\n", device_name);

	return 0;

#ifndef GPIO_CONS_INTERFACE_FRAMEWORK
err_free_button_gpio:
    gpio_free(button_gpio);
err_free_led_gpio:
    gpio_free(led_gpio);

    return stat;
#endif
}

static void __exit my_exit(void)
{
#ifndef GPIO_CONS_INTERFACE_FRAMEWORK
    gpio_set_value(led_gpio, LED_SET_OFF);
    gpio_free(led_gpio);
    gpio_free(button_gpio);
#else
    gpiod_set_value(led, LED_SET_OFF);
#endif
    free_irq(gpio_btn_irq_num, NULL);
    del_timer_sync(&debounce_timer);
    pr_info("%s: GPIO dummy detached\n", device_name);
}

module_init(my_init);
module_exit(my_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("fares.adham@gmail.com");
MODULE_DESCRIPTION("A simple gpio ctrl dum Linux Kernel module");
