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

#include "aroon_indicator.hh"
#include <click/appmsgs.hh>

using Synapse::MsgTimePeriodStats;

AroonIndicatorElement::AroonIndicatorElement()
  : _timer                  (this)
  , _file_flush_interval_sec(10)
  , _active                 (true)
{
}

AroonIndicatorElement::~AroonIndicatorElement()
{
}

int
AroonIndicatorElement::configure(Vector<String> &conf, ErrorHandler* errh)
{
 if (Args(conf)
        .read("FILE_FLUSH_INTERVAL",  _file_flush_interval_sec)
        .complete() >= 0)
  {
    click_chatter ("File flush interval is %d", _file_flush_interval_sec);
  }


  return 0;
}

int
AroonIndicatorElement::initialize (
  ErrorHandler*)
{
  _timer.initialize(this);
  if (_active)
  {
    _timer.schedule_after_sec(_file_flush_interval_sec);
  }
  return 0;

}

Packet *
AroonIndicatorElement::simple_action(Packet *p)
{
  if (!_active)
  {
      return p;
  }

  // 1. check if the msg type is correct

  if (p->get_packet_app_type () != Synapse::MSG_TIME_PERIOD_STATS)
  {
    click_chatter ("Aroon Indicator - incorrect msg type");
    return p;
  }
  // 2. get time period stats message
  const MsgTimePeriodStats* msg = reinterpret_cast<const MsgTimePeriodStats*>(p->data());

  // 3. fetch the aroon indicator for symbol,
  //    if absent, then create it
  SymbolDataHolder* symbol_data_holder = get_symbol_data (msg->_symbol);

  if (!symbol_data_holder)
  {
    click_chatter ("Aroon Indicator - could not get symbol data!");
    return p;
  }
  // 4. update the indicator
  symbol_data_holder->_aroon_indicator.update_indicator (msg->_close);
  // 5. get the values of up and down indicators
  write_indicators (msg->_symbol,
                    msg->_close,
                    symbol_data_holder->_aroon_indicator.get_aroon_up_indicator(),
                    symbol_data_holder->_aroon_indicator.get_aroon_down_indicator(),
                    symbol_data_holder->_file_struct);
  // 6. get the file handle
  // 7. add the timestamp and the value of up/down indicators

  return p;
}

SymbolDataHolder*
AroonIndicatorElement::get_symbol_data (
  const char(& symbol)[Synapse::ORDER_SYMBOL_LEN])
{
  AroonIndicatorMap::Pair* pair = _indicator_map.find_pair (symbol);

  if (!pair)
  {
    click_chatter ("a pair for symbol %s not found, creating one", symbol);
    bool inserted =_indicator_map.insert (symbol, SymbolDataHolder());

    if (!inserted)
    {
      return NULL;
    }
    pair = _indicator_map.find_pair (symbol);
  }

  return &(pair->value);
}

void AroonIndicatorElement::write_indicators(
  const char(& symbol)[Synapse::ORDER_SYMBOL_LEN],
  const double price,
  const double aroon_up_indicator,
  const double aroon_down_indicator,
  FILE*&       file_struct)
{
  // open the file
  if (!file_struct)
  {
    file_struct = fopen(symbol, "w");
  }
  // if file is already open, make sure there is not error
  // associated with it
  if (ferror(file_struct))
  {
    click_chatter ("Aroon Indicator - there is an error with the file %s", symbol);
    return;
  }

  Timestamp time_now = Timestamp::now_steady ();
  fprintf (file_struct, "%d,%f,%f,%f\n", time_now.sec(), price, aroon_up_indicator, aroon_down_indicator);
}

void
AroonIndicatorElement::run_timer (
  Timer*  timer)
{
  // flush files periodically
  for (AroonIndicatorMap::const_iterator i = _indicator_map.begin(); i.live(); ++i)
  {
    FILE* file_struct = i.value()._file_struct;

    if (file_struct)
    {
      fflush (file_struct);
    }
  }

  _timer.reschedule_after_sec (_file_flush_interval_sec);
}

void
AroonIndicatorElement::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS

