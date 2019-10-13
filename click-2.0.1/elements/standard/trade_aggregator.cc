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

#include "trade_aggregator.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

static int s_default_snaplen = 2046;

using Synapse::MsgTrade;
using Synapse::MsgTimePeriodStats;
using Synapse::SynapseElement;

TradeAggregator::TradeAggregator()
  : _timer                    (this)
  , _aggregation_interval_sec (10)
  , _debug                    (false)
  , _active                   (true)
{
}

TradeAggregator::~TradeAggregator()
{
}

int
TradeAggregator::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("AGGREGATION_INTERVAL_SEC", _aggregation_interval_sec)
        .read("DEBUG",                    _debug)
        .complete() >= 0)
  {
    click_chatter ("Aggregation interval is %d", _aggregation_interval_sec);
  }

  return 0;
}

int
TradeAggregator::initialize (
  ErrorHandler*)
{
  _timer.initialize(this);
  if (_active)
  {
    _timer.schedule_after_sec(_aggregation_interval_sec);
  }
  return 0;

}

void
TradeAggregator::push(
  int     port,
  Packet* p)
{
  if (!_active)
  {
    checked_output_push (port, p);
    return;
  }

  // check msg type
  if (!(p->get_packet_app_type () == Synapse::MSG_TRADE))
  {
    click_chatter ("Trade Aggregator - received an incorrect msg, passing on without modification");
    checked_output_push (port, p);
    return;
  }

  const MsgTrade* msg_trade = reinterpret_cast<const MsgTrade*>(p->data());

  if (_debug)
  {
    click_chatter ("Trade_aggregator: Received msg for symbol %s", msg_trade->_symbol);
  }

  // 1. find an element in the map (or create one)
  // TimeStats*  time_stats = get_time_stats (msg_trade->_symbol);
  TimeStats* time_stats = SynapseElement::get_or_create<TimeStatsMap> (_time_stats_map, msg_trade->_symbol);

  // just forward the msg if we couldn't process it
  if (!time_stats)
  {
    checked_output_push (port, p);
    return;
  }
  
  // 2. update its components
  update_stats (*time_stats, *msg_trade);
  
  // 3. destroy the msg
  SynapseElement::discard_packet  (*p);
}

void
TradeAggregator::update_stats (
  TimeStats&      time_stats,
  const MsgTrade& msg_trade)
{
  // special case - it's a first update
  if (!time_stats._first_time_updated)
  {
    time_stats.reset ();

    time_stats._open                = msg_trade._price;
    time_stats._high                = msg_trade._price;
    time_stats._low                 = msg_trade._price;
    time_stats._close = msg_trade._price;
    time_stats._size                = msg_trade._size;

    time_stats._first_time_updated  = true;

    return;
  }

  if (msg_trade._price > time_stats._high)
  {
    time_stats._high = msg_trade._price;
  }

  if (msg_trade._price < time_stats._low)
  {
    time_stats._low  = msg_trade._price;
  }

  time_stats._size += msg_trade._size;
  time_stats._close = msg_trade._price;
}

void TradeAggregator::send_stats_msg (
  const SymbolArrayWrapper& symbol,
  const TimeStats&          stats)
{
  // TODO: need to have a justification for s_default_snaplen var
  WritablePacket *p = Packet::make(Packet::default_headroom, 0, s_default_snaplen, 0);

  // no aggregation for now - just issue a new msg
  MsgTimePeriodStats msg_stats;

  // make sure there is enough room for stats msg
  const size_t stats_msg_size = sizeof (msg_stats);
  const size_t data_len       = p->length ();

  if (stats_msg_size > data_len)
  {
    click_chatter ("Trade aggregator - stats msg too small - cannot send!");
    SynapseElement::discard_packet (*p);
    return;
  }

  msg_stats._high   = stats._high;
  msg_stats._low    = stats._low;
  msg_stats._open   = stats._open;
  msg_stats._close  = stats._close;

  msg_stats._size   = stats._size;

  memcpy (msg_stats._symbol, symbol.array_ptr(), Synapse::ORDER_SYMBOL_LEN);

  // copy the msg to the packet
  memcpy (p->data(), reinterpret_cast<char*>(&msg_stats), stats_msg_size);
  // set new packet type
  p->set_packet_app_type (Synapse::MSG_TIME_PERIOD_STATS);

  checked_output_push (0, p);
  return;
}

void
TradeAggregator::run_timer (
  Timer*  timer)
{
  // flush files periodically
  for (TimeStatsMap::iterator i = _time_stats_map.begin(); i.live(); ++i)
  {
    send_stats_msg(i.key(), i.value());

    i.value()._first_time_updated = false;
  }

  _timer.reschedule_after_sec (_aggregation_interval_sec);
}

void
TradeAggregator::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
ELEMENT_MT_SAFE(TradeAggregator)
