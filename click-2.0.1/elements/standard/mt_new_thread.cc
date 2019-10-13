// -*- c-basic-offset: 4 -*-
/*
 * unqueue.{cc,hh} -- element pulls as many packets as possible from
 * its input, pushes them out its output
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2002 International Computer Science Institute
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
#include "mt_new_thread.hh"
#include <click/args.hh>
#include <click/error.hh>
#include <click/standard/scheduleinfo.hh>
CLICK_DECLS

MtNewThread::MtNewThread()
    : _active (true)
    , _debug (false)
    , _task(this)
    , _buffer (100)
{
}

MtNewThread::~MtNewThread()
{
}

int
MtNewThread::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _active = true;
    _debug = false;
    return Args(conf, this, errh)
        .read("DEBUG", _debug)
	.read("ACTIVE", _active).complete();
}

int
MtNewThread::initialize(ErrorHandler *errh)
{
    ScheduleInfo::initialize_task(this, &_task, _active, errh);
}

void MtNewThread::push (int, Packet* p)
{
  if (Packet *q = p->clone())
  {
    WritablePacket* write_packet = q->uniqueify ();
    // --> put the packet into the circ buffer
    _spinlock.acquire ();
      if (_buffer.is_full ())
      {
        click_chatter ("ERROR: MT NEW THREAD buffer full - we are dropping a packet");
      }
      _buffer.push_back (write_packet);
    _spinlock.release ();
  }
}

// the assumption here that it is only even entered by one thread
// at a time, i.e. we have 1 producer = 1 consumer scenario
bool
MtNewThread::run_task(Task *)
{
  if (!_active)
    return false;

  if (_buffer.empty ())
  {
    _task.fast_reschedule ();
    return true;
  }
  
  _spinlock.acquire ();
    WritablePacket* write_packet = _buffer.pop_front ();
  _spinlock.release ();

  output(0).push (write_packet);

  _task.fast_reschedule();

  return true;
}

#if 0 && defined(CLICK_LINUXMODULE)
#if __i386__ && HAVE_INTEL_CPU
/* Old prefetching code from run_task(). */
  if (p_next) {
    struct sk_buff *skb = p_next->skb();
    asm volatile("prefetcht0 %0" : : "m" (skb->len));
    asm volatile("prefetcht0 %0" : : "m" (skb->cb[0]));
  }
#endif
#endif

#if 0
int
MtNewThread::write_param(const String &conf, Element *e, void *user_data,
		     ErrorHandler *errh)
{
    MtNewThread *u = static_cast<MtNewThread *>(e);
    switch (reinterpret_cast<intptr_t>(user_data)) {
    case h_active:
	if (!BoolArg().parse(conf, u->_active))
	    return errh->error("syntax error");
	break;
    case h_reset:
	u->_count = 0;
	break;
    case h_limit:
	if (!IntArg().parse(conf, u->_limit))
	    return errh->error("syntax error");
	break;
    case h_burst:
	if (!IntArg().parse(conf, u->_burst))
	    return errh->error("syntax error");
	if (u->_burst < 0)
	    u->_burst = 0x7FFFFFFF;
	break;
    }
    if (u->_active && !u->_task.scheduled()
	&& (u->_limit < 0 || u->_count < (uint32_t) u->_limit))
	u->_task.reschedule();
    return 0;
}

void
MtNewThread::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::CHECKBOX, &_active);
    add_data_handlers("count", Handler::OP_READ, &_count);
    add_data_handlers("burst", Handler::OP_READ, &_burst);
    add_data_handlers("limit", Handler::OP_READ, &_limit);
    add_write_handler("active", write_param, h_active);
    add_write_handler("reset", write_param, h_reset, Handler::BUTTON);
    add_write_handler("reset_counts", write_param, h_reset, Handler::BUTTON | Handler::UNCOMMON);
    add_write_handler("burst", write_param, h_burst);
    add_write_handler("limit", write_param, h_limit);
    add_task_handlers(&_task, &_signal);
}

#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(MtNewThread)
ELEMENT_MT_SAFE(MtNewThread)
