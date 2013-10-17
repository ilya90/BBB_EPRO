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

MODULE_AUTHOR("Ilja Fursov <ilfur12@student.sdu.dk");	
MODULE_DESCRIPTION("Simple character device test driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
#define DEVICE_NAME "chardev"
#define CLASS_NAME "randomclass"

#define STATUS_BUFFER_SIZE 128
static int cnt = 0;
unsigned char text[100];
unsigned char answer[100];
struct chardev {
	struct cdev cdev;
	struct device* dev;
	struct class* class;
	dev_t dev_t;
	int major;
	int minor;
	char *status;
};

static struct chardev chardev;

static ssize_t chardev_write( struct file *filp, const char *user_buf, size_t count, loff_t *f_pos ){
	printk(KERN_INFO "WRITE FUNCTION CALLED!\n");
	int status = 0;
	char temp_buf[50];
	cnt = 0;
	
	if (copy_from_user(temp_buf, user_buf, 6)){
		printk(KERN_ALERT "chardev: Error copying data from userspace");
		status = -EFAULT;
		goto write_done;
	}
	if (temp_buf[0] == 'h' && temp_buf[1] == 'e' && temp_buf[2] == 'l' && temp_buf[3] == 'l' && temp_buf[4] == 'o'){
		sprintf(answer,"Good day to you too!!! ;) \n");
		printk(KERN_INFO "%s \n", answer);
		status = 6;
	}
	else{
		sprintf(answer,"Send 'hello' to see answer ;) \n");
		printk(KERN_INFO "%s \n", answer);
		status = count;
	}
	write_done:
		return status;
}

static ssize_t chardev_read( struct file* filp, char *user_buf, size_t count, loff_t *f_pos )
{
	printk(KERN_INFO "READ FUNCTION CALLED!\n");
	int status =0;
	
	sprintf(text,"You just read test character device - 'chardev'\n");
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

static int chardev_open( struct inode *inode, struct file *file )
{
	//Here you would set mutex etc. so no one else can access your critical regions.
	printk(KERN_INFO "OPEN FUNCTION CALLED!\n");
	return 0;
}

static int chardev_close( struct inode *inode, struct file *file )
{
	//Here you release your mutex again ... or set the device to sleep etc.
	printk(KERN_INFO "CLOSE FUNCTION CALLED!\n");
	return 0;
}

static struct file_operations f_ops =
{
	.owner		= THIS_MODULE,
	.open		= chardev_open,
	.read		= chardev_read,
	.write		= chardev_write,
	.release	= chardev_close,
};

static int chardev_init(void){
	printk(KERN_INFO "Initializing character device!\n");
	int error;
	
/*	devt structure for major/minor numbers*/
	chardev.dev_t = MKDEV(0, 0);
	error = alloc_chrdev_region(&chardev.dev_t, 0, 1, DEVICE_NAME);
	if (error < 0) {
		printk(KERN_WARNING "chardev: Can't get device number %d\n", chardev.minor);
	return error;
	}

	chardev.major = MAJOR(chardev.dev_t);
	printk(KERN_INFO "chardev: Successfully allocated device number Major: %d, Minor: %d\n", chardev.major, chardev.minor);
	
/*	cdev structure (uses fops structure)*/
	cdev_init(&chardev.cdev, &f_ops);
	chardev.cdev.owner = THIS_MODULE;

/*	Make device available*/
	error = cdev_add(&chardev.cdev, chardev.dev_t, 1);
	if (error) {
	printk(KERN_ALERT "chardev: cdev_add() failed: error = %d\n", error);
	unregister_chrdev_region(chardev.dev_t, 1);
	return -1;
	}	

/*	Class creation*/
	chardev.class = class_create(THIS_MODULE, DEVICE_NAME);

	if (!chardev.class) {
	printk(KERN_ALERT "chardev: class_create(i2c) failed\n");
	return -1;
	}

	if (!device_create(chardev.class, NULL, chardev.dev_t,NULL, DEVICE_NAME)) {
	class_destroy(chardev.class);
	return -1;
	}



	printk(KERN_INFO "chardev: Successfully registered driver with kernel.\n");

	return 0;
}

static void chardev_exit(void){
	printk(KERN_INFO "Removing character device!\n");

/*	When exit is called, clean up kernel data*/
	cdev_del(&chardev.cdev);
	unregister_chrdev_region(chardev.dev_t, 1);	

	printk(KERN_WARNING "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);
