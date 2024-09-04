#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/io.h>

#define KW_MAX_USER_SIZE 1024
#define BCM2837_GPIO_ADDRESS 0x3F200000
#define BCM2711_GPIO_ADDRESS 0xfe200000

static struct proc_dir_entry *kw_proc = NULL;

static char data_buffer[KW_MAX_USER_SIZE];

static unsigned int *gpio_registers = NULL;

static void gpio_pin_on(unsigned int pin)
{
	unsigned int fsel_index = pin/10;
	unsigned int fsel_bitpos = pin%10;
	unsigned int* gpio_fsel = gpio_registers + fsel_index;
	unsigned int* gpio_on_register = (unsigned int*)((char*)gpio_registers + 0x1c);

	*gpio_fsel &= ~(7 << (fsel_bitpos*3));
	*gpio_fsel |= (1 << (fsel_bitpos*3));
	*gpio_on_register |= (1 << pin);
	
	return;	
}

static void gpio_pin_off(unsigned int pin)
{
	unsigned int *gpio_off_register = (unsigned int*)((char*)gpio_registers + 0x28);
	*gpio_off_register |= (1<<pin);
	return;
}

ssize_t kw_read(struct file *file, char __user *user, size_t size, loff_t *off)
{
	return copy_to_user(user,"Hello!\n", 7) ? 0 : 7;
}

ssize_t kw_write(struct file *file, const char __user *user, size_t size, loff_t *off)
{
	unsigned int pin = UINT_MAX;
	unsigned int value = UINT_MAX;
	
	memset(data_buffer, 0x0, sizeof(data_buffer));

	if (size > KW_MAX_USER_SIZE)
	{
		size = KW_MAX_USER_SIZE;	
	}	

	if (copy_from_user(data_buffer, user, size))
		return 0;
	
	printk("Data buffer: %s\n", data_buffer);

	if(sscanf(data_buffer, "%d,%d", &pin, &value) != 2)
	{
		printk("Improper format\n");
		return size;
	}

	if (pin > 21 || pin < 0)
	{
		printk("Invalid pin number\n");
		return size;
	}

	if (value !=0 && value !=1)
	{  
		printk("Invalid value\n");
		return size;
	}

	printk("Input: pin %d, value %d\n", pin, value);

	if (value == 1)
	{
		gpio_pin_on(pin);
	} else if (value == 0)
	{
		gpio_pin_off(pin);
	}
	
	return size;
}

static const struct proc_ops kw_proc_fops =
{
	.proc_read = kw_read,
	.proc_write = kw_write,
};


static int __init gpio_driver_init(void)
{
	printk("Welcome to my driver!\n");
	
	gpio_registers = (int*)ioremap(BCM2837_GPIO_ADDRESS, PAGE_SIZE);
	if(gpio_registers == NULL)
	{
		printk("Failed to map GPIO memory to driver\n");
		return -1;
	}
	printk("Succesfully mapped in GPIO memory\n");
	// create an entry in the proc-fs
	kw_proc = proc_create("kw-gpio", 0666, NULL, &kw_proc_fops);
	if (kw_proc == NULL)
	{
		return -1;
	}

	return 0;
}

static void __exit gpio_driver_exit(void)
{
	printk("Leaving my driver!\n");
	proc_remove(kw_proc);
	return;
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KW");
MODULE_DESCRIPTION("Test of writing drivers for RASPI");
MODULE_VERSION("1.0");
