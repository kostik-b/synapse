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

#include "stat_printer.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"
#include "chix_trade_handler_utils.hh"

#include <click/confparse.hh>

static const int MAX_ALL_VALUES = 5000;

using Synapse::MsgIndicator;
using Synapse::MsgValue;
using Synapse::SynapseElement;
using Synapse::CyclesCounter;
using Synapse::RunningStat;
using Synapse::Stats;


static Vector<StatPair*>* g_svec_kb_synapse = NULL;

StatPrinter::StatPrinter()
  : _reporting_interval (100)
  , _active             (true)
  , _combined_mode      (false)
  , _longest_indic      (false) // default is average
  , _mt_arr_idx         (0)
{
  // hopefully the objects are created sequentially
  // so there is no race condition in the below instantiation
  if (g_svec_kb_synapse == NULL)
  {
    g_svec_kb_synapse = new Vector<StatPair*>;
  }

  StatPair* all_values  = new StatPair [MAX_ALL_VALUES];
  _all_values_counter   = 0;
  // we want every single value to be printed for now
  _cycles_counter.set_reporting_interval (1);

  g_svec_kb_synapse->push_back (all_values);

  _mt_arr_idx = g_svec_kb_synapse->size () - 1;
}

StatPrinter::~StatPrinter()
{
  // 1. if we only have 1 item in global array, just print the values.
  // 2. if there is more than 1 item, then do the following for every element in the arrays:
  //  - for every element in the array compare it to all other elements in
  //    all other arrays - use tstamp to see if we compare the correct msgs
  //  - choose the max value across all arrays.
  //  - calculate the latency
  //  - feed latency into  the _running.stat
  // 3. print the running stat
  click_chatter ("StatPrinter destructor is called, vector size is %d, "
                 "combined mode is %d, longest ind is %d", g_svec_kb_synapse->size (), _combined_mode, _longest_indic);
  if (!_combined_mode)
  {
    report_running_stat ();
  }
  else if ((_combined_mode) && (_mt_arr_idx == 0))
  {
    for (unsigned i = 0; i < _all_values_counter; ++i)
    {
      if (_longest_indic)
      {
        uint64_t max_cycles  = 0;
        uint64_t start_cycle = (*g_svec_kb_synapse)[0][i]._start_tstamp;

        for (unsigned j = 0; j < (*g_svec_kb_synapse).size (); ++j)
        {
          if (max_cycles < (*g_svec_kb_synapse)[j][i]._end_tstamp)
          {
            max_cycles = (*g_svec_kb_synapse)[j][i]._end_tstamp;
          }
          // big trouble!
          if (start_cycle != (*g_svec_kb_synapse)[j][i]._start_tstamp)
          {
            click_chatter ("Trouble, trouble, the msgs are not aligned!!!");
          }
        }
        _running_stat.update_counter (max_cycles - start_cycle);
      }
      else
      {
        uint64_t total_cycles = 0;
        uint64_t start_cycle  = (*g_svec_kb_synapse)[0][i]._start_tstamp;

        for (unsigned j = 0; j < (*g_svec_kb_synapse).size (); ++j)
        {
          // just a sanity check whether the starting tstamps are aligned
          // i.e. is it the same msg?
          if (start_cycle != (*g_svec_kb_synapse)[j][i]._start_tstamp)
          {
            click_chatter ("Trouble, trouble, the msgs are not aligned!!!");
          }

          total_cycles += ((*g_svec_kb_synapse)[j][i]._end_tstamp - start_cycle);

        }
        _running_stat.update_counter (total_cycles / (*g_svec_kb_synapse).size ());
      }
    }

    report_running_stat ();
  }

  click_chatter ("StatPrinter %d - printing the values", _mt_arr_idx);
  for (int i = 0; i < _all_values_counter; ++i)
  {
    click_chatter ("FixedPt value is %s", (*g_svec_kb_synapse)[_mt_arr_idx][i]._value.c_str ());
  }
}

