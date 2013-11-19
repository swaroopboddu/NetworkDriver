#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xa8c16cf3, "module_layout" },
	{ 0x15692c87, "param_ops_int" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x2f0270b3, "inet_del_protocol" },
	{ 0xdd9e7ec7, "__register_chrdev" },
	{ 0x2180dbed, "inet_add_protocol" },
	{ 0x1616b4e5, "dev_get_by_name" },
	{ 0x28318305, "snprintf" },
	{ 0x37a0cba, "kfree" },
	{ 0x69acdf38, "memcpy" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x91a3cdf4, "down_timeout" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x71e3cecb, "up" },
	{ 0xf22449ae, "down_interruptible" },
	{ 0x1b6314fd, "in_aton" },
	{ 0x85df9b6c, "strsep" },
	{ 0x783c7933, "kmem_cache_alloc_trace" },
	{ 0x352091e6, "kmalloc_caches" },
	{ 0xd98b3288, "ip_local_out" },
	{ 0x58910fc0, "ip_route_output_flow" },
	{ 0x1c62700c, "init_net" },
	{ 0x68093a17, "skb_push" },
	{ 0x128077ac, "skb_put" },
	{ 0x2dcf5a98, "__alloc_skb" },
	{ 0x27e1a049, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1F1A785E85F224EE8258EB5");
