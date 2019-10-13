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

#include "pdm.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::SynapseElement;

Pdm::Pdm()
{
  // initialize the local cache
  CacheStruct one, two, three;

  _local_cache.push_back (one);
  _local_cache.push_back (two);
  _local_cache.push_back (three);
}

Pdm::~Pdm()
{
}

int
Pdm::configure(Vector<String> &conf, ErrorHandler* errh)
{
  IndicatorBase::configure (conf, errh);

  return 0;
}

FixedPt Pdm::initialize_element (Buffers& buffers)
{
  // do nothing. the buffer will contain the necessary value
  // we can't really calculate even the initial value because
  // we don't have yet the previous value

  FixedPt result;
  result.set_valid (false);

  return result;
}

static FixedPt calculate_pdm (
  const FixedPt& new_low,
  const FixedPt& prev_low,
  const FixedPt& new_high,
  const FixedPt& prev_high)
{
  if ((new_high - prev_high) < (prev_low - new_low))
  {
    return FixedPt::fromInt (0);
  }
  else if ((new_high - prev_high) < FixedPt::fromInt (0))
  {
    return FixedPt::fromInt (0);
  }
  else
  {
    return (new_high - prev_high);
  }
}

FixedPt Pdm::process_naive (
  Buffers&  buffers)
{
  // copy buffer and calculate the value recursively
  // whereas buffer copy is not necessary we want to
  // stay faithful to the method described in a paper:
  // -> head(S) returns the last element of the stream
  // -> tail(S) returns the remaining elements in the
  //    stream
  RingBuffer& high_buf  = buffers[0];
  RingBuffer& low_buf   = buffers[1];

  FixedPt new_low   = low_buf[0];
  FixedPt prev_low  = low_buf[1];

  FixedPt new_high  = high_buf[0];
  FixedPt prev_high = high_buf[1];

  return calculate_pdm (new_low, prev_low, new_high, prev_high);
}

VectorCache& Pdm::process_ext (Buffers&         buffers)
{
  FixedPt result = process_naive (buffers);

  _local_cache[0]._value = result;

  _local_cache[1]._value = (buffers[0])[0];

  _local_cache[2]._value = (buffers[1])[0];

  return _local_cache;
}

VectorCache& Pdm::process_opt_ext (Vector<FixedPt>& increments,
                                   VectorCache&     cache)
{
  FixedPt new_high  = increments[0];
  FixedPt new_low   = increments[1];

  FixedPt prev_high = cache[1]._value;
  FixedPt prev_low  = cache[2]._value;
/*
  click_chatter ("KB-897: new_high is %s", new_high.c_str ());

  click_chatter ("KB-897: new_low is %s", new_low.c_str ());
  click_chatter ("KB-897: prev_high is %s", prev_high.c_str ());
  click_chatter ("KB-897: prev_low is %s", prev_low.c_str ());
*/
  FixedPt result = calculate_pdm (new_low, prev_low, new_high, prev_high);

  _local_cache[0]._value = result;

  _local_cache[1]._value = new_high;

  _local_cache[2]._value = new_low;

  return _local_cache;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Pdm)
