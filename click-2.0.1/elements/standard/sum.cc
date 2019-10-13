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

#include "sum.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::SynapseElement;

static bool s_debug = false;

Sum::Sum()
{
  CacheStruct one, two, three;

  _local_cache.push_back (one);
  _local_cache.push_back (two);
  _local_cache.push_back (three);
}

Sum::~Sum()
{
}

int
Sum::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf, errh)
        .read_m("SUM_PERIODS", _sum_periods)
        .execute() >= 0)
  {
    click_chatter ("SUM periods value is %d", _sum_periods);
  }

  IndicatorBase::configure (conf, errh);

  s_debug = _debug;

  return 0;
}

static FixedPt calculate_sum_recursive (
  RingBuffer&  buffer)
{
  if (s_debug)
  {
//    click_chatter ("EWMA: array value is %s", buffer[0].c_str ());
  }
  if (buffer.occupancy () == 1)
  {
    return buffer.pop_front ();
  }
  else
  {
    FixedPt front = buffer.pop_front ();
    return front + calculate_sum_recursive (buffer);
  }
}

FixedPt Sum::initialize_element (
  Buffers&         buffers)
{
  return (buffers[0])[0];
}

static void print_buffer (RingBuffer& ring_buffer)
{
  for (int i = 0; i < ring_buffer.occupancy (); ++i)
  {
    click_chatter (" | %d -> %s ", i, ring_buffer[i].c_str ());
  }
}


FixedPt Sum::process_naive (
  Buffers&  buffers)
{
  // copy buffer and calculate the value recursively
  RingBuffer copy = buffers[0];

  // print_buffer (copy);

  return calculate_sum_recursive (copy);
}

VectorCache& Sum::process_ext (Buffers& buffers)
{
  FixedPt result = process_naive (buffers);

  _local_cache[0]._value = result;

  _local_cache[1]._value = result;

  _local_cache[2]._ring_buffer = buffers[0];

  return _local_cache;
}

VectorCache& Sum::process_opt_ext(Vector<FixedPt>& increments,
                                  VectorCache&     cache)
{
  // we reuse the local cache's ring buffer
  // to avoid creating a new one, which would cause a
  // malloc/kalloc
  RingBuffer& sum_buf   = _local_cache[2]._ring_buffer;

  // now we copy the contents of the cached buffer into
  // our local buffer. If their size is the same, then
  // no memory allocation will be done
  sum_buf               = cache[2]._ring_buffer;

  FixedPt     sum       = cache[1]._value;

  FixedPt     new_value = increments[0];

  FixedPt result;

  if (sum_buf.is_full ())
  {
    // "shift" window
    FixedPt last = sum_buf.pop_back ();
    result = sum - last + new_value;
    sum_buf.push_front (new_value);
  }
  else
  {
    result = sum + new_value;
    sum_buf.push_front (new_value);
  }

  _local_cache[0]._value = result;

  _local_cache[1]._value = result;

  // this has already been done -- see the start of the function
  //_local_cache[2]._ring_buffer = sum_buf;

  return _local_cache;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Sum)
