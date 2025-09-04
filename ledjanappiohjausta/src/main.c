//Tavoittelen parasta pistemäärää eli 3
//Onnistuin saamaan kaikki toimimaan

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <inttypes.h>


// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Red,green,blue led thread initialization
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

//Global state variable for the state machine.
// 0 = red, 1 = yellow, 2 = green
volatile int led_state = 0;
volatile int pause_state = 0;
volatile int button_pressed = 0;
volatile bool red_on = false;
volatile bool green_on = false;
volatile bool blue_on = false;
int init_led();
int init_buttons();
int button0handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
int button1handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
int button2handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
int button3handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
int button4handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)

// #define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});
static struct gpio_callback button_0_data;
static struct gpio_callback button_1_data;
static struct gpio_callback button_2_data;
static struct gpio_callback button_3_data;
static struct gpio_callback button_4_data;


// Main program
int main(void)
{
	init_led();
	init_buttons();

	return 0;
}

// Initialize leds
int  init_led() {
	int ret;

	// Led pin initialization
	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);

	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&blue,0);

	printk("Led initialized ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
		if (led_state == 0) {
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		printk("Red on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		printk("Red off\n");
		// 4. sleep for 2 seconds
		k_sleep(K_SECONDS(1));

		if(led_state != 4) {
		led_state = 1;
		}
		}
		k_sleep(K_SECONDS(1));
	}
}

// Task to handle red led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		if(led_state == 1) {
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		printk("Yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		printk("Yellow off\n");
		// 4. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		if(led_state != 4) {
		led_state = 2;
		}
	}
	if (led_state == 5) {
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		printk("Yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		printk("Yellow off\n");
		}
		k_sleep(K_SECONDS(1));
	}
}

// Task to handle red led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		if(led_state == 2) {
		// 1. set led on 
		gpio_pin_set_dt(&green,1);
		printk("Green on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&green,0);
		printk("Green off\n");
		// 4. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		if(led_state != 4) {
		led_state = 0;
		}
		}
		k_sleep(K_SECONDS(1));
	}
}
// Button 0 interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 0 pressed\n");
	if(led_state != 4) {
		pause_state = led_state;
		led_state = 4;
	} 
	else {
		led_state = pause_state;
	}
}
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	 printk("Button 1 pressed\n");
    if (red_on) {
		//set led off
        gpio_pin_set_dt(&red, 0);
        red_on = false;
    } else {
		//set led on
        gpio_pin_set_dt(&red, 1);
        red_on = true;
    }
}

void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 2 pressed\n");
	if (red_on && green_on) {
		//set led off
        gpio_pin_set_dt(&red, 0);
		gpio_pin_set_dt(&green, 0);
        red_on = false;
		green_on = false;
    } else {
		//set led on
        gpio_pin_set_dt(&red, 1);
		gpio_pin_set_dt(&green, 1);
        red_on = true;
		green_on = true;
    }
}
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 3 pressed\n");
	if (green_on) {
		//set led off
        gpio_pin_set_dt(&green, 0);
        green_on = false;
    } else {
		//set led on
        gpio_pin_set_dt(&green, 1);
        green_on = true;
    }
}
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 4 pressed\n");
	if(led_state == 4) {
		led_state = 5;
	}
	else if (led_state == 5){
		led_state = 0;
	} 
	else {
		pause_state = led_state;
	}
	
}

// Button initialization
int init_buttons() {
    int r;

    // Check if all buttons are ready
    if (!gpio_is_ready_dt(&button_0) || !gpio_is_ready_dt(&button_1) || !gpio_is_ready_dt(&button_2) || !gpio_is_ready_dt(&button_3) || !gpio_is_ready_dt(&button_4)) {
        printk("Error: a button is not ready\n");
        return -1;
    }

    // Configure button pins as input
    r = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    if (r != 0) {
        printk("Error: failed to configure pin for button 0\n");
        return -1;
    }
    r = gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    if (r != 0) {
        printk("Error: failed to configure pin for button 1\n");
        return -1;
    }
    r = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    if (r != 0) {
        printk("Error: failed to configure pin for button 2\n");
        return -1;
    }
    r = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
    if (r != 0) {
        printk("Error: failed to configure pin for button 3\n");
        return -1;
    }
    r = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
    if (r != 0) {
        printk("Error: failed to configure pin for button 4\n");
        return -1;
    }
    
    // Configure interrupts for all buttons
    r = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    if (r != 0) {
        printk("Error: failed to configure interrupt on pin for button 0\n");
        return -1;
    }
    r = gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    if (r != 0) {
        printk("Error: failed to configure interrupt on pin for button 1\n");
        return -1;
    }
    r = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    if (r != 0) {
        printk("Error: failed to configure interrupt on pin for button 2\n");
        return -1;
    }
    r = gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
    if (r != 0) {
        printk("Error: failed to configure interrupt on pin for button 3\n");
        return -1;
    }
    r = gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);
    if (r != 0) {
        printk("Error: failed to configure interrupt on pin for button 4\n");
        return -1;
    }

    // Initialize and add callbacks for all buttons
    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_data);

    gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_data);

    gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_data);

    gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_data);

    printk("Set up all buttons ok\n");
    return 0;
}
