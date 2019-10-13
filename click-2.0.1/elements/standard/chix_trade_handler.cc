/*
 * print.{cc,hh} -- element prints packet contents to system log
 * John Jannotti, Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2008 Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/glue.hh>
#include <click/args.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "synapse_macros.h"
#ifdef CLICK_LINUXMODULE
# include <click/cxxprotect.h>
CLICK_CXX_PROTECT
# include <linux/sched.h>
CLICK_CXX_UNPROTECT
# include <click/cxxunprotect.h>
#endif
CLICK_DECLS

#include "chix_trade_handler.hh"
#include <click/appmsgs.hh>

using Synapse::MsgTrade;

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
      if (debug)
      {
        click_chatter ("Warning: unknown msg type %c", msg_type);
      }
      return_value = false;
      break;
  }

  return return_value;
}


ChixTradeHandler::ChixTradeHandler()
  : _debug              (false)
  , _active             (true)
  , _mac_ip_udp_len     (42)
{
  _cycles_counter.set_reporting_interval (100);
}

ChixTradeHandler::~ChixTradeHandler()
{
}

int
ChixTradeHandler::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("DEBUG",   _debug)
        .complete() >= 0)
  {
  }

  click_chatter ("chix_trade_handler configure called");

  return 0;
}

void ChixTradeHandler::parse_add_order (
  const unsigned char* md_msg,
  const OrderMsgType   order_msg_type)
{
  const int symbol_offset_short   = 25;
  const int symbol_len_short      = 6;

  const int symbol_offset_long    = 29;
  const int symbol_len_long       = symbol_len_short;

  const int shares_offset_short   = 19;
  const int shares_len_short      = 6;

  const int shares_offset_long    = shares_offset_short;
  const int shares_len_long       = 10;

  const int ref_num_offset        = 9;
  const int ref_num_len           = 9;

  const int price_offset_short    = 31;
  const int price_len_short       = 10;
  const int price_whole_len_short = 6;

  const int price_offset_long     = 35;
  const int price_len_long        = 19;
  const int price_whole_len_long  = 12;

  if (order_msg_type == SHORT)
  {
    do_parse_add_order<symbol_offset_short,
                    symbol_len_short,
                    shares_offset_short,
                    shares_len_short,
                    ref_num_offset,
                    ref_num_len,
                    price_offset_short,
                    price_len_short,
                    price_whole_len_short> (md_msg);
  }
  else if (order_msg_type == LONG)
  {
    do_parse_add_order<symbol_offset_long,
                    symbol_len_long,
                    shares_offset_long,
                    shares_len_long,
                    ref_num_offset,
                    ref_num_len,
                    price_offset_long,
                    price_len_long,
                    price_whole_len_long> (md_msg);
  }
}

void ChixTradeHandler::parse_cancel_order (
  const unsigned char* md_msg,
  const OrderMsgType   order_msg_type)
{
  const int ref_num_offset              = 9;
  const int ref_num_len                 = 9;

  const int cancel_shares_offset_short  = 18;
  const int cancel_shares_len_short     = 6;

  const int cancel_shares_offset_long   = cancel_shares_offset_short;
  const int cancel_shares_len_long      = 10;

  if (order_msg_type == SHORT)
  {
    do_parse_cancel_order <ref_num_offset, ref_num_len, cancel_shares_offset_short, cancel_shares_len_short> (md_msg);
  }
  else if (order_msg_type == LONG)
  {
    do_parse_cancel_order <ref_num_offset, ref_num_len, cancel_shares_offset_long, cancel_shares_len_long> (md_msg);
  }
}

void ChixTradeHandler::parse_execute_order (
  bool&                is_packet_reused,
  Packet&              packet,
  const unsigned char* md_msg,
  const OrderMsgType   order_msg_type)
{
  const int ref_num_offset                = 9;
  const int ref_num_len                   = 9;

  const int executed_shares_offset_short  = 18;
  const int executed_shares_len_short     = 6;

  const int executed_shares_offset_long   = executed_shares_offset_short;
  const int executed_shares_len_long      = 10;

  if (order_msg_type == SHORT)
  {
    do_parse_execute_order <ref_num_offset,
                            ref_num_len,
                            executed_shares_offset_short,
                            executed_shares_len_short> (is_packet_reused, packet, md_msg);
  }
  else if (order_msg_type == LONG)
  {
    do_parse_execute_order <ref_num_offset,
                            ref_num_len,
                            executed_shares_offset_long,
                            executed_shares_len_long> (is_packet_reused, packet, md_msg);
  }
}

void ChixTradeHandler::push(int port, Packet *p)
{
    if ((!_active) || (!p))
    {
      discard_packet(*p);
      return;
    }

    const size_t    md_msg_type_offset  = 8;
    const uint32_t  packet_len          = p->length();

    // needs to make sure that we can at least reach
    // the market data msg type field
    if (packet_len < _mac_ip_udp_len + md_msg_type_offset + 1)
    {
      discard_packet(*p);
      return;
    }

    const unsigned char*  md_msg   = p->data() + _mac_ip_udp_len;

    // a. if add order add to map
    // b. if cancel order, reduce shares and remove from the map if necessary
    // c. if executed order, generate msg trade and reduce shares or remove from
    //    the map

    // identify msg type
    const char            msg_type = md_msg[md_msg_type_offset];

    // check the length
    if (!is_long_enough (msg_type, packet_len - _mac_ip_udp_len, _debug))
    {
      if (_debug)
      {
        click_chatter ("Msg of type %c has incorrect length of %d bytes, discarding packet", msg_type, packet_len - _mac_ip_udp_len);
      }
      discard_packet(*p);
      return;
    }

    bool is_packet_reused = false;

    switch (msg_type)
    {
      case 'A':
        parse_add_order (md_msg, SHORT);
        REPORT_LATENCY (md_msg);
        break;
      case 'a':
        parse_add_order (md_msg, LONG);
        REPORT_LATENCY (md_msg);
        break;
      case 'X':
        parse_cancel_order(md_msg, SHORT);
        REPORT_LATENCY (md_msg);
        break;
      case 'x':
        parse_cancel_order (md_msg, LONG);
        REPORT_LATENCY (md_msg);
        break;
      case 'E':
        parse_execute_order (is_packet_reused, *p, md_msg, SHORT);
        REPORT_LATENCY (md_msg);
        break;
      case 'e':
        parse_execute_order (is_packet_reused, *p, md_msg, LONG);
        REPORT_LATENCY (md_msg);
        break;
      default:
        if (_debug)
        {
          click_chatter ("Warning: unknown msg type %c", msg_type);
        }
        break;
    }
    
    // discard the packet at last
    if (!is_packet_reused)
    {
      discard_packet(*p);
    }
}

/*
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
*/

