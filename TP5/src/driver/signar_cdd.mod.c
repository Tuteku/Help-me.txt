#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xfe990052, "gpio_free" },
	{ 0x7d958a42, "device_destroy" },
	{ 0x4a41ecb3, "class_destroy" },
	{ 0x607587f4, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x55bf416, "gpio_to_desc" },
	{ 0xa23da56e, "gpiod_get_raw_value" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x3142be5e, "kmalloc_caches" },
	{ 0x323febe6, "__kmalloc_cache_noprof" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x5d9d9fd4, "cdev_init" },
	{ 0xcc335c1c, "cdev_add" },
	{ 0xf311fc60, "class_create" },
	{ 0xab5c9881, "device_create" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xeb6dc7f8, "gpiod_direction_input" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CE854D39E7214824BFAD969");
