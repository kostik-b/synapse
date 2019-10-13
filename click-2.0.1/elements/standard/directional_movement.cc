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

#include "directional_movement.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"
#include <click/master.hh>

using Synapse::MsgTrade;
using Synapse::MsgIndicator;
using Synapse::SynapseElement;
using Synapse::ORDER_SYMBOL_LEN;

DirectionalMovement::DirectionalMovement()
  : _interval_len       (0)
  , _debug              (false)
  , _active             (true)
{
}

DirectionalMovement::~DirectionalMovement()
{
}

int
DirectionalMovement::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("INTERVAL_LEN", _interval_len)
        .read("DEBUG", _debug)
        .complete() >= 0)
  {
    click_chatter ("DirectionalMovement: configured interval len is %lld", _interval_len);
  }

  return 0;
}

void
DirectionalMovement::push (
  int     port,
  Packet* p)
{
  if (!_active)
  {
      checked_output_push (0, p);
      return;
  }

  // 1. get the time period message
  if (p->get_packet_app_type () != Synapse::MSG_TRADE)
  {
    click_chatter ("DirectionalMovement - incorrect msg type");
    checked_output_push (0, p);
    return;
  }

  // 1. Get the symbol
  const MsgTrade* msg = reinterpret_cast<const MsgTrade*>(p->data());

  assert (sizeof(*msg) <= p->length ());

  // 2. Fetch an entry from the hash map
  bool created = false;
  HighLow* high_low = 
        SynapseElement::get_or_create<HighLowMap> (_high_low_map, created, msg->_symbol);

  // 3. Get trade's time, subtract start time and divide by a,
  //    this is an interval i
  const int64_t interval_num = int64_t(msg->_timestamp - master()->get_start_time_as_cycles ()) / 
                                          int64_t(_interval_len);
  // if the trade originated before our system was started
  if (interval_num < 0)
  {
    SynapseElement::discard_packet (*p);
    return;
  }

  if (_debug)
  {
    click_chatter ("Directional Movement: precheck symbol %s, price is %d, intval is %lld, cint is %lld, savedint is %lld",
                    msg->_symbol, msg->_price, interval_num, high_low->_current_interval_num, high_low->_prev_interval_num);
  }
  // 4. does current interval have i?
  //    YES, update the value (High/Low)
  //    NO, copy current interval to previous,
  //        copy the trade value into value (High/Low)
  if (high_low->_current_interval_num == interval_num)
  {
    // update high
    if (msg->_price > high_low->_current_high)
    {
      high_low->_current_high = msg->_price;
    }
    // update low
    if (msg->_price < high_low->_current_low)
    {
      high_low->_current_low = msg->_price;
    }
  }
  else
  {
    high_low->_prev_low            = high_low->_current_low;
    high_low->_prev_high           = high_low->_current_high;
    high_low->_prev_interval_num   = high_low-> _current_interval_num;

    high_low->_current_interval_num= interval_num;
    high_low->_current_low         = msg->_price;
    high_low->_current_high        = msg->_price;
  }
  // 5. Does previous interval have a value?
  //    YES, compute DM and publish
  //    NO, discard trade

  if ((high_low->_prev_low == 0) && (high_low->_prev_high == 0))
  {
    SynapseElement::discard_packet (*p);
    return;
  }

  fixedpt plus_dm    = 0;
  fixedpt minus_dm   = 0;

  calculate_dm (plus_dm,
                minus_dm,
                high_low->_prev_high,
                high_low->_prev_low,
                high_low->_current_high,
                high_low->_current_low);

  if (_debug)
  {
    click_chatter ("Directional Movement: postcheck symbol %s, price is %d, plus_dm is %d, minus_dm is %d",
                    msg->_symbol, msg->_price, plus_dm, minus_dm);
    click_chatter ("Directional Movement: phigh %d, plow %d, chigh %d, clow %d",
                    high_low->_prev_high, high_low->_prev_low, high_low->_current_high,
                    high_low->_current_low);

  }

  // 6a. save symbol and timestamp
  char symbol_copy [ORDER_SYMBOL_LEN];
  memcpy (symbol_copy, msg->_symbol, ORDER_SYMBOL_LEN);
  const uint64_t timestamp_copy = msg->_timestamp;
  // 6b. clone a packet
  // 7. Original packet is -DM, cloned is +DM
  WritablePacket* plus_dm_packet = p->clone ()->uniqueify ();
  // 8. populate both packets.
  // 9. push -DM through port 0 and +DM through port 1
  if (!SynapseElement::populate_msg_indicator(*p, symbol_copy, minus_dm, timestamp_copy))
  {
    SynapseElement::discard_packet (*p);
  }
  else
  {
    checked_output_push (0, p);
  }
  if (!SynapseElement::populate_msg_indicator(*plus_dm_packet, symbol_copy, plus_dm, timestamp_copy))
  {
    SynapseElement::discard_packet (*plus_dm_packet);
  }
  else
  {
    checked_output_push (1, plus_dm_packet);
  }
}

void
DirectionalMovement::calculate_dm (
    fixedpt&      plus_dm,
    fixedpt&      minus_dm,
    const fixedpt previous_high,
    const fixedpt previous_low,
    const fixedpt current_high,
    const fixedpt current_low)
{
  fixedpt up_move    = fixedpt_sub(current_high, previous_high);
  fixedpt down_move  = fixedpt_sub(previous_low, current_low);

  // +dm
  if ((up_move > down_move) && (up_move > 0))
  {
    plus_dm = up_move;
  }
  else
  {
    plus_dm = 0;
  }

  // -dm
  if ((down_move > up_move) && (down_move > 0))
  {
    minus_dm = down_move;
  }
  else
  {
    minus_dm = 0;
  }
}

void
DirectionalMovement::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DirectionalMovement)
