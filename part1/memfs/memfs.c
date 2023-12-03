#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define MEMFS_INVALID          0xFFFFFFFF
#define MEMFS_FILE_NAME_MAX    16
#define MEMFS_INODE_MAX        128
#define MEMFS_BLK_MAX          128
#define MEMFS_FILE_SIZE_MAX    1024

struct memfs_sb {
    uint32_t blk_size_bit;
    uint32_t block_size;
    uint32_t magic;
    uint32_t private;
};

struct memfs_inode{
    char file_name[MEMFS_FILE_NAME_MAX];
    uint32_t mode;
    uint32_t idx; 
    uint32_t child; // =MEMFS_INVALID: no child
    uint32_t brother; // =MEMFS_INVALID: no brother
    uint32_t file_size;
    uint32_t data; // =MEMFS_INVALID: dir    0~127: file
};

struct memfs_block{
    char data[1024];
};

struct memfs_sb g_mf_sb = {
    .blk_size_bit = 10,
    .block_size = 1024,
    .magic = 0x20221001,
};

char g_inode_bitmap[16]={0};
char g_block_bitmap[16]={0};

static struct memfs_inode  *g_mf_inode;
static struct memfs_block  *g_mf_block;

static struct inode_operations memfs_inode_ops;

void set_bitmap(char *bitmap, uint32_t index)
{
    *(bitmap + (index>>3)) |= (1<< (index%8));
}

void reset_bitmap(char *bitmap, uint32_t index)
{
    *(bitmap + (index>>3)) &= ~(1<< (index%8));
}

uint32_t get_idle_index(char *bitmap)
{
    uint8_t tmp;
    for(int i=0; i < 16; i++){
        if(bitmap[i] != 0xFF) {
            tmp = bitmap[i];
            for(int j=0;j<8;j++) {
                if((tmp & 0x1) == 0 ) {
                    set_bitmap(bitmap, i*8 + j);
                    return i*8 + j;
                }
                else {
                    tmp >>= 1;
                }
            }
        }
    }
    return MEMFS_INVALID;
}

void put_used_index(char *bitmap, uint32_t index) 
{
    reset_bitmap(bitmap, index);
}

int memfs_alloc_mem(void)
{
    g_mf_inode = kvzalloc(5120, GFP_KERNEL);
    if(!g_mf_inode)
        return -1;
    g_mf_block = kvzalloc(128*1024, GFP_KERNEL);
    if(!g_mf_block)
        return -1;

    for(int i=0;i<MEMFS_INODE_MAX;i++) {
        g_mf_inode[i].brother = MEMFS_INVALID;
        g_mf_inode[i].child = MEMFS_INVALID;
    }
    return 0;
}

void memfs_free_mem(void)
{
    kfree(g_mf_inode);
    kfree(g_mf_block);
}

// 读取目录的实现
static int memfs_readdir(struct file *filp, struct dir_context *ctx)
{
    struct memfs_inode *mf_inode, *child_inode;
    if (ctx->pos) 
        return 0;
    mf_inode = &g_mf_inode[filp->f_path.dentry->d_inode->i_ino];
    if (!S_ISDIR(mf_inode->mode)) {
        return -ENOTDIR;
    }
    if(mf_inode->child != MEMFS_INVALID) {
        child_inode = &g_mf_inode[mf_inode->child];
    }
    else {
        return 0;
    }
    while(child_inode->idx != MEMFS_INVALID) {
        if (!dir_emit(ctx, child_inode->file_name, MEMFS_FILE_NAME_MAX, child_inode->idx, DT_UNKNOWN)) {
            return 0;
        }
        ctx->pos += sizeof(struct memfs_inode);
        if(child_inode->brother != MEMFS_INVALID)
            child_inode = &g_mf_inode[child_inode->brother];
        else
            break;
    }
    return 0;
}

ssize_t memfs_read_file(struct file * filp, char __user * buf, size_t len, loff_t *ppos)
{
    struct memfs_inode *inode;
    char *buffer;
    inode = &g_mf_inode[filp->f_path.dentry->d_inode->i_ino];
    if (*ppos >= inode->file_size)
        return 0;
    buffer = (char*)&g_mf_block[inode->data];
    buffer += *ppos;
    len = min((size_t)(inode->file_size - *ppos), len);
    if (copy_to_user(buf, buffer, len)) {
        return -EFAULT;
    }
    *ppos += len;
    return len;
}

ssize_t memfs_write_file(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
    struct memfs_inode *inode;
    char *buffer;
    inode = &g_mf_inode[filp->f_path.dentry->d_inode->i_ino];
    if (*ppos + len > MEMFS_FILE_SIZE_MAX )
        return 0;
    buffer = (char*)&g_mf_block[inode->data];
    buffer += *ppos;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }
    *ppos += len;
    inode->file_size = *ppos;
    return len;
}

