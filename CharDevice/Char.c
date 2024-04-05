#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/uaccess.h>

#define MINOR_FIRST 0
#define MINOR_COUNT 1
#define DEV_NAME "Char"
#define BUFFER_SIZE 256

static struct cdev my_device;
static struct class *my_class;
static struct device *oni_device;
static size_t size_of_msg = 0;
char msg[BUFFER_SIZE] = {0};

static dev_t my_device_number;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kora Nguyen");
MODULE_DESCRIPTION("A simple Linux char device driver");
MODULE_VERSION("0.1");

static int driver_open(struct inode *, struct file *);
static int driver_close(struct inode *, struct file *);
static ssize_t driver_write(struct file *, const char __user *, size_t count, loff_t *offset);
static ssize_t driver_read(struct file *, char __user *, size_t count, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.write = driver_write,
	.read = driver_read
};

static ssize_t driver_read(struct file *filp, char __user *buffer, size_t count, loff_t *offset)
{
	if ((*offset + count) > BUFFER_SIZE)
		count = BUFFER_SIZE - *offset;

	if (copy_to_user(buffer, msg, count))
	{
		pr_info("Info: Failed to send %zu chars to the user\n", count);
		return -EFAULT;
	}

	*offset += count;
	pr_info("Info: Number of bytes successfully read = %zu\n", count);
	return count;
}

static ssize_t driver_write(struct file *filp, const char __user *buffer, size_t count, loff_t *offset)
{
	if ((*offset + count) > BUFFER_SIZE)
		count = BUFFER_SIZE - *offset;

	if (!count)
		return -ENOMEM;
	if(copy_from_user(msg, buffer, count))
	{
		return -EFAULT;
	}

	*offset += count;
	pr_info("Info: receive %zu characters from the user\n", count);
	return count;
}

static int driver_open(struct inode *node, struct file *filp)
{
  printk("open was called\n");
	return 0;
}

static int driver_close(struct inode *node, struct file *filp)
{
  printk("close was called\n");
	return 0;
}

void __exit oni_exit(void)
{
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	cdev_del(&my_device);
	unregister_chrdev_region(my_device_number, MINOR_COUNT);
}

int __init oni_init(void)
{
	int ret;
	ret = alloc_chrdev_region(&my_device_number, MINOR_FIRST, MINOR_COUNT, DEV_NAME);
	if (ret != 0)
	{
		printk(KERN_WARNING "Cannot allocate a device number");
		return ret;
	}
	cdev_init(&my_device, &my_fops);
	ret = cdev_add(&my_device, my_device_number, MINOR_COUNT);
	if (ret != 0)
	{
		unregister_chrdev_region(my_device_number, MINOR_COUNT);
		printk(KERN_WARNING "Cannot add device to kernel");
		return ret;
	}
	
	my_class = class_create(THIS_MODULE, DEV_NAME);
	if (IS_ERR(my_class))
	{
		cdev_del(&my_device);
		unregister_chrdev_region(my_device_number, MINOR_COUNT);
		printk(KERN_WARNING "Cannot create class");
		return PTR_ERR(my_class);
	}
	
	oni_device = device_create(my_class, NULL, my_device_number, NULL, DEV_NAME);
	if (IS_ERR(oni_device))
	{
		class_destroy(my_class);
		cdev_del(&my_device);
		unregister_chrdev_region(my_device_number, MINOR_COUNT);
		printk(KERN_WARNING "Cannot create device file");
		return PTR_ERR(oni_device);
	}
	return 0;
}

module_init(oni_init);
module_exit(oni_exit);
