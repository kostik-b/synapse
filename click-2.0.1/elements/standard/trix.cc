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

#include "trix.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::SynapseElement;

Trix::Trix()
{
  CacheStruct one, two;

  _local_cache.push_back (one);
  _local_cache.push_back (two);
}

Trix::~Trix()
{
}

int
Trix::configure(Vector<String> &conf, ErrorHandler* errh)
{
  IndicatorBase::configure (conf, errh);

  return 0;
}

FixedPt Trix::initialize_element (Buffers& buffers)
{
  // do nothing. the buffer will contain the value of the last ewma

  return FixedPt::fromInt (0);
}

FixedPt Trix::process_naive (
  Buffers&  buffers)
{
  // copy buffer and calculate the value recursively
  // whereas buffer copy is not necessary we want to
  // stay faithful to the method described in a paper:
  // -> head(S) returns the last element of the stream
  // -> tail(S) returns the remaining elements in the
  //    stream
  RingBuffer copy = buffers[0];

  // TODO: the underlying buffer HAS to contain 2 values
  //       -> need to introduce the MIN_BUFFER_LEN parameter
  //          to the indicator base!!! OR need a way to abort
  //          here - I think that was the initial design idea
  FixedPt last_value        = copy.pop_front ();
  FixedPt penultimate_value = copy.pop_front ();

  return 100*(last_value - penultimate_value)/penultimate_value;
}

VectorCache& Trix::process_ext (Buffers&         buffers)
{
  FixedPt result = process_naive (buffers);

  _local_cache[0]._value = result;

  _local_cache[1]._value = (buffers[0])[0];

  return _local_cache;
}

VectorCache& Trix::process_opt_ext(Vector<FixedPt>& increments,
                                   VectorCache&     cache)
{
  FixedPt penultimate_value =  cache[1]._value;
  FixedPt last_value        =  increments[0];
  
  FixedPt result = 100*((last_value - penultimate_value)/penultimate_value);

  _local_cache[0]._value = result;

  _local_cache[1]._value = last_value;

  return _local_cache;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Trix)
