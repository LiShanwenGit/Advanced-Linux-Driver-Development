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
//定义一个延时工作项
struct delayed_work  delay_work;

static void work_handle(struct work_struct *work)
{
    printk(KERN_INFO "delay work execute!\n");
}

static int test_thread(void *data)
{
    while(!kthread_should_stop()) {
        msleep(2000); //睡眠2000ms
        //延迟500ms执行work
        schedule_delayed_work(&delay_work, msecs_to_jiffies(500));            
    }
    return 0;
}

static int __init delay_work_init(void)
{
    INIT_DELAYED_WORK(&delay_work, work_handle);
    //创建一个线程
    thread_worker = kthread_run(test_thread, NULL, "test_kthread");
    if (IS_ERR(thread_worker)) {
        return PTR_ERR(thread_worker);
    }
    return 0;
}

static void __exit delay_work_exit(void)
{
    kthread_stop(thread_worker);
    cancel_delayed_work_sync(&delay_work);
}

module_init(delay_work_init);
module_exit(delay_work_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("delay work demo");
