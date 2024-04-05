#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0x19d04c07, "module_layout" },
	{ 0xaeafc33b, "i2c_del_driver" },
	{ 0x50f0c0dc, "i2c_unregister_device" },
	{ 0x936c6436, "i2c_put_adapter" },
	{ 0xafef394, "i2c_register_driver" },
	{ 0xe07991b4, "i2c_new_device" },
	{ 0x37d8436d, "i2c_get_adapter" },
	{ 0xc5850110, "printk" },
	{ 0xf9a482f9, "msleep" },
	{ 0xdecd0b29, "__stack_chk_fail" },
	{ 0xaefed8e7, "i2c_transfer_buffer_flags" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x8f678b07, "__stack_chk_guard" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("i2c:ETX_OLED");

MODULE_INFO(srcversion, "21CECB2F26A4629C49CB9D6");
