/* This is a simple template for a character device 
 * driver which can be used as frame for further extension.
 * It creates a device from a custom class and provides the
 * device file /dev/dummy_dulldev which accepts the commands
 * "set" and "clr" to control the state of a virtual LED.
 *
 * After the module is compiled and copied to your beaglebone
 * install the module using:
 *
 * ~# insmod dulldev.ko
 *
 * You can check the kernel log if everything went well:
 *
 * ~# dmesg | tail
 * [   49.307617] dulldev: Successfully allocated device number Major: 246, Minor: 0
 * [   49.307647] dulldev: Successfully registered driver with kernel.
 * [   49.307739] dulldev: Successfully created custom device class.
 * [   49.313629] dulldev: Successfully created device.
 * [   49.313659] dulldev: Successfully created device file.
 * [   49.313690] dulldev: Successfully allocated memory at address cfb392c0.
 *
 * If all went well then the device file should be created in the /dev directory.
 *
 * ~# ls -l | grep dummy
 * -rw-r--r-- 1 root root      4 Feb 14 22:28 dummy_dulldev
 * 
 * You can now change the status of the virtual LED by writing into the device file:
 *
 * ~# echo set > /dev/dummy_dulldev
 * ~# dmesg | tail
 * [  996.621307] dulldev: Switching virtual LED: ON
 *
 * You can then also check what the status of your virtual LED is at the moment by
 * reading from the device:
 *
 * ~# cat /dev/dummy_dulldev
 * ~# dmesg | tail
 * [ 1128.117309] dulldev: Read device status. Status: set
 *
 * Now you could clear the status again and check the status change. play around with it and 
 * try to understand the code a bit.  
 */ 

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>

#define DEVICE_NAME "dulldev"
#define CLASS_NAME "dummy"

#define STATUS_BUFFER_SIZE 128


/* 
 * Get rid of taint message by declaring code as GPL. 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robert Brehm <brehm@mci.sdu.dk");	/* Who wrote this module? */
MODULE_DESCRIPTION("A dummy character device driver as demonstrator."); /* What does this module do */

struct dulldev {
	struct cdev cdev;
	struct device* dev;
	struct class* class;
	dev_t dev_t;
	int major;
	int minor;
	char *status;
};

static struct dulldev dulldev;

static ssize_t dulldev_read( struct file* F, char *buf, size_t count, loff_t *f_pos )
{
	int status = 0;
	int len = strlen(dulldev.status);

	if(len < count)
		count = len;	
	printk(KERN_INFO "dulldev: Read device status. Status: %s", dulldev.status);
	
	if(copy_to_user(buf, dulldev.status,count)) {
		status = -EFAULT;
		goto read_done;
	}
	
	read_done:
		return status;
}


static ssize_t dulldev_write( struct file* F, const char *buf, size_t count, loff_t *f_pos )
{
	ssize_t status = 0;
	//int i = 0;
	
	/* Copy data from the userspace (how much is read depends on the return value) */
	if(copy_from_user(dulldev.status, buf, count)) {
		printk(KERN_ALERT "dulldev: Error copying data from userspace");
		status = -EFAULT;
		goto write_done;	
	}	

	/* Now process the command */
	if(dulldev.status[0] == 's' && dulldev.status[1] == 'e' 
					&& dulldev.status[2] == 't') {
		printk(KERN_INFO "dulldev: Switching virtual LED: ON\n");
		status = 4;
	}
	else if(dulldev.status[0] == 'c' && dulldev.status[1] == 'l' 
						&& dulldev.status[2] == 'r') {
		printk(KERN_INFO "dulldev: Switching virtual LED: OFF\n");
		status = 4;
	}
	else { 
		//printk(KERN_WARNING "dulldev: Illegal command!\n");
		//status = 1;
		status = 1;
	}

	write_done:
		return status;
}

static int dulldev_open( struct inode *inode, struct file *file )
{
	//Here you would set mutex etc. so no one else can access your critical regions.

	return 0;
}

static int dulldev_close( struct inode *inode, struct file *file )
{
	//Here you release your mutex again ... or set the device to sleep etc.
	return 0;
}

static struct file_operations FileOps =
{
	.open         = dulldev_open,
	.read         = dulldev_read,
	.write        = dulldev_write,
	.release      = dulldev_close,
};

static ssize_t sculld_show_dev(struct device *ddev,
                                struct device_attribute *attr,
                               char *buf)
 {
         /* no longer a good example. should use attr parameter. */
        return 0;
}
 