int
StatPrinter::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("REPORT_INTERVAL",_reporting_interval)
        .read("COMBINED_MODE",  _combined_mode)
        .read("LD",             _longest_indic)
        .complete() >= 0)
  {
    click_chatter ("StatPrinter: reporting interval is %d", _reporting_interval);
  }

  return 0;
}

Packet *
StatPrinter::simple_action (Packet *p)
{
  if (!_active)
  {
    return p;
  }

  switch (p->get_packet_app_type ())
  {
    case Synapse::MSG_INDICATOR:
      //update_running_stat (*p);
      click_chatter ("Not supporting msg indicator for now");
      break;
    case Synapse::MSG_ADD:
      //click_chatter ("Not supporting msg add for now");
      break; // not yet
    case Synapse::MSG_UPDATE:
      update_running_stat_new (*p);
      break;
    default:
      click_chatter ("StatPrinter - received an incorrect msg, passing on without modification");
      break;
  }

  return p;
}

void
StatPrinter::report_running_stat ()
{
  const Stats& stats = _running_stat.get_running_stats ();
  click_chatter ("RunningStat:N is %d,\tAggr. cycles is %lld,\tHigh is %lld",
                  stats._N, stats._aggregate_cycles, stats._highest);
  click_chatter ("RunningStat:Low is %lld,\ts1 is %lld,\ts2 is %lld",
                  stats._lowest, stats._s1, stats._s2);
}

#if 0
void
StatPrinter::update_running_stat (Packet& p)
{
  const MsgIndicator*   msg           = reinterpret_cast<const MsgIndicator*>(p.data ());
  const uint64_t        cycles_start  = msg->_timestamp;
  // get the timestamp
#ifdef __X86_64__
  const uint64_t        cycles_now    = ChixTradeHandlerUtils::gcc_rdtsc ();
#else
  const uint64_t        cycles_now    = ChixTradeHandlerUtils::bmk_rdtsc ();
#endif

  _running_stat.update_counter (cycles_now - cycles_start);

  static int report_counter = 0;
  ++report_counter;

  if (report_counter % _reporting_interval == 0)
  {
    report_running_stat ();
  }
}
#endif

void
StatPrinter::update_running_stat_new (Packet& p)
{
  const MsgValue*       msg           = reinterpret_cast<const MsgValue*>(p.data ());
  const uint64_t        cycles_start  = msg->_timestamp;
  // get the timestamp
#ifdef __x86_64__
  const uint64_t        cycles_now    = Synapse::gcc_rdtsc ();
#else
  const uint64_t        cycles_now    = Synapse::bmk_rdtsc ();
#endif

  StatPair* the_arr = (*g_svec_kb_synapse)[_mt_arr_idx];

  if ((_combined_mode) && (g_svec_kb_synapse->size () > 1))
  {
    if (_all_values_counter < (MAX_ALL_VALUES - 1))
    {
      the_arr [_all_values_counter]._value        = msg->_value;
      the_arr [_all_values_counter]._start_tstamp = cycles_start;
      the_arr [_all_values_counter]._end_tstamp   = cycles_now;
      ++_all_values_counter;
    }
  }
  else
  {
    _running_stat.update_counter (cycles_now - cycles_start);

    if (_all_values_counter < (MAX_ALL_VALUES - 1))
    {
      the_arr [_all_values_counter]._value        = msg->_value;
      ++_all_values_counter;
    }
  }

/*
  static int report_counter = 0;
  ++report_counter;

  if (report_counter % _reporting_interval == 0)
  {
    report_running_stat ();
  }
*/
}


#if 0
void
StatPrinter::update_stats (Packet& p)
{
  const MsgIndicator*   msg           = reinterpret_cast<const MsgIndicator*>(p.data ());
  const uint64_t        cycles_start  = msg->_timestamp;
  // get the timestamp
  const uint64_t        cycles_now    = ChixTradeHandlerUtils::bmk_rdtsc ();

  const bool            should_report =
    _cycles_counter.update_counter (cycles_now - cycles_start);

  static int            str_report_counter = 0;

  if (should_report)
  {
    click_chatter ("KB: Cycles counter is %lld", _cycles_counter.get_counter ());

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
#endif

void
StatPrinter::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(StatPrinter)
