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
#ifdef CLICK_LINUXMODULE
# include <click/cxxprotect.h>
CLICK_CXX_PROTECT
# include <linux/sched.h>
CLICK_CXX_UNPROTECT
# include <click/cxxunprotect.h>
#endif
CLICK_DECLS

#include "true_range.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"
#include <click/master.hh>

using Synapse::MsgIndicator;
using Synapse::MsgTrade;
using Synapse::SynapseElement;

TrueRange::TrueRange()
  : _interval_len (0)
  , _debug        (false)
  , _active       (true)
{
}

TrueRange::~TrueRange()
{
}

int
TrueRange::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("INTERVAL_LEN", _interval_len)
        .read("DEBUG", _debug)
        .complete() >= 0)
  {
    click_chatter ("TrueRange: configured interval len is %lld", _interval_len);
  }

  return 0;
}

void
TrueRange::push (
  int     port,
  Packet* p)
{
  if (!_active)
  {
      checked_output_push (0, p);
      return;
  }

  // 1. make sure that the msg is the right one
  if (p->get_packet_app_type () != Synapse::MSG_TRADE)
  {
    click_chatter ("DirectionalMovement - incorrect msg type");
    checked_output_push (0, p);
    return;
  }

  // similar to directional movement
  // 1. get symbol
  // 2. fetch the record from the hashmap
  const MsgTrade* msg = reinterpret_cast<const MsgTrade*>(p->data());

  assert (sizeof(*msg) <= p->length ());

  // 2. lookup the symbol in question in the map
  //    if symbol is not there create it and return
  bool    created         = false;
  HighLowClose* high_low_close  =
        SynapseElement::get_or_create<HighLowCloseMap> (_high_low_close_map, created, msg->_symbol);

  // 3. calculate interval number i
  const int64_t interval_num = int64_t(msg->_timestamp - master()->get_start_time_as_cycles ()) /
                                          int64_t(_interval_len);

  if (_debug)
  {
    click_chatter ("TrueRange: precheck symbol %s, price %d", msg->_symbol, msg->_price);
  }
  // 4a. if i is current
  //    YES - update current low, high and close
  //    NO  - rollover close and current interval num
  //     update current interval num, low, high and close
  if (high_low_close->_current_interval_num == interval_num)
  {
    if (msg->_price > high_low_close->_current_high)
    {
      high_low_close->_current_high = msg->_price;
    }
    if (msg->_price < high_low_close->_current_low)
    {
      high_low_close->_current_low = msg->_price;
    }
    high_low_close->_current_close = msg->_price;
  }
  else
  {
    high_low_close->_prev_interval_num  = high_low_close->_current_interval_num;
    high_low_close->_prev_close         = high_low_close->_current_close;

    high_low_close->_current_close         = msg->_price;
    high_low_close->_current_high          = msg->_price;
    high_low_close->_current_low           = msg->_price;
    high_low_close->_current_interval_num  = interval_num;
  }
  // 4b. if previous is nonexistent, discard msg and return
  if (high_low_close->_prev_close == 0)
  {
    SynapseElement::discard_packet (*p);
    return;
  }
  // 5. call calculate function and publish msg indicator
  const fixedpt true_range = calculate_true_range (high_low_close->_current_high,
                                                   high_low_close->_current_low,
                                                   high_low_close->_prev_close);

  if (_debug)
  {
    click_chatter ("TrueRange: postcheck sym %s, tstamp %lld, price %d, trange %d", msg->_symbol,
                    msg->_timestamp, msg->_price, true_range);
    click_chatter ("TrueRange: chigh %d, clow %d, pclose %d",
                    high_low_close->_current_high, high_low_close->_current_low, high_low_close->_prev_close);
  }

  char symbol_copy [Synapse::ORDER_SYMBOL_LEN];
  memcpy (symbol_copy, msg->_symbol, Synapse::ORDER_SYMBOL_LEN);

  if (!SynapseElement::populate_msg_indicator(*p, symbol_copy, true_range, msg->_timestamp))
  {
    SynapseElement::discard_packet (*p);
  }
  else
  {
    checked_output_push (0, p);
  }
}

const fixedpt
TrueRange::calculate_true_range (
  const fixedpt  high,
  const fixedpt  low,
  const fixedpt  previous_close)
{
  const fixedpt high_low   = fixedpt_sub(high, low);
  const fixedpt high_close = fixedpt_sub(high, previous_close);
  const fixedpt close_low  = fixedpt_sub(previous_close, low);

  const fixedpt max1       = high_low > high_close ? high_low : high_close;

  return (max1 > close_low) ? max1 : close_low;  
}

void
TrueRange::add_handlers()
{
  add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TrueRange)
