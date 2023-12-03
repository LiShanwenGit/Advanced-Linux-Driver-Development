#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>

//定义一个任务
struct task_struct *thread_worker;
//定义一个工作项
struct work_struct work;

void work_func(struct work_struct *work)
{
    printk(KERN_INFO "work execute!\n");
}

static int test_thread(void *data)
{
    while(!kthread_should_stop()) {
        schedule_work(&work);
        msleep(1000);
    }
    return 0;
}

static int __init work_init(void)
{
    INIT_WORK(&work, work_func);
    //创建一个线程
    thread_worker = kthread_run(test_thread, NULL, "test_kthread");
    if (IS_ERR(thread_worker)) {
        return PTR_ERR(thread_worker);
    }
    return 0;
}

static void __exit work_exit(void)
{
    kthread_stop(thread_worker);
    cancel_work_sync(&work);
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");        
MODULE_AUTHOR("1477153217@qq.com");  
MODULE_VERSION("0.1");      
MODULE_DESCRIPTION("work demo");
