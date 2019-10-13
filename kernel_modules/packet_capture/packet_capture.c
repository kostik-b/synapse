// Copyright Queen's University Belfast 2012

#include <linux/module.h>
#include <linux/init.h>

// for init_net
#include <net/net_namespace.h>
#include <linux/netdevice.h>

#include <shmq/shmq.h>

MODULE_LICENSE ("GPL");

static char* s_netif_param = "eth0";
module_param (s_netif_param, charp, S_IRUGO);

static struct net_device* s_capture_netif = NULL;
static struct sk_buff* custom_hook(struct sk_buff *skb);

// to avoid declaring in in the header file
extern void netif_set_custom_hook1 (struct sk_buff* (*)(struct sk_buff* skb));

static int __init init_func (void)
{
  // 1. build a list of net devices
  s_capture_netif = dev_get_by_name(&init_net, s_netif_param);

  if (s_capture_netif)
  {
    printk (KERN_CRIT "KB_packet_capture - network interface found");
  }

  // 2. register hook
  netif_set_custom_hook1 (custom_hook);

  return 0; // success
}

static struct sk_buff* custom_hook(struct sk_buff* skb)
{
  // 1. check if the device in question is the one that
  //    we have specified in configuration param
  if ((s_capture_netif != NULL) && (skb->dev != s_capture_netif))
  {
    return skb;
  }

  struct sk_buff* nskb = skb; //skb_clone(skb, GFP_ATOMIC);

  // make sure that mac header is also included in the data
  // section
  skb_push(nskb, nskb->data - skb_mac_header(nskb));

  shm_queue_send_msg (nskb->data, nskb->len);

  kfree_skb(nskb);

  return NULL;
}

static void __exit cleanup_func (void)
{
  // 1. deregister the hook
  netif_set_custom_hook1 (NULL);
}

module_init (init_func);
module_exit (cleanup_func);
