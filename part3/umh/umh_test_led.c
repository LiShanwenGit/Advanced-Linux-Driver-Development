#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>       
#include <asm/io.h>          //含有ioremap函数iounmap函数
#include <asm/uaccess.h>     //含有copy_from_user函数和含有copy_to_user函数
#include <linux/device.h>    //含有类相关的设备函数
#include <linux/cdev.h>
#include <linux/platform_device.h> //包含platform函数
#include <linux/of.h>        //包含设备树相关函数
#include <linux/kobject.h>   //包含sysfs文件系统对象类
#include <linux/sysfs.h>     //包含sysfs操作文件函数
#include <linux/gpio/consumer.h> //包含gpio子系统接口
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/gpio.h>    //包含gpio一些宏
#include <linux/irq.h>       //含有IRQ_HANDLED和IRQ_TYPE_EDGE_RISING
#include <linux/interrupt.h> //含有request_irq、free_irq函数

static struct kobject *umh_test_obj;  //定义一个umh_test_obj

static ssize_t umh_show(struct kobject* kobjs,struct kobj_attribute *attr,char *buf)
{
    struct subprocess_info *info;
    char  app_path[] = "/bin/sh";
    char *envp[]={"HOME=/", "PATH=/sbin:/bin:/user/bin",NULL};
    char *argv_ss[]={app_path, "-c", """/bin/ls  /etc  > /dev/ttyS0""", NULL};
    char *argv[] = {"/bin/echo","off > /sys/devices/platform/mdev_hotplug/led", NULL, };
    call_usermodehelper(app_path, argv_ss, envp,UMH_WAIT_PROC);
    return 0;
}

static ssize_t umh_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
    struct subprocess_info *info;
    char app_path[]="/bin/echo";
    static char *argv[] = { "this is a test umh store\n", NULL, };
    printk(KERN_INFO"store\n");
    info = call_usermodehelper_setup(app_path, argv, NULL, GFP_KERNEL, NULL, NULL,NULL);
    call_usermodehelper_exec(info, UMH_NO_WAIT );
    return 1;
}

static struct kobj_attribute umh_test_attr = __ATTR(umh_test_attr, 0660, umh_show, umh_store);


static int __init umh_test_init(void)
{
    int ret;
    umh_test_obj = kobject_create_and_add("umh_test",NULL);
    if(umh_test_obj == NULL)
    {
        printk(KERN_INFO"create umh_test_kobj failed!\n");
        return -1;
    }
    ret = sysfs_create_file(umh_test_obj, &umh_test_attr.attr);
    if(ret != 0)
    {
        printk(KERN_INFO"create sysfs file failed!\n");
        return -1;
    }
    return 0;
}


static void __exit umh_test_exit(void)
{


}

module_init(umh_test_init);
module_exit(umh_test_exit);

MODULE_LICENSE("GPL");          //不加的话加载会有错误提醒
MODULE_AUTHOR("1477153217@qq.com");     //作者
MODULE_VERSION("0.1");          //版本
MODULE_DESCRIPTION("umh_test");  //简单的描述
