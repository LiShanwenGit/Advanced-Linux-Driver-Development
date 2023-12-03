#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>            
#include <linux/device.h> 
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>

//定义一个任务队列指针
static struct workqueue_struct *workqueue = NULL;
//定义一个任务
struct task_struct *thread_worker = NULL;
//定义一个工作项1
struct work_struct work1;
//定义一个工作项2
struct work_struct work2;

void work1_func(struct work_struct *work)
{
    printk(KERN_INFO "work1 execute!\n");
}

void work2_func(struct work_struct *work)
{
    printk(KERN_INFO "work2 execute!\n");
}

static int test_thread(void *data)
{
    while(!kthread_should_stop()) {
        //将work1放到工作队列中执行
        queue_work(workqueue,&work1);
        //将work2放到工作队列中执行
        queue_work(workqueue,&work2);
        msleep(1000);
    }
    return 0;
}

static int __init work_init(void)
{
    INIT_WORK(&work1, work1_func);
    INIT_WORK(&work2, work2_func);
    workqueue = create_singlethread_workqueue("wq_test");
    if(workqueue == NULL){
        return -1;
    }
    //创建一个线程
    thread_worker = kthread_run(test_thread, NULL, "test_kthread");
    if (IS_ERR(thread_worker)) {
        destroy_workqueue(workqueue);
        return PTR_ERR(thread_worker);
    }
    return 0;
}

static void __exit work_exit(void)
{
    kthread_stop(thread_worker);
    destroy_workqueue(workqueue);
    cancel_work_sync(&work1);
    cancel_work_sync(&work2);
}

module_init(work_init);
module_exit(work_exit);

MODULE_LICENSE("GPL");        
MODULE_AUTHOR("1477153217@qq.com");  
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("work queue demo");
