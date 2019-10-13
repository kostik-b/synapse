// Copyright Queen's University Belfast 2013

#ifndef CHIX_GLUE_H
#define CHIX_GLUE_H

#ifndef CLICK_LINUXMODULE

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define RESOLVE_SYSCALLS() 1

#else

#include <click/cxxprotect.h>
CLICK_CXX_PROTECT
#include <linux/kallsyms.h>
#include <asm/uaccess.h>
#include <linux/math64.h>
CLICK_CXX_UNPROTECT
#include <click/cxxunprotect.h>


#define RESOLVE_SYSCALLS() resolve_syscalls()

extern "C"
{

asmlinkage int  (*s_sys_msgget)     (key_t key, int msgflg) = NULL;
asmlinkage long (*s_sys_msgctl)     (int msqid, int cmd, void __user *buf) = NULL; // originally buf param is not void
asmlinkage long (*s_sys_msgsnd)     (int msqid, void __user *msgp, size_t msgsz, int msgflg) = NULL; // originally msgp param is not void

} // extern "C"

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

static int msgget(key_t key, int msgflg)
{
  return s_sys_msgget (key, msgflg);
}

static int msgctl(int msqid, int cmd, void* buf)
{
  return s_sys_msgctl (msqid, cmd, buf);
}

static int msgsnd(int msqid, const void* msgp, size_t msgsz, int msgflg)
{
  // save the current addr limit (which is 3GB)
  mm_segment_t oldfs = get_fs();
  // set the addr limit to 4GB (I think ...)
  set_fs(get_ds());
  int return_value = s_sys_msgsnd (msqid, msgp, msgsz, msgflg);
  // restore the previous addr limit
  set_fs (oldfs);

  return return_value;
}

extern "C"
{

int64_t __divdi3 (int64_t u, int64_t v)
{
  long c = 0;
  int64_t w;

  if (u < 0)
    {
      c = ~c;
      u = -u;
    }
  if (v < 0)
    {
      c = ~c;
      v = -v;
    }
  w = div64_u64 (u, v);
  if (c)
    w = -w;
  return w;
}


uint64_t __udivdi3 (uint64_t u, uint64_t v)
{
  return div64_u64 (u, v);
}

// this is a bad implementation, because I am
// reducing the size of integers
uint64_t
__umoddi3 (uint64_t u, uint64_t v)
{
  uint64_t w;

  // I am using this version because it is deemed to
  // be the fastest one
  iter_div_u64_rem (u, (uint32_t)v, &w);
  return w;
}

} // extern "C"

#endif // ifndef click_linuxmodule

#endif // header guard
