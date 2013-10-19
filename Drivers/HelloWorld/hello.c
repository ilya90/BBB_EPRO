#include <linux/module.h>
#include <linux/init.h>

MODULE_AUTHOR("Ilja Fursov <ilfur12@student.sdu.dk");	
MODULE_DESCRIPTION("Hello World test driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");

static int hello_init(void){
	printk(KERN_INFO "Hello World\n");
	return 0;
}

static void hello_exit(void){
	printk(KERN_INFO "Goodbye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);
