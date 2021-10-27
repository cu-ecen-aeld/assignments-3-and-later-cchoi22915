/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesd-circular-buffer.h" // circular buffer operations
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Chris Choi"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev* my_device;

	PDEBUG("open");
	/**
	 * TODO: handle open
	 */

	my_device = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = my_device;
	
	return 0;
	
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */

	ssize_t retval = 0;
	ssize_t buffer_offset;
	ssize_t buffer_bytes;
	struct aesd_dev *my_device = NULL;
	struct aesd_buffer_entry *read_entry = NULL;
	my_device = filp->private_data;

	//lock mutex and check return value
	retval = mutex_lock_interruptible(&my_device->lock);
	if(retval < 0)
	{
	  retval = -ERESTARTSYS;
	  return retval;
	}
	
	//find entry offset and check return for error
	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&my_device->buff, *f_pos,&buffer_offset);
	if(read_entry == NULL)
	{
	  mutex_unlock(&my_device->lock);
	  return 0;
	}
	
	//set bytes read and check to see if count < bytes read
	buffer_bytes = read_entry->size - buffer_offset;
	if(count < buffer_bytes)
	   buffer_bytes = count;
	
	//copy and set retval, check retval for fault
	retval = copy_to_user(buf, (read_entry->buffptr + buffer_offset), buffer_bytes);
	if (retval != 0)
	{
		retval = -EFAULT;
		return retval;
	}

	//add bytes read
	*f_pos += buffer_bytes;

	//release mutex
	mutex_unlock(&my_device->lock); 

	//return bytes read
	return buffer_bytes;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */

	ssize_t retval;
	ssize_t bytes_remaining;
	const char* buffer_ret = NULL;
	struct aesd_dev *my_device = NULL;
	char* line_position = NULL;

	my_device = filp->private_data;

	retval = mutex_lock_interruptible(&my_device->lock);
		if(retval < 0)
			return retval;
	
	if (my_device->entry.size == 0) 
	{
		//allocate memory
		my_device->entry.buffptr = kzalloc(count*sizeof(char), GFP_KERNEL); 
		
		//check for error
		if (my_device->entry.buffptr == 0) 
		{
			mutex_unlock(&my_device->lock);
			retval = -ENOMEM; 
	        return retval;
		}
	}
	else 
	{
		//reallocate memory
	    my_device->entry.buffptr = krealloc(my_device->entry.buffptr, my_device->entry.size + count, GFP_KERNEL); 
		
		//check for error
		if (my_device->entry.buffptr == 0) 
		{
			mutex_unlock(&my_device->lock);
	        return retval;
		}
	 }

	//copy from kernel space to user space
	bytes_remaining = copy_from_user((void *)(&my_device->entry.buffptr[my_device->entry.size]), buf, count);
	
	//set return value by calculating count - bytes remaining and add to entry size
	retval = count - bytes_remaining;
	my_device->entry.size += retval;

	//find line end position
	line_position = (char *)memchr(my_device->entry.buffptr, '\n', my_device->entry.size);
	
	//if line end found
	if (line_position != NULL) 
	{
		//add entry and check return
		buffer_ret = aesd_circular_buffer_add_entry(&my_device->buff, &my_device->entry);
	   	if (buffer_ret != NULL) 
			kfree(buffer_ret);
	   
		//set size and ptr to 0
	  	my_device->entry.size = 0;
    	my_device->entry.buffptr = 0;
	}

    *f_pos = 0;
    mutex_unlock(&my_device->lock);

    return retval;
       
	 
}
struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *my_device)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&my_device->cdev, &aesd_fops);
	my_device->cdev.owner = THIS_MODULE;
	my_device->cdev.ops = &aesd_fops;
	err = cdev_add (&my_device->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}



int aesd_init_module(void)
{
	dev_t my_device = 0;
	int result;
	result = alloc_chrdev_region(&my_device, aesd_minor, 1,
			"aesdchar");
	aesd_major = MAJOR(my_device);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */
	mutex_init(&aesd_device.lock); 
	aesd_circular_buffer_init(&aesd_device.buff);

	result = aesd_setup_cdev(&aesd_device);
       
	if( result ) {
		unregister_chrdev_region(my_device, 1);
	}
	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	aesd_circular_buffer_release(&aesd_device.buff);
	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
