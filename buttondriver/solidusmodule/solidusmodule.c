#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>		//file_operations structure- which of course allows use to open/close, read/write to device
#include <linux/cdev.h> 	//this is char driver; makes cdev available
#include <linux/semaphore.h>	//used to access semaphores; sychronization behaviors
#include <asm/uaccess.h>	//copy_to_user; copy_from_user
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Weinan Ji <weji12@student.sdu.dk");	/* Who wrote this module? */
MODULE_DESCRIPTION("A fake character device driver."); /* What does this module do */


//(1) Create a structure for our fake device
struct fake_device 
{
	char* data;
	int major_number;
	dev_t dev_num; 		//will hold major number that kernel gives us
	struct cdev *mcdev;	//(2) To later register our device we need a cdev object and some other variables
	struct device* mdev;
	struct class* class;
	struct semaphore sem;
} virdev;

int ret;  		//will be used to hold return values of functions; this is because the kernel stack is very small
   			//so declaring variables all over the pass in our module functions eats up the stack very fast 

   			
#define DEVICE_NAME "solidusdevice"    //name--> appears in /proc/devices
#define CLASS_NAME "virtual_dev"

#define STATUS_BUFFER_SIZE 128

//(7)called on device_file open
// inode reference to the file on disk
// and contains information about that file
// struct file is represents an abstract open file
int device_open(struct inode *inode, struct file *filp)
{
	//only allow one process to open this device by using a semaphore as mutual exclusive lock- mutex
	if(down_interruptible(&virdev.sem) != 0)
	{
		printk(KERN_ALERT "soliduscode: could not lock device during open");
		return -1;
	}
	printk(KERN_INFO "soliduscode: opened device");
	return 0;
}

//(8) called when user wants to get information from the device
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
	//virdev.data = "shit";
	//take data from kernel space(device) to user space (processs)
	//copy_to_user (destination, source, sizeToTransfer)
	printk(KERN_INFO "soliduscode: Reading from device");
	printk(KERN_INFO "device data is %s.", virdev.data);
	//ret = copy_to_user(bufStoreData, virdev.data, bufCount);
	return ret;
}

//(9) called when user wants to send information to the device
static ssize_t device_write(struct file* filp, const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
	//ssize_t status = 6;
	//int i = 0;
	
	/* Copy data from the userspace (how much is read depends on the return value) */
	/*if(copy_from_user(virdev.data, bufSourceData, bufCount)) {
		printk(KERN_ALERT "dulldev: Error copying data from userspace");
		//status = -EFAULT;
		//goto write_done;	
	}	*/

	/* Now process the command */
	/*if(virdev.data[0] == 's' && virdev.data[1] == 'e' 
					&& virdev.data[2] == 't') {
		printk(KERN_INFO "dulldev: Switching virtual LED: ON\n");
		//status = 4;
	}
	else if(virdev.data[0] == 'c' && virdev.data[1] == 'l' 
						&& virdev.data[2] == 'r') {
		printk(KERN_INFO "dulldev: Switching virtual LED: OFF\n");
		//status = 4;
	}
	else { 
		//printk(KERN_WARNING "dulldev: Illegal command!\n");
		//status = 1;
		//status = 4;
	}

	//write_done:
		return status;*/
	
	//send data from user to kernel
	//copy_from_user (dest, source, count)
	printk(KERN_INFO "soliduscode: writing to device");
	ret = copy_from_user(virdev.data, bufSourceData, bufCount);
	printk("soliduscode: User send: %s", virdev.data);
	return 8;
}

//(10) called upon user close
int device_close(struct inode *inode, struct file *filp)
{
	//by calling up , which is opposite of down for semaphore, we release the mutex that we obtained at device open
	//this has the effect of allowing other process to use the device now
	up(&virdev.sem);
	printk(KERN_INFO "soliduscode: closed device");
	return 0;
}

//HERE
//(6) Tell the kernel which functions to call when user operates on our device file
struct file_operations fops = 
{
	.owner = THIS_MODULE,  //prevent unloading of this module when operations are in use
	.open = device_open,  //points to the method to call when opening the device
	.release = device_close, //points to the method to call when closing the device
	.write = device_write,  //points to the method to call when writing to the device
	.read = device_read  //points to the method to call when reading from the device
};


