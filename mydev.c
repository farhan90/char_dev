#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h> /*class_create, device_create*/
#include <linux/types.h> /*dev_t*/
#include <linux/fs.h> /* struct file_operatiosn*/
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h> /* snprintf*/
#include <linux/time.h> /*current_kernel_time */

static dev_t dev; /*To get the device number*/
static struct cdev c_dev; /* The char device */
static struct class *cl; /* Device class*/

static char* data_buff;
static int data_buff_len=100;

int dev_open(struct inode *i, struct file *file){
	printk("FARHAN: Opening my char device\n");
	return 0;
}

ssize_t dev_read(struct file *f, char __user *ubuf, 
			size_t len,loff_t *off){
	
	struct timespec ts;
	int ret=0;
	printk("FARHAN: Reading my char device\n");

	ts=current_kernel_time();
	
	ret=snprintf(data_buff,data_buff_len,
				"Current Kernel Time: %lu \n",ts.tv_sec);
	if(copy_to_user(ubuf,data_buff,data_buff_len)<0){
		printk("FARHAN: Failed to copy to user\n");
		kfree(data_buff);
		return -1;
	}
		
	return ret;

}

static int init_data(void){
	
	data_buff=kmalloc(data_buff_len,GFP_KERNEL);
	if (!data_buff){
		printk("FARHAN: No memory\n");
		return -ENOMEM;
	}
	return 0;
}

/* Defining file operations of my device */
static struct file_operations dev_fops={
	.owner=THIS_MODULE,
	.open=dev_open,
	.read=dev_read,
};

static int mydev_init(void){
	printk("FARHAN: Trying to register my char dev\n");

	/* Allocating a device with major & minor number*/
	if(alloc_chrdev_region(&dev,0,1,"mydev")<0){
		printk("FARHAN:Error allocating chardev region\n");
		return -1;
	}
	/* ls /sys/class */
	if((cl=class_create(THIS_MODULE,"mydev"))==NULL){
		printk("FARHAN: Failed to create class\n");
		goto rm_dev;
	}
	/* ls /dev */
	/* We create a device node */
	if(device_create(cl,NULL,dev,NULL,"mydev")==NULL){
		printk("FARHAN: Failed to create device\n");
		goto rm_class;
	}

	/* Initializing the char device with my
 	device specific functions */
	cdev_init(&c_dev,&dev_fops);

	if(cdev_add(&c_dev,dev,1)<0){
		printk("FARHAN : Failed to add device\n");
		goto rm_cdev;
	}

	if(init_data()<0){
		printk("FARHAN: Error init data\n");
		goto rm_cdev;
	}
	printk("FARHAN : Finished registering the device\n");
	return 0;	


rm_cdev:
	device_destroy(cl,dev);

rm_class:
	class_destroy(cl);

rm_dev:
	unregister_chrdev_region(dev,1);
	return -1;


}

static void mydev_exit(void){
	printk("FARHAN: Removing my device\n");
	cdev_del(&c_dev);
	device_destroy(cl,dev);
	class_destroy(cl);
	unregister_chrdev_region(dev,1);
	if(data_buff){
		kfree(data_buff);
	}	
	return;
}

module_init(mydev_init);
module_exit(mydev_exit);
MODULE_LICENSE("GPL");




