#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>  
#include <linux/platform_device.h>      /* For platform devices */
#include <linux/interrupt.h>            /* For IRQ */
#include <linux/gpio.h>                 /* For Legacy integer based GPIO */
#include <linux/of_gpio.h>              /* For of_gpio* functions */
#include <linux/of.h>                   /* For DT*/

#define DRIVER_NAME "led7control"
#define FIRST_MINOR 0
#define BUFF_SIZE 100
#define TEST_LED7 0
#define USLEEP_RANGE_MIN 0
#define USLEEP_RANGE_MAX 1000

#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s: "fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s: "fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s: "fmt,DRIVER_NAME, ##args)

int res; 
bool thread_run = false;
char buffs[9];

dev_t device_num ;
struct class *device_class;
struct device *device;
struct gpio_desc *sclk_gpio;
struct gpio_desc *rclk_gpio;
struct gpio_desc *dio_gpio;
static struct task_struct *thread;
static struct cdev cdev;

typedef struct privatedata {

} private_data_t;
private_data_t *data;

//active low
const int segment[] =
{// 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F    -   .
  0x03,0x9F,0x25,0x0D,0x99,0x49,0x41,0x1F,0x01,0x09,0x11,0xC1,0x63,0x85,0x61,0x71,0xFD,0xFE
};

const int index_segment[] =
{// 1    2    3    4    5    6    7    8 
  0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01
};

void set_sclk(void)
{
	gpiod_set_value(sclk_gpio,0);
	gpiod_set_value(sclk_gpio,1);
}

void set_rclk(void)
{
	gpiod_set_value(rclk_gpio,0);
	gpiod_set_value(rclk_gpio,1);
}

void set_num(int t_index, int t_num) 
{
	int i = 0;

    for	(i = 0; i < 8; i++) {
        // pr_info("DIOs:  %d", t_num & index_segment[7-i] ? 1 : 0);
        gpiod_set_value(dio_gpio,t_num & index_segment[7-i] ? 1 : 0);
		set_sclk();
	}

    for	(i = 0; i < 8; i++) {
        // pr_info("IDEs:  %d", t_index & index_segment[7-i] ? 1 : 0);
        gpiod_set_value(dio_gpio, t_index & index_segment[7-i] ? 1 : 0);
		set_sclk();
	}

	set_rclk();
}

void clear_num(void)
{
    int h = 0;

    set_sclk(); 
    set_rclk();
    
    for	(h = 0; h < 8; h++) {
        gpiod_set_value(dio_gpio,0);
        set_sclk();   
    }
    set_rclk();

    gpiod_set_value(dio_gpio,0);
    gpiod_set_value(sclk_gpio,0);
    gpiod_set_value(rclk_gpio,0);
}

void add_led(char *buff)
{
    set_num(index_segment[0], segment[buff[7]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[1], segment[buff[6]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[2], segment[buff[5]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[3], segment[buff[4]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[4], segment[buff[3]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[5], segment[buff[2]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[6], segment[buff[1]-48]);
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
    set_num(index_segment[7], segment[buff[0]-48]); 
    usleep_range(USLEEP_RANGE_MIN, USLEEP_RANGE_MAX);
}

int thread_function(void *pv)
{
    while(!kthread_should_stop()) {
        add_led(buffs);
    }

    do_exit(0);
    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
};

static ssize_t setled_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t len)
{
    private_data_t *data = dev_get_drvdata(dev);
    if (!data)
        PERR("Can't get private data from device, pointer value: %p\n", data);
    else
        PINFO ("Set number %d\n", buff[0]);
    
    if ((buff[0] == 's' && buff[1] == 't' && buff[2] == 'o' && buff[3] == 'p' && (len == 5)) || thread_run == true)
    {
        thread_run = false;
        clear_num();
        PINFO("Stop thread\n");
        kthread_stop(thread);
    }

    if (thread_run == false)
    {
        int i = 0;

        if ((len < 9) & (len > 0))
        {
            for (i = len; i >= 0; i--) buffs[i + 9 - len] = buff[i];
            for (i = 0; i < 9 - len; i++) buffs[i] = 48;
        } else 
            for (i = 0; i < 9; i++) buffs[i] = buff[i];

        thread_run = true;
        thread = kthread_create(thread_function,NULL,"Thread_No_0");
        if(thread) {
            PINFO("Kthread Created Successfully...\n");
            wake_up_process(thread);
        } else {
            PERR("Cannot create kthread\n");
            return -1;
        }
    }

    return len;
} 

static DEVICE_ATTR_WO(setled);

static struct attribute *device_attrs[] = {
        &dev_attr_setled.attr,
	    NULL
};
ATTRIBUTE_GROUPS(device);

static const struct of_device_id gpio_dt_ids[] = {
    { .compatible = "led7-segments", },
    { /* sentinel */ }
};
static int my_pdrv_probe (struct platform_device *pdev)
{
    res = alloc_chrdev_region(&device_num, FIRST_MINOR, 250, DRIVER_NAME); 
    if (res){
        PERR("Can't register driver, error code: %d \n", res); 
        goto error;
    } else
        PINFO("success register driver with major is %d, minor is %d \n", MAJOR(device_num), MINOR(device_num));
    
    cdev_init(&cdev,&fops);
 
    // Adding character device to the system
    if((cdev_add(&cdev,device_num,1)) < 0){
        PINFO("Cannot add the device to the system\n");
        goto error_class;
    }

    // create class 
    device_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(device_class))
    {
        PERR("Class create failed, error code: %p\n", device_class);
        goto error_class;
    } else

    // create private data
    data = (private_data_t*)kcalloc(1, sizeof(private_data_t), GFP_KERNEL);

    // create device and add attribute simultaneously
    device = device_create_with_groups(device_class, NULL, device_num, data, device_groups, DRIVER_NAME"s");
    if (IS_ERR(device))
    {
        PERR("device create fall, error code: %p\n", device);
        goto error_device;
    }

    // init gpio
    sclk_gpio =  gpiod_get(&pdev->dev, "sclk", GPIOD_OUT_LOW);
    rclk_gpio =  gpiod_get(&pdev->dev, "rclk", GPIOD_OUT_LOW);
    dio_gpio =  gpiod_get(&pdev->dev, "dio", GPIOD_OUT_LOW);

    
    gpiod_direction_output(sclk_gpio, 0);
    gpiod_direction_output(rclk_gpio, 0);
    gpiod_direction_output(dio_gpio, 0);
    clear_num();

    // turn on TEST MODE at #define
#if TEST_LED7
    //TEST NUMBER
    int j = 0;
    for (j = 0; j < 108; j++)
    {
        set_num(index_segment[j%8], segment[j%18]);
        msleep(50);
    }
    msleep(1000);
    clear_num();
#endif

    PINFO("Start LED 7-segment driver!\n");
	
    return 0;

    //error handle
#if 0
error_gpio:
    gpiod_put(sclk_gpio);
    gpiod_put(rclk_gpio);
    gpiod_put(dio_gpio);
error_reset_gpio:
    device_destroy(device_class, device_num);
#endif

error_device:
    class_destroy(device_class);
error_class:
    unregister_chrdev_region(device_num, FIRST_MINOR); 
    cdev_del(&cdev);
error:
    return -1;
}
static int my_pdrv_remove(struct platform_device *pdev)
{
    clear_num();
    if (thread_run == true) kthread_stop(thread);
    gpiod_put(sclk_gpio);
    gpiod_put(rclk_gpio);
    gpiod_put(dio_gpio);
    device_destroy(device_class, device_num);
    class_destroy(device_class);
    cdev_del(&cdev);
    unregister_chrdev_region(device_num, FIRST_MINOR); 
    kfree(data);
    PINFO("Remove driver.\n");
    return 0;
}
static struct platform_driver mydriver = {
    .probe      = my_pdrv_probe,
    .remove     = my_pdrv_remove,
    .driver     = {
        .name     = "gpio_led7segment_sample",
        .of_match_table = of_match_ptr(gpio_dt_ids), 
        .owner    = THIS_MODULE,
    },
};
module_platform_driver(mydriver);
MODULE_AUTHOR("Le Phuong Nam <le.phuong.nam@styl.solutions>");
MODULE_LICENSE("GPL v2");