const struct file_operations memfs_file_operations = {
    .read = memfs_read_file,
    .write = memfs_write_file,
};
 
const struct file_operations memfs_dir_operations = {
    .owner = THIS_MODULE,
    .iterate_shared = memfs_readdir,
};

//dir: 当前目录的inode
//dentry：要创建的文件的dentry
//mode：要创建的文件的mode
static int memfs_do_create(struct user_namespace *mnt_userns, struct inode *dir, struct dentry *dentry, umode_t mode)
{
    struct inode *inode;
    struct super_block *sb;
    struct memfs_inode *mf_inode, *p_mf_inode, *tmp_mf_inode;
    uint32_t idx_inode;
    //获取sb指针
    sb = dir->i_sb;
    //判断是否是目录和常规文件，如果不是，返回错误
    if (!S_ISDIR(mode) && !S_ISREG(mode)) {
        return -EINVAL;
    }
    if (strlen(dentry->d_name.name) > MEMFS_FILE_NAME_MAX) {
        return -ENAMETOOLONG;
    }
    inode = new_inode(sb);
    if (!inode) {
        return -ENOMEM;
    }
    //初始化当前inode的sb
    inode->i_sb = sb;
    inode->i_op = &memfs_inode_ops; //初始化当前的inode的ops
    //初始化创建时间和修改时间为当前时间
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    //获取一个空闲的inode，用于保存当前创建的目录或者文件的inode信息
    idx_inode = get_idle_index(g_inode_bitmap);
    if (idx_inode == MEMFS_INVALID) {
        return -ENOSPC;
    }
    mf_inode = &g_mf_inode[idx_inode];
    mf_inode->idx = idx_inode;

    inode->i_ino = idx_inode;
    mf_inode->mode = mode;
    //初始化操作集合
    //如何是一个文件，则分配一个block，如果是一个目录则不用分配block
    if (S_ISDIR(mode)) {
        mf_inode->data = MEMFS_INVALID;
        inode->i_fop = &memfs_dir_operations;
    } else if (S_ISREG(mode)) {
        mf_inode->child = MEMFS_INVALID;
        mf_inode->file_size = 0;
        inode->i_fop = &memfs_file_operations;
        mf_inode->data = get_idle_index(g_block_bitmap);
        if(mf_inode->data == MEMFS_INVALID) {
            return -ENOSPC;
        }
    }
    //获取当前新创建的父目录节点
    p_mf_inode = &g_mf_inode[dir->i_ino];
    //当前目录是否为空目录
    if(p_mf_inode->child == MEMFS_INVALID) { //空目录
        p_mf_inode->child = mf_inode->idx;
    }
    else { //非空目录，找到最后一个child
        //找到父目录最后一个child
        tmp_mf_inode = &g_mf_inode[p_mf_inode->child];//第一个child
        while(tmp_mf_inode->brother != MEMFS_INVALID) {
            tmp_mf_inode = &g_mf_inode[tmp_mf_inode->brother];
        }
        //最后一个child，并设置brother
        tmp_mf_inode->brother = mf_inode->idx;
    }
    //初始化内核的dentry名称
    strcpy(mf_inode->file_name, dentry->d_name.name);
    //添加inode到dir中
    inode_init_owner(mnt_userns, inode, dir, mode);
    //绑定内核dentry与inode
    d_add(dentry, inode);
    return 0;
}

static int memfs_inode_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
			   struct dentry *dentry, umode_t mode)
{
    return memfs_do_create(mnt_userns, dir, dentry, S_IFDIR | mode);
}

static int memfs_inode_create(struct user_namespace *mnt_userns,struct inode *dir, struct dentry *dentry,
			    umode_t mode, bool excl)
{
    return memfs_do_create(mnt_userns, dir, dentry, mode);
}

//parent_inode: 父目录节点
//find_dentry: 要查找的dentry
static struct dentry *memfs_inode_lookup(struct inode *parent_inode, struct dentry *find_dentry, unsigned int flags)
{
    return NULL;
}

//删除空目录
int memfs_inode_rmdir(struct inode *dir, struct dentry *dentry)
{
    uint32_t index = dentry->d_inode->i_ino;
    struct memfs_inode *p_mf_inode, *child_mf_inode, *tmp_mf_inode;
    //如果是非空目录，返回错误
    if(g_mf_inode[index].child != MEMFS_INVALID)
        return -ENOTEMPTY;
    p_mf_inode = &g_mf_inode[dir->i_ino];
    //获取第一个child
    child_mf_inode = &g_mf_inode[p_mf_inode->child];
    put_used_index(g_inode_bitmap, index);
    if(p_mf_inode->child == index) {
        if(child_mf_inode->brother == MEMFS_INVALID)
            p_mf_inode->child = MEMFS_INVALID;
        else
            p_mf_inode->child = child_mf_inode->brother;
    }
    else {
        while(child_mf_inode->idx != MEMFS_INVALID) {
            if(child_mf_inode->brother != MEMFS_INVALID) {
                tmp_mf_inode = child_mf_inode;
                child_mf_inode = &g_mf_inode[child_mf_inode->brother];
                if(child_mf_inode->idx == index) {
                    if(child_mf_inode->brother != MEMFS_INVALID)
                        tmp_mf_inode->brother = child_mf_inode->brother;
                    else
                        tmp_mf_inode->brother = MEMFS_INVALID;
                    break;
                }
            }
        }
    }
    g_mf_inode[index].idx = MEMFS_INVALID;
    g_mf_inode[index].brother = MEMFS_INVALID;
    return simple_unlink(dir, dentry);
}

