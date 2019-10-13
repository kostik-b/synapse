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

#include "ewma_calculator.hh"
#include <click/appmsgs.hh>

#include <click/confparse.hh>
#include "synapseelement.hh"
#include <click/global_sizes.hh>
#include <click/master.hh>

using Synapse::MsgTrade;
using Synapse::MsgIndicator;
using Synapse::SynapseElement;
using Synapse::ORDER_SYMBOL_LEN;

EwmaElement::EwmaElement()
  : _ewma_alpha_value   (0)
  , _interval_len       (0)
  , _debug              (false)
  , _active             (true)
{
}

EwmaElement::~EwmaElement()
{
}

int
EwmaElement::configure(Vector<String> &conf, ErrorHandler* errh)
{
  int alpha = 0;
 
  if (Args(conf)
        .read("ACTIVE", _active)
        .read("DEBUG", _debug)
        .read_m("EWMA_ALPHA", alpha)
        .read_m("INTERVAL_LEN", _interval_len)
        .complete() >= 0)
  {
    _ewma_alpha_value = fixedpt_div (fixedpt_fromint (1), fixedpt_fromint(alpha));
    
    click_chatter ("EwmaElement: interval len is %lld,"
                  "EWMA alpha value is %s",  _interval_len, fixedpt_cstr(_ewma_alpha_value, 4));
  }

  return 0;
}

void
EwmaElement::push (int port, Packet *p)
{
  if (!_active)
  {
      checked_output_push (port, p);
      return;
  }

  // 1. make sure that this is the msg indicator
  if (p->get_packet_app_type () != Synapse::MSG_INDICATOR)
  {
    click_chatter ("EwmaElement - incorrect msg type");
    checked_output_push (0, p);
    return;
  }

  const MsgIndicator* msg = reinterpret_cast<const MsgIndicator*>(p->data());

  assert (sizeof(*msg) <= p->length ());

  // 2. fetch the circular buffer for symbol (or create one)
  EwmaCalculator* ewma_calculator = 
      SynapseElement::get_or_create<EwmaMap> (_ewma_map,
                                              msg->_symbol);

  // 3. calculate the interval i and compare to the saved current interval num
  //    if they are the same recalculate the current EMWA - note if the
  //    prev value is absent, the current value of EWMA is the value of msg_indicator.
  //    if they are not the same and the current value is not zero (in which case the
  //    ewma is the value of indicator), we need to rollover the current into previous
  //    and to calculate the ewma
  const int64_t interval_num = int64_t(msg->_timestamp - master()->get_start_time_as_cycles ()) /
                                       int64_t(_interval_len);
  if (_debug)
  {
    click_chatter ("Ewma: in %d", msg->_indicator);
  }
  if (interval_num == ewma_calculator->_current_interval_num)
  {
    if (ewma_calculator->_prev_ewma_value == 0)
    {
      // ewma_0 is x_0
      ewma_calculator->_current_ewma_value = msg->_indicator;
    }
    else
    {
      // we actually need to calculate ewma
      fixedpt part_one = fixedpt_mul(fixedpt_sub(fixedpt_fromint(1), _ewma_alpha_value), 
                                     msg->_indicator); 
      fixedpt part_two = fixedpt_mul (_ewma_alpha_value, ewma_calculator->_prev_ewma_value);
      ewma_calculator->_current_ewma_value = fixedpt_add (part_one, part_two);
    }
  }
  else
  {
    if (_debug )click_chatter ("Ewma: Rollover");
    // need to roll over first, then calculate the current ewma
    ewma_calculator->_prev_interval_num = ewma_calculator->_current_interval_num;
    ewma_calculator->_prev_ewma_value   = ewma_calculator->_current_ewma_value;

    if (ewma_calculator->_prev_ewma_value == 0)
    {
      // ewma_0 is x_0
      ewma_calculator->_current_ewma_value  = msg->_indicator;
      ewma_calculator->_current_interval_num= interval_num;
    }
    else
    {
      // if the prev interval is preceding, just calculatte according
      // to the formula
      // we actually need to calculate ewma
      fixedpt part_one = fixedpt_mul(fixedpt_sub(fixedpt_fromint(1), _ewma_alpha_value), 
                                     msg->_indicator); 
      fixedpt part_two = fixedpt_mul (_ewma_alpha_value, ewma_calculator->_prev_ewma_value);

      ewma_calculator->_current_ewma_value  = fixedpt_add (part_one, part_two);
      ewma_calculator->_current_interval_num= interval_num;
    }
  }
  if (_debug)
  {
    click_chatter ("Ewma: prev value is %d, current value is %d", ewma_calculator->_prev_ewma_value, ewma_calculator->_current_ewma_value);
  }
  // 4. if we have a value, simply publish it onwards
  char symbol_copy [Synapse::ORDER_SYMBOL_LEN];
  memcpy (symbol_copy, msg->_symbol, Synapse::ORDER_SYMBOL_LEN);

  if (!SynapseElement::populate_msg_indicator(*p,
                                              symbol_copy,
                                              ewma_calculator->_current_ewma_value,
                                              msg->_timestamp))
  {
    SynapseElement::discard_packet (*p);
  }
  else
  {
    checked_output_push (0, p);
  }
}

void
EwmaElement::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(EwmaElement)
