// Jere Kortesalmi
// Tavoite: 3/3 pistettä ja kaikki toimii fifo ja sekvenssit

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/uart.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define STACKSIZE 500
#define PRIORITY 5

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Global state
volatile int led_state = 0;
volatile int pause_state = 0;
volatile bool red_on = false;
volatile bool green_on = false;
volatile bool blue_on = false;
int init_leds();
int init_buttons();

void button0handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button1handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button2handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button3handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button4handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void uart_task(void *, void *, void *);
void dispatcher_task(void *, void *, void *);

// Thread initialization
K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);

K_FIFO_DEFINE(data_fifo);

// FIFO jokaiselle LED-värille
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);

struct data_t {
    void *fifo_reserved;
    char seq[50];
    int len;
};

// LED-komento struktuuri ajastuksella
struct led_cmd_t {
    void *fifo_reserved;
    char color;
    int duration_ms;
};

int init_uart(void){
    if (!device_is_ready(uart_dev)){
        return 1;
    }
    return 0;
}

void button_add_char(char c){
    struct data_t *item = k_malloc(sizeof(struct data_t));
    if(item){
        item->seq[0] = c;
        item->len = 1;
        k_fifo_put(&data_fifo, item);
        printk("Lisättiin sekvenssiin: %c\n", c);
    }
}

// Semaforit LED-taskien synkronointiin
K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(yellow_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(release_sem, 0, 1);

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Button pin definitions
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


// Main
int main(void)
{
    if(init_uart() != 0){
        printk("UART init failed\n");
        return 1;
    }

    k_msleep(100);
    init_leds();
    init_buttons();

    while(1) { k_sleep(K_MSEC(100)); }
    return 0;
}

// LED-taskit ajastuksella
void red_led_task(void *, void *, void*){
    struct led_cmd_t *cmd;
    while(1){
        cmd = k_fifo_get(&red_fifo, K_FOREVER);
        gpio_pin_set_dt(&red,1);
        k_msleep(cmd->duration_ms);
        gpio_pin_set_dt(&red,0);
        k_free(cmd);
        k_sem_give(&release_sem);
    }
}

void yellow_led_task(void *, void *, void*){
    struct led_cmd_t *cmd;
    while(1){
        cmd = k_fifo_get(&yellow_fifo, K_FOREVER);
        gpio_pin_set_dt(&red,1);
        gpio_pin_set_dt(&green,1);
        k_msleep(cmd->duration_ms);
        gpio_pin_set_dt(&red,0);
        gpio_pin_set_dt(&green,0);
        k_free(cmd);
        k_sem_give(&release_sem);
    }
}

void green_led_task(void *, void *, void*){
    struct led_cmd_t *cmd;
    while(1){
        cmd = k_fifo_get(&green_fifo, K_FOREVER);
        gpio_pin_set_dt(&green,1);
        k_msleep(cmd->duration_ms);
        gpio_pin_set_dt(&green,0);
        k_free(cmd);
        k_sem_give(&release_sem);
    }
}

void button0handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
	button_add_char('R');
}
void button1handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
	button_add_char('Y');
}
void button2handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
	button_add_char('G');
}
void button3handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
	button_add_char('T');
}
void button4handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
	button_add_char('\r');
}

// UART-task: vastaanottaa sekvenssiä muodossa R,1000\rY,500\rT,3\r
char uart_buffer[50];
int uart_index = 0;
void uart_task(void *, void *, void *){
    unsigned char ch;
    while(1){
        if(uart_poll_in(uart_dev, &ch) == 0){
            if(ch != '\r'){
                uart_buffer[uart_index++] = ch;
            } else {
                struct data_t *item = k_malloc(sizeof(struct data_t));
                if(item){
                    memcpy(item->seq, uart_buffer, uart_index);
                    item->len = uart_index;
                    k_fifo_put(&data_fifo, item);
                }
                uart_index = 0;
            }
            if(uart_index >= sizeof(uart_buffer)) uart_index = 0;
        }
        k_msleep(20);
    }
}

// Dispatcher-task: purkaa sekvenssin ja laittaa LED-FIFOihin
void dispatcher_task(void *, void *, void *){
    while(1){
        struct data_t *rec_item = k_fifo_get(&data_fifo, K_FOREVER);

        struct led_cmd_t cmds[20];
        int cmd_count = 0;
        int repeat_count = 1;

        char *ptr = rec_item->seq;
        char *line = strtok(ptr, "\r");

        while(line != NULL){
            if(line[0] == 'T'){ // toisto
                repeat_count = atoi(line+2);
            } else {
                char color;
                int duration;
                if(sscanf(line, "%c,%d", &color, &duration) == 2){
                    cmds[cmd_count].color = color;
                    cmds[cmd_count].duration_ms = duration;
                    cmd_count++;
                } else {
                    printk("Virheellinen sekvenssi: %s\n", line);
                }
            }
            line = strtok(NULL, "\r");
        }

        for(int r=0; r<repeat_count; r++){
            for(int i=0; i<cmd_count; i++){
                struct led_cmd_t *cmd = k_malloc(sizeof(struct led_cmd_t));
                *cmd = cmds[i];
                switch(cmd->color){
                    case 'R': k_fifo_put(&red_fifo, cmd); break;
                    case 'Y': k_fifo_put(&yellow_fifo, cmd); break;
                    case 'G': k_fifo_put(&green_fifo, cmd); break;
                    default: 
                        printk("Väärä väri: %c\n", cmd->color);
                        k_free(cmd);
                        break;
                }
                k_sem_take(&release_sem, K_FOREVER);
            }
        }

        k_free(rec_item);
    }
}

// LED & Button initialization functions (sama kuin alkuperäisessä koodissa)
int init_leds() {
    int ret;
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    ret |= gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    ret |= gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);

    gpio_pin_set_dt(&red,0);
    gpio_pin_set_dt(&green,0);
    gpio_pin_set_dt(&blue,0);

    printk("LEDs initialized\n");
    return ret;
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
    gpio_init_callback(&button_0_data, button0handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    gpio_init_callback(&button_1_data, button1handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_data);

    gpio_init_callback(&button_2_data, button2handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_data);

    gpio_init_callback(&button_3_data, button3handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_data);

    gpio_init_callback(&button_4_data, button4handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_data);

    printk("Set up all buttons ok\n");
    return 0;
}
