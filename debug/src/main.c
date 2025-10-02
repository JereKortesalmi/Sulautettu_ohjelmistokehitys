// Jere Kortesalmi
// Tavoite: 3 pistettä ja toimii
// Debug printeillä ja ilman readme.tiedostossa , Debug task ja debug päälle ja pois 

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/timing/timing.h>

#define STACKSIZE 500
#define PRIORITY 5

// UART
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// LED pins
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Buttons
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)
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

K_FIFO_DEFINE(data_fifo);


struct data_t {
	void *fifo_reserved;
	char seq[10];
	int len;
	uint64_t time;
};

int init_uart(void){
	if (!device_is_ready(uart_dev)){
		return 1;
	}
	return 0;
}

K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(yellow_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(release_sem, 0, 1);
K_SEM_DEFINE(debug_sem, 0, 1);

// Thread prototypes
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void uart_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void debug_task(void *, void *, void*);

// Threads
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(debug_thread,STACKSIZE,debug_task,NULL,NULL,NULL,PRIORITY,0,0); 


void button_add_char(char c){
	struct data_t *item = k_malloc(sizeof(struct data_t));
	if(item){
		item->seq[0] = c;
		item->len = 1;
		item->time = k_uptime_get();
		k_fifo_put(&data_fifo, item);
		//printk("Lisättiin sekvenssiin: %c\n", c);
	}
}

// Button-handlers
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){ button_add_char('R'); }
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){ button_add_char('Y'); }
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){ button_add_char('G'); }
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){ button_add_char('D'); }
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){ button_add_char('F'); }

// Button initialization
int init_buttons() {
    const struct gpio_dt_spec buttons[] = {button_0, button_1, button_2, button_3, button_4};
    struct gpio_callback *callbacks[] = {&button_0_data, &button_1_data, &button_2_data, &button_3_data, &button_4_data};
    void (*handlers[])(const struct device *, struct gpio_callback *, uint32_t) = {button_0_handler, button_1_handler, button_2_handler, button_3_handler, button_4_handler};

    for(int i=0;i<5;i++){
        if(!gpio_is_ready_dt(&buttons[i])){
            printk("Button %d not ready\n", i);
            return -1;
        }
        if(gpio_pin_configure_dt(&buttons[i], GPIO_INPUT) != 0){
            printk("Button %d config failed\n", i);
            return -1;
        }
        if(gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE) != 0){
            printk("Button %d interrupt failed\n", i);
            return -1;
        }
        gpio_init_callback(callbacks[i], handlers[i], BIT(buttons[i].pin));
        gpio_add_callback(buttons[i].port, callbacks[i]);
    }
    printk("All buttons initialized\n");
    return 0;
}

// Red LED task
void red_led_task(void *, void *, void *)
{
	while(true){
		timing_start();
		timing_t start_time = timing_counter_get();

		k_sem_take(&red_sem, K_FOREVER);
		gpio_pin_set_dt(&red, 1);
		//printk("Red on\n");
        k_sleep(K_MSEC(1000));
        gpio_pin_set_dt(&red, 0);
		//printk("Red off\n");
		k_sem_give(&release_sem);

		struct data_t *buf = k_malloc(sizeof(struct data_t));
		if (buf == NULL) {
			return;
		}
        
		timing_stop();
		timing_t end_time = timing_counter_get();
        uint64_t diff = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));

		buf->time = diff;
		k_fifo_put(&data_fifo, buf);
		// printk("Red added to fifo: %lld\n",buf->time);

		k_yield();
	}
}

// Yellow LED task
void yellow_led_task(void *, void *, void*) {

	while(true){
		timing_start();
		timing_t yellow_start_time = timing_counter_get();

   		k_sem_take(&yellow_sem, K_FOREVER);
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);
        //printk("Yellow on\n");
        k_sleep(K_MSEC(1000));
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        //printk("Yellow off\n");
		k_sem_give(&release_sem);

		timing_t yellow_end_time = timing_counter_get();
		timing_stop();
    	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&yellow_start_time, &yellow_end_time));
		printk("Yellow task: %lld\n", timing_ns);
	}

}

    

// Green LED task
void green_led_task(void *, void *, void*) {

	while(true){
		timing_start();
		timing_t green_start_time = timing_counter_get();

		k_sem_take(&green_sem, K_FOREVER);
        gpio_pin_set_dt(&green, 1);
        //printk("Green on\n");
        k_sleep(K_MSEC(1000));
        gpio_pin_set_dt(&green, 0);
        //printk("Green off\n");
        gpio_pin_set_dt(&red, 0);
		k_sem_give(&release_sem);

		timing_t green_end_time = timing_counter_get();
		timing_stop();
    	uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&green_start_time, &green_end_time));
		printk("Green task: %lld\n", timing_ns);
	}
}


char buffer[10];
int index = 0;
void uart_task(void *, void *, void *)
{
	unsigned char ch;

	while(true){
		if(uart_poll_in(uart_dev, &ch) == 0) {
			if(ch != '\r'){
				buffer[index++] = ch;
			} else {
				struct data_t *item = k_malloc(sizeof(struct data_t));
				if(item){
					memcpy(item->seq, buffer, 10);
					item->len = 10;
					k_fifo_put(&data_fifo, item);
				}
			}
						
			if(index == 10){

				index = 0;
			}
		}
		k_msleep(20);
	}
}

void dispatcher_task(void *, void *, void *)
{
	while(true){
		struct data_t *rec_item = k_fifo_get(&data_fifo, K_FOREVER);

		for(int i = 0; i < rec_item->len; i++){
			char c = rec_item->seq[i];

			if(c >= 'a' && c <= 'z'){
				c = c - 'a' + 'A';
			}

			switch(c){
				case 'R' :
					k_sem_give(&red_sem);
					break;
				case 'Y' :
					k_sem_give(&yellow_sem);
					break;
				case 'G' :
					k_sem_give(&green_sem);
					break;
				case 'D' :
					k_sem_give(&debug_sem);
					break;
				default : 
					printk("Was given wrong char, give a new one\n");
					break;
			}
			k_sem_take(&release_sem, K_FOREVER);
		}

		k_free(rec_item);
	}
}

// LED initialization
int init_leds() {
    int r = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    r |= gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    r |= gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&red,0); gpio_pin_set_dt(&green,0); gpio_pin_set_dt(&blue,0);
    return r;
}

void debug_task(void *, void *, void*) {

	k_sem_take(&debug_sem, K_FOREVER);
	// Store received data
	struct data_t *received;

	while (true) {

		received = k_fifo_get(&data_fifo, K_FOREVER);
		printk("Debug received: %lld\n", received->time);
		k_free(received);

		k_yield();
		
		k_sem_give(&release_sem);
	}
}

// Main
int main(void){
    if(init_uart() != 0){ printk("UART failed\n"); return 1;}

	timing_init();
    timing_start();
	timing_t start_time = timing_counter_get();

    k_msleep(100);
    init_leds();
    init_buttons();

	printk("Program started..\n");

	timing_t end_time = timing_counter_get();
	timing_stop();
    uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&start_time, &end_time));
	printk("Initialization: %lld\n", timing_ns);

    while(1) k_sleep(K_MSEC(100));
    return 0;
}