static ssize_t sculld_show_dev(struct device *ddev,
                               struct device_attribute *attr,
                               char *buf)
{
        /* no longer a good example. should use attr parameter. */
        return 0;
}
 
static DEVICE_ATTR(solidusdevice, S_IRUGO, sculld_show_dev, NULL);

static void driver_exit(void)
{
	/* unregister fs (node) in /dev */
	device_remove_file(virdev.mdev, &dev_attr_solidusdevice);

	/* destroy device */
	device_destroy(virdev.class, virdev.dev_num);

	/* destroy the class */
	class_destroy(virdev.class);

	//(5) unregister everyting in reverse order
	//(a)
	cdev_del(virdev.mcdev);

	kfree(virdev.data);

	//(b)
	unregister_chrdev_region(virdev.dev_num, 1);
	printk(KERN_ALERT "soliduscode: unloaded module");
}

static int driver_entry(void)
{ 
	//virdev.dev_num = MKDEV(224,108);
	//(3) Register our device with the system: a two step process
	//step(1) use dynamic allocation to assign our device
	//a major number-- alloc_chrdev_region(dev_t*, uint fminor, uint count, char* name)
	//ret = register_chrdev_region(&virdev.dev_num, 1, DEVICE_NAME);
	ret = alloc_chrdev_region(&virdev.dev_num,0,1,DEVICE_NAME);
	if(ret < 0) 
	{ //at time kernel functions return negatives, there is an error
		printk(KERN_ALERT "soliduscode: failed to allocate a major number");
		return ret; //propagate error
	}
 	virdev.major_number = MAJOR(virdev.dev_num); //extracts the major number and store in our variable (MACRO)
 	printk(KERN_INFO "soliduscode: major number is %d", virdev.major_number);
 	printk(KERN_INFO "\tuse \"mknod /dev/%s c %d 0\" for device file", DEVICE_NAME, virdev.major_number); //dmesg
 	//step(2)
	
	virdev.mcdev = cdev_alloc(); //create our cdev structure, initialized our cdev
	virdev.mcdev->ops = &fops;  //struct file_operations

	//cdev_init(virdev.mcdev, &fops);
 	
 	virdev.mcdev->owner = THIS_MODULE;
 	//now that we created cdev, we have to add it to the kernel 
 	//int cdev_add(struct cdev* dev, dev_t num, unsigned int count)
	ret = cdev_add(virdev.mcdev, virdev.dev_num, 1);
	if(ret < 0) 
	{ //always check errors
		printk(KERN_ALERT "soliduscode: unable to add cdev to kernel");
		return ret;
	}


	virdev.class = class_create(THIS_MODULE, CLASS_NAME);
  	if (virdev.class < 0) 
	{
  		printk(KERN_WARNING "soliduscode: Failed to register device class.\n");
  		goto fail;
 	}
	printk(KERN_INFO "soliduscode: Successfully created custom device class.\n");

	virdev.mdev = device_create(virdev.class, NULL, virdev.dev_num, NULL, CLASS_NAME"_"DEVICE_NAME);
	if (virdev.mdev < 0) {
		printk(KERN_WARNING "soliduscode: Failed to create device.\n");
  		goto fail;
	}
	printk(KERN_INFO "soliduscode: Successfully created device.\n");

	ret = device_create_file(virdev.mdev, &dev_attr_solidusdevice);
	if(ret < 0) {
		printk(KERN_WARNING "soliduscode: Error creatinbg device file.\n");
	}
	printk(KERN_INFO "soliduscode: Successfully created device file.\n");

	
	//AND allocate some memory for our status buffer:
	
	if(!virdev.data) {
		virdev.data = kmalloc(STATUS_BUFFER_SIZE, GFP_KERNEL);
		if(!virdev.data) {
			printk(KERN_ALERT "soliduscode: Failed to alloacte memory.\n");
			return -ENOMEM;
		}	
	}
	printk(KERN_ALERT "soliduscode: Successfully allocated memory at address %p.\n", virdev.data);

	//(4) Initialize our semaphore
 	sema_init(&virdev.sem,1); //initial value of one

  	return 0; 

	fail:
		driver_exit();
		return ret;
}


//Inform the kernel where to start and stop with our module/driver
module_init(driver_entry);
module_exit(driver_exit);
