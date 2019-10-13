// Copyright Queen's University Belfast 2012

#include <linux/module.h>
#include <linux/init.h>

// for init_net
#include <net/net_namespace.h>
#include <linux/netdevice.h>

MODULE_LICENSE ("GPL");

static char* s_netif_param = "eth0";
module_param (s_netif_param, charp, S_IRUGO);

static bool s_debug = true;
module_param (s_debug, bool, S_IRUGO);

static struct net_device* s_capture_netif = NULL;
static struct sk_buff* custom_hook(struct sk_buff *skb);
static struct sk_buff* custom_hook_new(struct sk_buff *skb);

// to avoid declaring in in the header file
extern void netif_set_custom_hook1 (struct sk_buff* (*)(struct sk_buff* skb));

static const size_t s_ip_udp_len = 28;

static bool is_long_enough (
  const char      msg_type,
  const uint32_t  msg_len,
  const bool      debug)
{

  bool return_value = false;

  switch (msg_type)
  {
    case 'A':
      if (msg_len == 42)
      {
        return_value = true;
      }
      break;
    case 'a':
      if (msg_len == 55)
      {
        return_value = true;
      }
      break;
    case 'X':
      if (msg_len == 24)
      {
        return_value = true;
      }
      break;
    case 'x':
      if (msg_len == 28)
      {
        return_value = true;
      }
      break;
    case 'E':
      if (msg_len == 33)
      {
        return_value = true;
      }
      break;
    case 'e':
      if (msg_len == 37)
      {
        return_value = true;
      }
      break;
    default:
      return_value = false;
      break;
  }

  return return_value;
}

static void print_as_hex (const char* str, const size_t str_len)
{
  size_t i = 0;
  for (; i < str_len; ++i)
  {
    if ((str[i] < '0') || (str[i] > 'z'))
    {
      printk (KERN_CRIT "%d: %2X", i, (uint8_t)str[i]);
    }
    else
    {
      printk (KERN_CRIT "%d: %2X %c", i, (uint8_t)str[i], str[i]);
    }
  }
}

static void print_as_hex_inline (const char* str, const size_t str_len)
{
  const int buf_len = 300;
  char  buffer [buf_len];
  memset (buffer, '\0', buf_len);
  char* buf_ptr = buffer;
  size_t i = 0;
  for (; (i < str_len) && ((buffer - buf_ptr) > -buf_len); ++i)
  {
    int written = sprintf (buf_ptr, "%2X ", (uint8_t)(str[i]));
    buf_ptr += written;
  }
  printk (KERN_CRIT "1:%s", buffer);

  memset (buffer, '\0', buf_len);
  buf_ptr = buffer;

  i = 0;
  for (; i < str_len; ++i)
  {
    int written = 0;
    if ((str[i] < '0') || (str[i] > 'z'))
    {
      written = sprintf (buf_ptr, "   ");
    }
    else
    {
      written = sprintf (buf_ptr, " %c ", str[i]);
    }
    buf_ptr += written;
  }
  printk (KERN_CRIT "2:%s", buffer);
}

#ifdef __x86_64__
__inline uint64_t gcc_rdtsc (void)
{
  uint64_t msr;

  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          : 
          : "rdx");

  return msr;
}
#else
__inline__ uint64_t bmk_rdtsc( void )
{
  uint64_t x;
  __asm__ volatile("rdtsc\n\t" : "=A" (x));
  return x;
}
#endif

static int __init init_func (void)
{
  // 1. build a list of net devices
  s_capture_netif = dev_get_by_name(&init_net, s_netif_param);

  if (s_capture_netif)
  {
    printk (KERN_CRIT "KB: timestamper - network interface found");
  }

  // 2. register hook
  netif_set_custom_hook1 (custom_hook_new);

  return 0; // success
}
#if 0
static struct sk_buff* custom_hook(struct sk_buff* skb)
{
  // 1. check if the device in question is the one that
  //    we have specified in configuration param
  if ((s_capture_netif != NULL) && (skb->dev != s_capture_netif))
  {
    return skb;
  }
  // 2. check the length of the packet
  // 3. check the type of the packet
  const size_t    md_msg_type_offset  = 8;
  const uint32_t  packet_len          = skb->len;

  // needs to make sure that we can at least reach
  // the market data msg type field
  if (packet_len < s_ip_udp_len + md_msg_type_offset + 1)
  {
    return skb;
  }

  unsigned char*  md_msg   = skb->data + s_ip_udp_len;

  // identify msg type
  const char            msg_type = md_msg[md_msg_type_offset];

  // check the length
  if (!is_long_enough (msg_type, packet_len - s_ip_udp_len, s_debug))
  {
    if (s_debug)
    {
      printk (KERN_CRIT "Msg of type %c is too short: only %d bytes, discarding packet", msg_type, packet_len - s_ip_udp_len);
      //print_as_hex_inline (skb->data, packet_len);
    }
    return skb;
  }
  // 4. if the type is good, replace the timestamp

  uint64_t timestamp = bmk_rdtsc ();

  memcpy (md_msg, &(timestamp), sizeof(timestamp));

  // set checksum to 0 to avoid errors
  uint8_t* csum_ptr = md_msg - 2;
  csum_ptr[0] = 0;
  csum_ptr[1] = 0;

  return skb;
}
#endif
static struct sk_buff* custom_hook_new (struct sk_buff* skb)
{
  // 1. check if the device in question is the one that
  //    we have specified in configuration param
  if ((s_capture_netif != NULL) && (skb->dev != s_capture_netif))
  {
    return skb;
  }
  // 2. check the length of the packet
  // 3. check the type of the packet
  // const size_t    md_msg_type_offset  = 8;
  const uint32_t  packet_len          = skb->len;

  // needs to make sure that we can at least reach
  // the market data msg type field
  const reserved_field_len           = 9;
  if (packet_len < s_ip_udp_len + reserved_field_len + 1)
  {
    return skb;
  }

  // check if this is "our" message
  unsigned char*  md_msg   = skb->data + s_ip_udp_len;
  int i = 0;
  for ( ; i < reserved_field_len; ++i)
  {
    if (md_msg[i] != 'R')
    {
      return skb;
    }
  }

  // identify msg type
/*
  const char            msg_type = md_msg[md_msg_type_offset];

  // check the length
  if (!is_long_enough (msg_type, packet_len - s_ip_udp_len, s_debug))
  {
    if (s_debug)
    {
      printk (KERN_CRIT "Msg of type %c is too short: only %d bytes, discarding packet", msg_type, packet_len - s_ip_udp_len);
      //print_as_hex_inline (skb->data, packet_len);
    }
    return skb;
  }
*/
  // 4. if the type is good, replace the timestamp

#ifdef __x86_64__
  uint64_t timestamp = gcc_rdtsc ();
#else
  uint64_t timestamp = bmk_rdtsc ();
#endif

  //printk ("KB: timestamp value is %lld\n", timestamp);

  memcpy (md_msg, &(timestamp), sizeof(timestamp));

  // set checksum to 0 to avoid errors
  uint8_t* csum_ptr = md_msg - 2;
  csum_ptr[0] = 0;
  csum_ptr[1] = 0;

  return skb;
}


static void __exit cleanup_func (void)
{
  // 1. deregister the hook
  netif_set_custom_hook1 (NULL);
  // 2. deallocate all allocated values, if any

}

module_init (init_func);
module_exit (cleanup_func);