static DEVICE_ATTR(dulldev, S_IRUGO, sculld_show_dev, NULL);


static void exit_dulldev(void) { 
	/* unregister fs (node) in /dev */
	device_remove_file(dulldev.dev, &dev_attr_dulldev);

	/* destroy device */
	device_destroy(dulldev.class, dulldev.dev_t);

	/* destroy the class */
	class_destroy(dulldev.class);

	/* Get rid of our char dev entries */
	cdev_del(&dulldev.cdev);

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(dulldev.dev_t, 1);	

	printk(KERN_WARNING "dulldev: Device unregistered\n");
}

static int init_dulldev(void)
{
	int error;
	
	dulldev.dev_t = MKDEV(0,0);
	/* Allocate a device number (major and minor). The kernel happily do this 
	 * for you using the alloc_chrdev_region function.
	 *
	 * https://www.kernel.org/doc/htmldocs/kernel-api/API-alloc-chrdev-region.html
   	 *
  	 */

	error = alloc_chrdev_region(&dulldev.dev_t, dulldev.minor, 0, DEVICE_NAME);		
	if (error < 0) {
		printk(KERN_WARNING "dulldev: Can't get device number %d\n", dulldev.minor);
		return error;
	}
	dulldev.major = MAJOR(dulldev.dev_t);
	
	printk(KERN_INFO "dulldev: Successfully allocated device number Major: %d, Minor: %d\n",dulldev.major, dulldev.minor);

	/* The kernel uses structures of type struct cdev to represent char devices internally. 	 
	* Before the kernel invokes our device's operations, we must allocate and register one or 		 
	* more of these structures. To do so, the code must include <linux/cdev.h>, where the 		 
	* structure and its associated helper functions are defined.
	* 
	* In our example here we want to embed the cdev structure within a device-specific structure 		 
	* of your own. In this case we initialize the structure that we have already allocated.  
	 */
	cdev_init(&dulldev.cdev, &FileOps);
	/*struct cdev has an owner field that should be set to THIS_MODULE.*/
	dulldev.cdev.owner = THIS_MODULE;
	/*Set your specific fiel operations defined in the above FileOps struct*/	
	dulldev.cdev.ops = &FileOps;

	/* Once the cdev structure is set up, the final step is to tell the kernel about it with a 	    
	* call to:
	* https://www.kernel.org/doc/htmldocs/kernel-api/API-cdev-add.html
	* Fail gracefully if need be 
	*/
	if (cdev_add (&dulldev.cdev, dulldev.dev_t, 1)) {
		printk(KERN_WARNING "dulldev: Error adding device.\n");
		goto fail;	
	}
	printk(KERN_INFO "dulldev: Successfully registered driver with kernel.\n");


	/* 
	 * Now we need to create the device node for user interaction in /dev
	 * 
	 *
 	 * We can either tie our device to a bus (existing, or one that we create)
 	 * or use a "virtual" device class. For this example, we choose the latter.
	 *
  	 */
 	dulldev.class = class_create(THIS_MODULE, CLASS_NAME);
  	if (dulldev.class < 0) {
  		printk(KERN_WARNING "dulldev: Failed to register device class.\n");
  		goto fail;
 	}
	printk(KERN_INFO "dulldev: Successfully created custom device class.\n");

	/* With a class, the easiest way to instantiate a device is to call device_create() */
 	dulldev.dev = device_create(dulldev.class, NULL, dulldev.dev_t, NULL, CLASS_NAME"_"DEVICE_NAME);
	if (dulldev.dev < 0) {
		printk(KERN_WARNING "dulldev: Failed to create device.\n");
  		goto fail;
	}
	printk(KERN_INFO "dulldev: Successfully created device.\n");

	error = device_create_file(dulldev.dev, &dev_attr_dulldev);
	if(error < 0) {
		printk(KERN_WARNING "dulldev: Error creatinbg device file.\n");
	}
	printk(KERN_INFO "dulldev: Successfully created device file.\n");
		

	//AND allocate some memory for our status buffer:
	
	if(!dulldev.status) {
		dulldev.status = kmalloc(STATUS_BUFFER_SIZE, GFP_KERNEL);
		if(!dulldev.status) {
			printk(KERN_ALERT "dulldev: Failed to alloacte memory.\n");
			return -ENOMEM;
		}	
	}
	printk(KERN_ALERT "dulldev: Successfully allocated memory at address %p.\n",	
			dulldev.status);

	return 0; /* succeed */

  	fail:
		exit_dulldev();
		return error;
}
module_init(init_dulldev);
module_exit(exit_dulldev);
