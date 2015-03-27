#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/list.h>

#define DEV_NAME "pipe"

static int pipe_open(struct inode *, struct file *);
static int pipe_release(struct inode *, struct file *);
static ssize_t pipe_write(struct file *, const char __user *, size_t, loff_t *);
static ssize_t pipe_read(struct file *, char __user *, size_t, loff_t *);

static int major_number;
//static int pipe_write_busy;
//static int pipe_read_busy;
static int pipe_buf_size = 2048;

struct pipe_buf
{
	struct list_head node;
	int uid;
	int head;
	int tail;
	int count;
	char *buf;
	char pipe_write_busy;
	char pipe_read_busy;
};

static const struct file_operations pipe_fops = {
	.read = pipe_read,
	.write = pipe_write,
	.open = pipe_open,
	.release = pipe_release
};

DECLARE_WAIT_QUEUE_HEAD(queue);

static struct pipe_buf *buffers_list;

static int __init init_pipe(void)
{
	pr_info("Begin to run PIPE device\n");

	major_number = register_chrdev(0, DEV_NAME, &pipe_fops);
	if (major_number < 0) {
		pr_alert("Fail to register PIPE with error %d\n", major_number);
		return -ENODEV;
	}

	pr_info("Started with major number %d\n", major_number);

	buffers_list = kmalloc(sizeof(struct pipe_buf), GFP_KERNEL);
	INIT_LIST_HEAD(&buffers_list->node);
	buffers_list->uid = -1;
	buffers_list->buf = NULL;

	return 0;
}

static void __exit exit_pipe(void)
{
	struct pipe_buf *buf_curr;
	struct list_head *p;

	list_for_each(p, &buffers_list->node) {
		buf_curr = list_entry(p, struct pipe_buf, node);
		kfree(buf_curr->buf);
		list_del(&buf_curr->node);
		kfree(buf_curr);
	}
	unregister_chrdev(major_number, DEV_NAME);
	pr_info("Unload PIPE driver\n");
}

static int pipe_open(struct inode *id, struct file *f)
{
	char create_flag = 1;
	struct pipe_buf *buf_curr;
	int uid_curr;
	struct list_head *p;

	uid_curr = current_uid().val;

	pr_info("Opened PIPE by %d with mode %x by user with UID: %d\n",
		task_pid_nr(current), f->f_flags, uid_curr);

	list_for_each(p, &buffers_list->node) {
		buf_curr = list_entry(p, struct pipe_buf, node);
		if (buf_curr->uid == uid_curr) {
			create_flag = 0;
			break;
		}
	}

	if (create_flag) {
		buf_curr = kzalloc(sizeof(struct pipe_buf), GFP_KERNEL);
		buf_curr->buf = kmalloc(sizeof(pipe_buf_size), GFP_KERNEL);
		buf_curr->uid = uid_curr;
		INIT_LIST_HEAD(&buf_curr->node);
		list_add(&buf_curr->node, &buffers_list->node);
	} else {
		if ((f->f_flags & O_ACCMODE) == O_WRONLY) {
			if (buf_curr->pipe_write_busy == 1) {
				pr_info("PIPE write busy\n");
				return -EBUSY;
			}
			buf_curr->pipe_write_busy = 1;
		}

		if ((f->f_flags & O_ACCMODE) == O_RDONLY) {
			if (buf_curr->pipe_read_busy == 1) {
				pr_info("PIPE read busy\n");
				return -EBUSY;
			}
			buf_curr->pipe_read_busy = 1;
		}
	}

	f->private_data = buf_curr;

	return 0;
}

static int pipe_release(struct inode *id, struct file *f)
{
	struct pipe_buf *tmp;

	tmp = f->private_data;
	
	if ((f->f_flags & O_ACCMODE) == O_WRONLY)
		tmp->pipe_write_busy--;

	if ((f->f_flags & O_ACCMODE) == O_RDONLY)
		tmp->pipe_read_busy--;

	pr_info("Closed by %d\n", task_pid_nr(current));

	return 0;
}

static ssize_t pipe_write(
	struct file *f,
	const char __user *u_buf,
	size_t sz,
	loff_t *off
)
{
	struct pipe_buf *curr;
	unsigned int free_bytes, bytes_to_copy;

	curr = f->private_data;
	free_bytes = pipe_buf_size - curr->count;

	if (sz > free_bytes) {
		pr_info("PIPE: Not enough memory");
		return -ENOMEM;
	}

	bytes_to_copy = sz;

	while (bytes_to_copy) {
		if (curr->tail == pipe_buf_size)
			curr->tail = 0;
		if (get_user(curr->buf[curr->tail], u_buf)) {
			pr_alert("PIPE: error copy from userspace in pipe_write\n");
			return -EFAULT;
		}
		u_buf++;
		curr->tail++;
		curr->count++;
		bytes_to_copy--;
	}

	wake_up_interruptible(&queue);

	return sz;
}

static ssize_t pipe_read(
	struct file *f,
	char __user *u_buf,
	size_t sz,
	loff_t *off
)
{
	unsigned int bytes_to_read, really_sended = 0;
	struct pipe_buf *curr;

	curr = f->private_data;

	if (curr->count == 0)
		wait_event_interruptible(queue, curr->count > 0);

	if (curr->count >= sz)
		bytes_to_read = sz;
	else
		bytes_to_read = curr->count;

	while (bytes_to_read) {
		if (curr->head == pipe_buf_size)
			curr->head = 0;
		if (put_user(curr->buf[curr->head], u_buf)) {
			pr_alert("PIPE: error copy to userspace in pipe_read\n");
			return -EFAULT;
		}
		u_buf++;
		curr->head++;
		curr->count--;
		bytes_to_read--;
		really_sended++;
	}

	return (ssize_t) really_sended;
}

module_init(init_pipe);
module_exit(exit_pipe);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edgar");
MODULE_DESCRIPTION("Simple pipe driver");