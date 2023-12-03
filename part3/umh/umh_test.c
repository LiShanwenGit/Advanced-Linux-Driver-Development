#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>       
#include <linux/kobject.h>  
#include <linux/sysfs.h>     

static struct kobject *umh_test_obj;  //定义一个umh_test_obj

static ssize_t umh_show(struct kobject* kobjs,struct kobj_attribute *attr,char *buf)
{
    struct subprocess_info *info;
    char  app_path[] = "/bin/sh";
    char *envp[]={"HOME=/", "PATH=/sbin:/bin:/user/bin",NULL};
    char *argv[]={app_path, "-c", """/bin/ls  /etc  > /dev/ttyS0""", NULL};
    info = call_usermodehelper_setup(app_path, argv, envp, GFP_KERNEL, NULL, NULL,NULL);
    call_usermodehelper_exec(info, UMH_WAIT_PROC);
    return 0;
}

static struct kobj_attribute umh_test_attr = __ATTR(umh_test_attr, 0660, umh_show, NULL);


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
    sysfs_remove_file(umh_test_obj, &umh_test_attr.attr);
    kobject_put(umh_test_obj);
}

module_init(umh_test_init);
module_exit(umh_test_exit);


MODULE_LICENSE("GPL");      
MODULE_AUTHOR("1477153217@qq.com");   
MODULE_VERSION("0.1");          
MODULE_DESCRIPTION("umh_test"); 


