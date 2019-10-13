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

#include "msg_printer.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

#include <click/confparse.hh>

using Synapse::MsgIndicator;
using Synapse::MsgTimePeriodStats;
using Synapse::MsgTrade;
using Synapse::SynapseElement;

MsgPrinter::MsgPrinter()
  : _active         (true)
{
  click_chatter ("MsgPrinter constructor is called");
}

MsgPrinter::~MsgPrinter()
{
  click_chatter ("MsgPrinter destructor is called");
}

int
MsgPrinter::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("ACTIVE", _active)
        .read("FILE_EXTENSION", _file_extension)
        .complete() >= 0)
  {
    click_chatter ("MsgPrinter: file extension is %s", _file_extension.c_str());
  }

  return 0;
}

FILE*
MsgPrinter::get_file_ptr (
  bool&         first_open,
  const char(&  symbol)[Synapse::ORDER_SYMBOL_LEN])
{
  // 1. lookup the symbol in the map, create it if it doesn't exist
  FILE* null_value = NULL;
  FILE** file_ptr = SynapseElement::get_or_create<FileStructMap> (_file_struct_map, symbol, null_value);
  // 2. if file not open, then open it - use symbol name and file
  //    extension
  if (!(*file_ptr))
  {
    // TODO: concatenate a symbol here
    String file_name = symbol;
    file_name += ".";
    file_name += _file_extension;
    *file_ptr = fopen (file_name.c_str(), "w");

    first_open = true;
  }

  // 3. write a line to the file and flush it
  if (ferror(*file_ptr))
  {
    click_chatter ("MsgPrinter: extension %s - there is an error with the file %s", _file_extension.c_str(), symbol);
    return NULL;
  }

  return *file_ptr;
}

void
MsgPrinter::write_msg_indicator (
  Packet& p)
{
  const MsgIndicator*   msg_indicator = reinterpret_cast<const MsgIndicator*>(p.data());

  bool  first_open  = false;
  FILE* file_ptr    = get_file_ptr (first_open, msg_indicator->_symbol);

  if (!file_ptr)
  {
    return; // oh well ...
  }

  if (first_open)
  {
    fprintf       (file_ptr, "Timestamp,IndicatorValue\n");
  }

  Timestamp time_now = Timestamp::now_steady ();
  fprintf       (file_ptr, "%d,%f\n", time_now.sec(), msg_indicator->_indicator);
  fflush        (file_ptr);
  click_chatter ("Symbol: %s,%d,%f\n", msg_indicator->_symbol, time_now.sec(), msg_indicator->_indicator);
}

void
MsgPrinter::write_msg_stats (
  Packet& p)
{
  const MsgTimePeriodStats*   msg_stats = reinterpret_cast<const MsgTimePeriodStats*>(p.data());

  bool  first_open  = false;
  FILE* file_ptr    = get_file_ptr (first_open, msg_stats->_symbol);

  if (!file_ptr)
  {
    return;
  }

  if (first_open)
  {
    fprintf (file_ptr, "Timestamp,High,Low,Open,Close\n");
  }

  Timestamp time_now = Timestamp::now_steady ();
  fprintf       (file_ptr, "%d,%4.2f,%4.2f,%4.2f,%4.2f\n", time_now.sec(), msg_stats->_high, msg_stats->_low, msg_stats->_open, msg_stats->_close);
  fflush        (file_ptr);
  click_chatter ("%d,%4.2f,%4.2f,%4.2f,%4.2f\n", time_now.sec(), msg_stats->_high, msg_stats->_low, msg_stats->_open, msg_stats->_close);
}

void MsgPrinter::write_msg_trade (Packet& p)
{
  // open the file
  const MsgTrade*   msg_trade = reinterpret_cast<const MsgTrade*>(p.data());
/*
  bool  first_open  = false;
  FILE* file_ptr    = get_file_ptr (first_open, "chix_trade_msgs_formatted");

  if (!file_ptr)
  {
    return;
  }
*/
  static uint64_t previousTimestamp = msg_trade->_timestamp;

  const uint64_t timestampDiffMcsecs = (msg_trade->_timestamp - previousTimestamp)*uint64_t(1000);

  char priceStr [25];
  memset (priceStr, '\0', 25);
  fixedpt_str (msg_trade->_price, priceStr, 4);
  // write the formatted msg
  click_chatter ("Received the trade msg, symbol is %s, price is %s, timestamp is %lld",
                  msg_trade->_symbol, priceStr, msg_trade->_timestamp); 
  click_chatter ("R|%lld|%s|%s|%d", timestampDiffMcsecs, msg_trade->_symbol, priceStr, msg_trade->_size);

  previousTimestamp = msg_trade->_timestamp;
}

Packet *
MsgPrinter::simple_action (Packet *p)
{
  if (!_active)
  {
      return p;
  }

  switch (p->get_packet_app_type ())
  {
    case Synapse::MSG_INDICATOR:
      write_msg_indicator (*p);
      break;
    case Synapse::MSG_TIME_PERIOD_STATS:
      write_msg_stats (*p);
      break;
    case Synapse::MSG_TRADE:
      write_msg_trade (*p);
      break;
    default:
      click_chatter ("MsgPrinter - received an incorrect msg, passing on without modification");
      return p;
      break;
  }

  return p;
}

void
MsgPrinter::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
ELEMENT_MT_SAFE(MsgPrinter)
