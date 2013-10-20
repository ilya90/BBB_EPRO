#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>

MODULE_AUTHOR("Ilja Fursov <ilfur12@student.sdu.dk");	
MODULE_DESCRIPTION("Button GPIO device driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
#define DEVICE_NAME "button1"
#define CLASS_NAME "button1class"

#define STATUS_BUFFER_SIZE 128
#define GPIO_BUTTON1_NUMBER 30
#define GPIO_LED1_NUMBER 60

static int cnt = 0;
unsigned char text[100];

/*	echo 30 > /sys/class/gpio/export		*/
/*	echo in > /sys/class/gpio/gpio30/direction	*/
/*	cat /sys/class/gpio/gpio30/value		*/

struct button1 {
	struct cdev cdev;
	struct device* dev;
	struct class* class;
	dev_t dev_t;
	int major;
	int minor;
	char *status;
};

static struct button1 button1;

static ssize_t button1_write( struct file *filp, const char *user_buf, size_t count, loff_t *f_pos ){
	printk(KERN_INFO "WRITE FUNCTION CALLED!\n");
	int status = 0;
	return status;
}

static ssize_t button1_read( struct file* filp, char *user_buf, size_t count, loff_t *f_pos )
{
	printk(KERN_INFO "READ FUNCTION CALLED!\n");
	int status =0;

	int button_value = gpio_get_value(GPIO_BUTTON1_NUMBER);
        printk(KERN_INFO "BUTTON VALUE = %d\n",button_value );
	
	if (button_value == 0)
	{
		sprintf(text,"Button is PRESSED\n");
		gpio_set_value(GPIO_LED1_NUMBER, 1);
	}	
	else 
	{
		sprintf(text,"Button is RELEASED\n");
		gpio_set_value(GPIO_LED1_NUMBER, 0);
	}

	if(text[cnt]!='\0') {
                copy_to_user(user_buf, &text[cnt], 1);
                status = 1;
                cnt++;
        }
        else {
                status = 0;
                cnt=0;        
        }
	
	return status;
}

static int button1_open( struct inode *inode, struct file *file )
{
	//Here you would set mutex etc. so no one else can access your critical regions.
	printk(KERN_INFO "OPEN FUNCTION CALLED!\n");
	return 0;
}

static int button1_close( struct inode *inode, struct file *file )
{
	//Here you release your mutex again ... or set the device to sleep etc.
	printk(KERN_INFO "CLOSE FUNCTION CALLED!\n");
	return 0;
}

static struct file_operations f_ops =
{
	.owner		= THIS_MODULE,
	.open		= button1_open,
	.read		= button1_read,
	.write		= button1_write,
	.release	= button1_close,
};

static int button1_init(void){
	printk(KERN_INFO "Initializing character device!\n");
	int error;
	
/*	devt structure for major/minor numbers*/
	button1.dev_t = MKDEV(0, 0);
	error = alloc_chrdev_region(&button1.dev_t, 0, 1, DEVICE_NAME);
	if (error < 0) {
		printk(KERN_WARNING "button1: Can't get device number %d\n", button1.minor);
	return error;
	}

	button1.major = MAJOR(button1.dev_t);
	printk(KERN_INFO "button1: Successfully allocated device number Major: %d, Minor: %d\n", button1.major, button1.minor);
	
/*	cdev structure (uses fops structure)*/
	cdev_init(&button1.cdev, &f_ops);
	button1.cdev.owner = THIS_MODULE;

/*	Make device available*/
	error = cdev_add(&button1.cdev, button1.dev_t, 1);
	if (error) {
	printk(KERN_ALERT "button1: cdev_add() failed: error = %d\n", error);
	unregister_chrdev_region(button1.dev_t, 1);
	return -1;
	}	

/*	Class creation*/
	button1.class = class_create(THIS_MODULE, DEVICE_NAME);

	if (!button1.class) {
	printk(KERN_ALERT "button1: class_create(i2c) failed\n");
	return -1;
	}

	if (!device_create(button1.class, NULL, button1.dev_t,NULL, DEVICE_NAME)) {
	class_destroy(button1.class);
	return -1;
	}

	if(gpio_request(GPIO_BUTTON1_NUMBER, "button1")){
                printk( KERN_ALERT "gpio_button1 request failed\n" );
                device_destroy( button1.class, button1.dev_t );
                class_destroy( button1.class );
                unregister_chrdev_region( button1.dev_t, 1 );
                return -1;
        }
	gpio_direction_input(GPIO_BUTTON1_NUMBER);
	gpio_export(GPIO_BUTTON1_NUMBER, true);
	
	if(gpio_request(GPIO_LED1_NUMBER, "led1")){
                printk( KERN_ALERT "gpio_led1 request failed\n" );
                device_destroy( button1.class, button1.dev_t );
                class_destroy( button1.class );
                unregister_chrdev_region( button1.dev_t, 1 );
                return -1;
        }
	gpio_direction_output(GPIO_LED1_NUMBER, 0);
	gpio_export(GPIO_LED1_NUMBER, true);

	printk(KERN_INFO "button1: Successfully registered driver with kernel.\n");

	return 0;
}

static void button1_exit(void){
	printk(KERN_INFO "Removing character device!\n");

/*	When exit is called, clean up kernel data*/
	cdev_del(&button1.cdev);
	unregister_chrdev_region(button1.dev_t, 1);	
	gpio_free(GPIO_BUTTON1_NUMBER);
	gpio_free(GPIO_LED1_NUMBER);

	printk(KERN_WARNING "button1: Device unregistered\n");
}

module_init(button1_init);
module_exit(button1_exit);
