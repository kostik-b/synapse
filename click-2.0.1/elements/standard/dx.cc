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

#include "dx.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::SynapseElement;

Dx::Dx()
{
  CacheStruct one;

  _local_cache.push_back (one);
}

Dx::~Dx()
{
}

int
Dx::configure(Vector<String> &conf, ErrorHandler* errh)
{
  IndicatorBase::configure (conf, errh);

  return 0;
}

FixedPt Dx::initialize_element (Buffers& buffers)
{
  return process_naive (buffers);
}

FixedPt Dx::process_naive (
  Buffers&  buffers)
{
  // copy buffer and calculate the value recursively
  // whereas buffer copy is not necessary we want to
  // stay faithful to the method described in a paper:
  // -> head(S) returns the last element of the stream
  // -> tail(S) returns the remaining elements in the
  //    stream
  RingBuffer& pdi_buf  = buffers[0];
  RingBuffer& ndi_buf  = buffers[1];

  FixedPt pdi = pdi_buf[0];
  FixedPt ndi = ndi_buf[0];

  if ((pdi + ndi).getC () == 0)
  {
    return FixedPt::fromInt (0);
  }

  return 100*fpt_abs(pdi - ndi)/ (pdi + ndi);
}

VectorCache& Dx::process_ext (Buffers&         buffers)
{
  FixedPt result = process_naive (buffers);

  _local_cache[0]._value = result;

  return _local_cache;
}

VectorCache& Dx::process_opt_ext (Vector<FixedPt>& increments,
                                  VectorCache&     cache)
{
  FixedPt pdi = increments[0];
  FixedPt ndi = increments[1];

  FixedPt result;

  if ((pdi + ndi).getC () == 0)
  {
    result = FixedPt::fromInt (0);
  }
  else
  {
    result = 100*fpt_abs(pdi - ndi) / (pdi + ndi);
  }

  _local_cache[0]._value = result;

  return _local_cache;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Dx)
