// Copyright Queen's University Belfast 2012

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>

#include "shmq_msg.h"
#include "shmq.h"

MODULE_LICENSE ("GPL");

static int s_msg_queue_key_param = 121314;
module_param (s_msg_queue_key_param, int, S_IRUGO);


static asmlinkage int  (*s_sys_msgget)     (key_t key, int msgflg) = NULL;
static asmlinkage long (*s_sys_msgctl)     (int msqid, int cmd, struct msqid_ds __user *buf) = NULL;
static asmlinkage long (*s_sys_msgsnd)     (int msqid, struct msgbuf __user *msgp, size_t msgsz, int msgflg) = NULL;

static            void send_msg_from_queue (void* param);

static int                      s_msg_queue_id        = 0;
static struct kfifo             s_ring_buffer;
static struct workqueue_struct* s_workqueue           = NULL;

DECLARE_WORK (s_msg_sender_work, send_msg_from_queue);

static size_t s_max_msg_size = 0;

static bool resolve_syscalls ()
{
  unsigned long sys_msgget_address = 0;
  unsigned long sys_msgctl_address = 0;
  unsigned long sys_msgsnd_address = 0;

  // find sys_msgget, sys_msgsnd and sys_msgctl
  sys_msgget_address = kallsyms_lookup_name ("sys_msgget");
  
  if (!sys_msgget_address)
  {
    printk( KERN_INFO "ShmQ-Kernel : Unable to find sys_msgget function\n" );
    return false;
  }

  s_sys_msgget = sys_msgget_address;

  sys_msgctl_address = kallsyms_lookup_name ("sys_msgctl");

  if (!sys_msgctl_address)
  {
    printk( KERN_INFO "ShmQ-Kernel : Unable to find sys_msgctl function\n" );
    return false;
  }

  s_sys_msgctl = sys_msgctl_address;

  sys_msgsnd_address = kallsyms_lookup_name ("sys_msgsnd");

  if (!sys_msgsnd_address)
  {
    printk( KERN_INFO "ShmQ-Kernel : Unable to find sys_msgsnd function\n" );
    return false;
  }

  s_sys_msgsnd = sys_msgsnd_address;

  return true;
}

static int __init init_func (void)
{
  if (!resolve_syscalls ())
  {
    return -1;
  }

  s_msg_queue_id = s_sys_msgget( s_msg_queue_key_param, 0666 | IPC_CREAT );

  if( s_msg_queue_id < 0 )
  {
    printk( KERN_INFO "ShmQ-Kernel : Unable to obtain s_msg_queue_id, it's %d\n", s_msg_queue_id );
    return -1;
  }
  
  printk( KERN_INFO "ShmQ-Kernel : s_msg_queue_id is %d\n", s_msg_queue_id );

  // set up the kfifo
  if (kfifo_alloc (&s_ring_buffer, 2048, GFP_KERNEL) < 0) // allocates an internal buffer 2028 bytes long
  {
    printk( KERN_INFO "ShmQ-Kernel : Couldn't allocate ring buffer\n");
    return -1;
  }

  // set up the work queue that will attempt to read the messages
  // from the kfifo
  s_workqueue = create_singlethread_workqueue ("shmq_kernel");

  if (!s_workqueue)
  {
    printk (KERN_INFO "ShmQ-Kernel : Could not allocate workqueue");
  }

  // submit the work to workqueue
  queue_work (s_workqueue, &s_msg_sender_work);

  return 0; // success
}

static bool           cancel_work         = false;
static unsigned int   work_sleep_interval = 1; // I imply it's a millisecond

static void send_msg_from_queue (void* param)
{
  struct shmq_msg msg;

  printk ("KB: entered send_msg_from_queue");

  while (!cancel_work)
  {
    // 1. check if there is at least one msg in a buffer
    //    -if not, then sleep for the sleep interval
    while (kfifo_len (&s_ring_buffer) < sizeof (struct shmq_msg))
    {
      msleep (work_sleep_interval);

      if (cancel_work)
        goto exit;
    }
    // 2. remove that msg
    kfifo_out (&s_ring_buffer, &msg, sizeof (struct shmq_msg));
    // 3. place it on system queue
    s_sys_msgsnd(s_msg_queue_id, (struct shmq_msg*)&msg, sizeof(msg._msg_data_struct), 0);
    // Job done!
  }

exit:
  printk ("KB: send_msg_from_queue - exiting");
  return;
}

// this function simply places the message onto the queue
int shm_queue_send_msg (
  const char*   msg,
  const size_t  msg_size)
{
  struct shmq_msg buf;
  int             result    = 0;
  size_t          copy_size = 0;

  if (cancel_work)
  {
    return 0;
  }

  // this is just for info
  if (msg_size > s_max_msg_size)
  {
    s_max_msg_size = msg_size;
    printk (KERN_INFO "KB: new msg max size is %d", s_max_msg_size);
  }

  memset (&buf, '\0', sizeof(buf));
  buf._mtype = SHMQ_MSG_KERNEL_TYPE;

  copy_size = (msg_size >= SHMQ_MSG_MAX_DATA_LEN ? SHMQ_MSG_MAX_DATA_LEN - 1 : msg_size);

  // first save size
  buf._msg_data_struct._size = copy_size;
  // then write the actual data
  memcpy (buf._msg_data_struct._data, msg, copy_size);
  result  = kfifo_in (&s_ring_buffer, &buf, sizeof(buf));

  return result;
}
EXPORT_SYMBOL (shm_queue_send_msg);

static void __exit cleanup_func (void)
{
  int result  = 0;

  cancel_work = true;

  msleep (work_sleep_interval * 3); // just to be sure

  cancel_work_sync    (&s_msg_sender_work);
  flush_workqueue     (s_workqueue);
  destroy_workqueue   (s_workqueue);

  result = s_sys_msgctl( s_msg_queue_id, IPC_RMID, NULL );
  if( result < 0 )
  {
    printk( KERN_INFO
        "ShmQ-Kernel : Unable to remove message queue from system, return code is %d\n", result );
  }

  kfifo_free          (&s_ring_buffer);
}

module_init (init_func);
module_exit (cleanup_func);
