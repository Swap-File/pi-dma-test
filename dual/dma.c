/*
* Test SPI DMA
*
* Copyright (C) 2020 Cirrus Logic, Inc. and
*                    Cirrus Logic International Semiconductor Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/
#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function


#define TEST_SIZE (4096*20)  //This is 20ms of transmission at 32mhz


#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer


// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

char *buffer1;
char *buffer2;
struct regmap *regmap1;
struct regmap *regmap2;

int dev = 0;

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static struct regmap_config test_spi_regmap1 = {
	.name = "test_spi1",
	.reg_bits = 32,
	.pad_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,

	.max_register = 0xFFFFFFFF,
	.cache_type = REGCACHE_NONE,
};

static struct regmap_config test_spi_regmap2 = {
	.name = "test_spi2",
	.reg_bits = 32,
	.pad_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,

	.max_register = 0xFFFFFFFF,
	.cache_type = REGCACHE_NONE,
};

static int test_spi_probe(struct spi_device *spi)
{
	
	int i, ret = 0;
	
	if (dev == 0){
		
		printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

		// Try to dynamically allocate a major number for the device -- more difficult but worth it
		majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
		if (majorNumber<0){
			printk(KERN_ALERT "EBBChar failed to register a major number\n");
			return majorNumber;
		}
		printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

		// Register the device class
		ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
		if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
			unregister_chrdev(majorNumber, DEVICE_NAME);
			printk(KERN_ALERT "Failed to register device class\n");
			return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
		}
		printk(KERN_INFO "EBBChar: device class registered correctly\n");

		// Register the device driver
		ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
		if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
			class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
			unregister_chrdev(majorNumber, DEVICE_NAME);
			printk(KERN_ALERT "Failed to create the device\n");
			return PTR_ERR(ebbcharDevice);
		}
		printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
		
		
		pr_info("%s",__func__);
		
		printk(KERN_INFO "Move Data.\n");
		
		regmap1 = devm_regmap_init_spi(spi, &test_spi_regmap1);
		if (IS_ERR(regmap1)) {
			ret = PTR_ERR(regmap1);
			dev_err(&spi->dev, "Failed to allocate register map: %d\n", ret);
		}
		
		buffer1 = kmalloc(TEST_SIZE, GFP_KERNEL);
		
		for (i = 0; i < TEST_SIZE; i++){
			buffer1[i] = i % 0xff;
		}
		
		dev = 1;
		
	} else {
		
		pr_info("%s",__func__);
		
		printk(KERN_INFO "Move Data.\n");
		
		regmap2 = devm_regmap_init_spi(spi, &test_spi_regmap2);
		if (IS_ERR(regmap2)) {
			ret = PTR_ERR(regmap2);
			dev_err(&spi->dev, "Failed to allocate register map: %d\n", ret);
		}
		
		buffer2 = kmalloc(TEST_SIZE, GFP_KERNEL);
				
		for (i = 0; i < TEST_SIZE; i++){
			buffer2[i] = i % 0xff;
		}


	}
	return ret;
}


static int test_spi_remove(struct spi_device *spi)
{
	int ret = 1;
	
	if (dev == 1){

		device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
		class_unregister(ebbcharClass);                          // unregister the device class
		class_destroy(ebbcharClass);                             // remove the device class
		unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
		printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");

		kfree(buffer1);
		kfree(buffer2);

		dev = 0;
	}
	return ret;

}


static const struct of_device_id test_spi_match[] = {
	{ .compatible = "spi-dma-test", },
	{},
};
MODULE_DEVICE_TABLE(of, test_spi_match);

static struct spi_driver test_spi_driver = {
	.driver = {
		.name = "spi-dma-test",
		.of_match_table = test_spi_match,
	},
	.probe = &test_spi_probe,
	.remove = &test_spi_remove,
};

module_spi_driver(test_spi_driver);

MODULE_DESCRIPTION("Test RPI4 SPI BUG");
MODULE_AUTHOR("Lucas Tanure <tanureal@opensource.cirrus.com>");
MODULE_LICENSE("GPL v2");


/** @brief The device open function that is called each time the device is opened
*  This will only increment the numberOpens counter in this case.
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_open(struct inode *inodep, struct file *filep){
	numberOpens++;
	printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
	return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
*  being sent from the device to the user. In this case is uses the copy_to_user() function to
*  send the buffer string to the user and captures any errors.
*  @param filep A pointer to a file object (defined in linux/fs.h)
*  @param buffer The pointer to the buffer to which this function writes the data
*  @param len The length of the b
*  @param offset The offset if required
*/
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
	int error_count = 0;
	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
	error_count = copy_to_user(buffer, message, size_of_message);

	if (error_count==0){            // if true then have success
		printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
		return (size_of_message=0);  // clear the position to the start and return 0
	}
	else {
		printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
		return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
	}
}

/** @brief This function is called whenever the device is being written to from user space i.e.
*  data is sent to the device from the user. The data is copied to the message[] array in this
*  LKM using the sprintf() function along with the length of the string.
*  @param filep A pointer to a file object
*  @param buffer The buffer to that contains the string to write to the device
*  @param len The length of the array of data that is being passed in the const char buffer
*  @param offset The offset if required
*/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	sprintf(message, "%s(%zu letters)", buffer, len);   // appending received string with its length
	size_of_message = strlen(message);                 // store the length of the stored message
	printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);

	regmap_raw_write_async(regmap1, 0, buffer1, TEST_SIZE);
	regmap_raw_write_async(regmap2, 0, buffer2, TEST_SIZE);
	
	return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
*  the userspace program
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_release(struct inode *inodep, struct file *filep){
	printk(KERN_INFO "EBBChar: Device successfully closed\n");
	return 0;
}