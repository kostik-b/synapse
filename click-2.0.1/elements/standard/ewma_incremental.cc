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

#include "ewma_incremental.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::SynapseElement;

static bool s_debug = false;

EwmaIncremental::EwmaIncremental()
{
  CacheStruct one, two;

  _local_cache.push_back (one);
  _local_cache.push_back (two);
}

EwmaIncremental::~EwmaIncremental()
{
}

int
EwmaIncremental::configure(Vector<String> &conf, ErrorHandler* errh)
{
  int alpha = 0;

  if (Args(conf, errh)
        .read_m("ALPHA_PERIODS", alpha)
        .execute() >= 0)
  {
    //_alpha = fixedpt_div (fixedpt_fromint (1), fixedpt_fromint(alpha));
    _alpha = FixedPt::fromInt (2) / 
              (FixedPt::fromInt (alpha) + FixedPt::fromInt (1));
    
    click_chatter ("EWMA alpha value is %s", _alpha.c_str ());
  }

  IndicatorBase::configure (conf, errh);

  s_debug = _debug;

  return 0;
}

static FixedPt calculate_ewma_recursive (
  RingBuffer&  buffer,
  FixedPt&        alpha)
{
  if (s_debug)
  {
    click_chatter ("EWMA: array value is %s", buffer[0].c_str ());
  }
  if (buffer.occupancy () == 1)
  {
    return buffer.pop_front ();
  }
  else
  {
    FixedPt part_one = (1 - alpha) * buffer.pop_front ();
    return  part_one + 
      alpha * calculate_ewma_recursive (buffer, alpha);
  }
}

FixedPt EwmaIncremental::initialize_element (
  Buffers&         buffers)
{
  return (buffers[0])[0];
}

FixedPt EwmaIncremental::process_naive (
  Buffers&  buffers)
{
  // copy buffer and calculate the value recursively
  RingBuffer copy = buffers[0];

  return calculate_ewma_recursive (copy, _alpha);
}

VectorCache& EwmaIncremental::process_ext(Buffers&         buffers)
{
  FixedPt result = process_naive (buffers);

  _local_cache[0]._value = result;

  _local_cache[1]._value = result;

  return _local_cache;
}
VectorCache& EwmaIncremental::process_opt_ext(Vector<FixedPt>& increments,
                                              VectorCache&     cache)
{
  FixedPt result = (1 - _alpha) * increments[0] + 
                      _alpha * cache[1]._value;

  _local_cache[0]._value = result;

  _local_cache[1]._value = result;

  return _local_cache;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(EwmaIncremental)