//删除文件操作
int memfs_inode_unlink(struct inode *dir, struct dentry *dentry) {
    uint32_t index = dentry->d_inode->i_ino;
    struct memfs_inode *p_mf_inode, *child_mf_inode, *tmp_mf_inode;
    p_mf_inode = &g_mf_inode[dir->i_ino];
    //获取第一个child
    child_mf_inode = &g_mf_inode[p_mf_inode->child];
    put_used_index(g_inode_bitmap, index);
    put_used_index(g_block_bitmap, g_mf_inode[index].data);
    if(p_mf_inode->child == index) {
        if(child_mf_inode->brother == MEMFS_INVALID)
            p_mf_inode->child = MEMFS_INVALID;
        else
            p_mf_inode->child = child_mf_inode->brother;
    }
    else {
        while(child_mf_inode->idx != MEMFS_INVALID) {
            if(child_mf_inode->brother != MEMFS_INVALID) {
                tmp_mf_inode = child_mf_inode;
                child_mf_inode = &g_mf_inode[child_mf_inode->brother];
                if(child_mf_inode->idx == index) {
                    if(child_mf_inode->brother != MEMFS_INVALID)
                        tmp_mf_inode->brother = child_mf_inode->brother;
                    else
                        tmp_mf_inode->brother = MEMFS_INVALID;
                    break;
                }
            }
        }
    }
    g_mf_inode[index].idx = MEMFS_INVALID;
    g_mf_inode[index].brother = MEMFS_INVALID;
    return simple_unlink(dir, dentry);
}

static struct inode_operations memfs_inode_ops = {
    .create = memfs_inode_create,
    .lookup = memfs_inode_lookup,
    .mkdir = memfs_inode_mkdir,
    .rmdir = memfs_inode_rmdir,
    .unlink = memfs_inode_unlink,
};

static int memfs_demo_fill_super (struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    int mode = S_IFDIR | 0755;
    root_inode = new_inode(sb);//新建inode，用于保存根节点
    root_inode->i_ino = 0; //设置根节点的编号为0
	//初始化根节点权限
    root_inode->i_mode = mode;
    //inode_init_owner(sb->s_user_ns, root_inode, NULL, mode);
    root_inode->i_sb = sb;//设置根节点的超级块
	//设置根节点的节点操作集合
    root_inode->i_op = &memfs_inode_ops;
	//设置根节点目录操作集合
    root_inode->i_fop = &memfs_dir_operations;
	//设置根节点的创建修改时间为当前时间
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    strcpy(g_mf_inode[0].file_name, "memfs");
    g_mf_inode[0].mode = mode;
    g_mf_inode[0].idx = 0;
    g_mf_inode[0].child = MEMFS_INVALID;
    g_mf_inode[0].brother = MEMFS_INVALID;
    set_bitmap(g_inode_bitmap, g_mf_inode[0].idx);
    root_inode->i_private = &g_mf_inode[0]; //将根节点保存到root_inode私有制作中
    sb->s_root = d_make_root(root_inode);
    sb->s_magic = g_mf_sb.magic;
    sb->s_blocksize_bits = g_mf_sb.blk_size_bit;
    sb->s_blocksize = g_mf_sb.blk_size_bit;
    return 0;
}

struct dentry *memfs_demo_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, memfs_demo_fill_super);
}

static void memfs_kill_sb(struct super_block *sb) {
  kill_anon_super(sb);
}

static struct file_system_type memfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "memfs",
	.mount      = memfs_demo_mount,
	.kill_sb	= memfs_kill_sb,
};

static int __init memfs_demo_init(void)
{
    if(memfs_alloc_mem()) {
        printk(KERN_ERR "alloc memory failed\n");
        return -ENOMEM;
    }
	return register_filesystem(&memfs_type);
}

static void __exit memfs_demo_exit(void)
{
    memfs_free_mem();
	unregister_filesystem(&memfs_type);
}

module_init(memfs_demo_init);
module_exit(memfs_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("1477153217@qq.com");
MODULE_DESCRIPTION("memory fs demo");