void ChixTradeHandler::send_msg_trade (
  Packet&         packet,
  const char(&    symbol)[ORDER_SYMBOL_LEN],
  const fixedpt   price,
  const int64_t   shares,
  const uint64_t  timestamp)
{
  MsgTrade  msg;

  msg._price      = price;
  msg._size       = shares;
  msg._timestamp  = timestamp;

  memcpy (msg._symbol, symbol, ORDER_SYMBOL_LEN);

  if (_debug)
  {
/*
    click_chatter ("ChixTradeHandler: sending msg trade - symbol and shares are:");
    print_as_hex_inline (reinterpret_cast<char*>(&(msg._size)), sizeof(msg._size)); 
    print_as_hex_inline (reinterpret_cast<char*>(&(msg._price)), sizeof(msg._price)); 
    print_as_hex_inline (msg._symbol, sizeof(msg._symbol)); 
*/
  }

  // make sure we have enough room in the packet

  const size_t packet_size = packet.length ();
  const size_t msg_length  = sizeof (msg);

  // TODO: shift data instead in the future
  if (msg_length > packet_size)
  {
    if (_debug)
    {
      click_chatter ("ChixTradeHandler - can't send Trade Msg - packet not large enough");
    }
    discard_packet (packet);
    return;
  }

  WritablePacket* write_packet = packet.put (0);

  memcpy (write_packet->data(), reinterpret_cast<char*>(&msg), msg_length);

  write_packet->set_packet_app_type (Synapse::MSG_TRADE);

  checked_output_push (0, write_packet);
}

void
ChixTradeHandler::report_cycles (
 const unsigned char* md_msg)
{
  // turn that off for now
  return;
//-------------------------------
  const uint64_t        cycles_start  = *(reinterpret_cast<const uint64_t*>(md_msg));
  // get the timestamp
  const uint64_t        cycles_now    = ChixTradeHandlerUtils::bmk_rdtsc ();

  const bool            should_report =
    _cycles_counter.update_counter (cycles_now - cycles_start);

  static int            str_report_counter = 0;

  if (should_report)
  {
    click_chatter ("KB: Discard cycles counter is %lld", _cycles_counter.get_counter ());

    // each number should take no more than 10 chars
    ++str_report_counter;

    if (str_report_counter % 10 == 0)
    {
      int num_of_bytes = 200;
      int num_of_last_counters = 10;
      char last_ptrs_str [num_of_bytes];
      memset (last_ptrs_str, '\0', sizeof(last_ptrs_str));

      _cycles_counter.get_x_last_counters_str(last_ptrs_str,
                                              sizeof (last_ptrs_str) - 1,
                                              num_of_last_counters);

      click_chatter ("KB: last %d ptrs are %s", num_of_last_counters, last_ptrs_str);
    }

    _cycles_counter.reset_counter ();
  }
}

void
ChixTradeHandler::discard_packet (
  Packet&     packet)
{
  packet.kill();
}

void
ChixTradeHandler::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS


