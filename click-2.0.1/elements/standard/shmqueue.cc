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

#include <click/confparse.hh>

#include "shmqueue.hh"
#include "synapseelement.hh"
#include "chix_glue.h"

#include <click/appmsgs.hh>

#include <click/shmq_msg.h>

using Synapse::SynapseElement;
using Synapse::MsgTrade;

ShmQueue::ShmQueue()
  : _active         (true)
  , _shmq_key       (-1)
  , _shmq_id        (-1)
{
  // FIXME: need to act on failure
  if (RESOLVE_SYSCALLS () < 0)
  {
    click_chatter ("ShmQueue - couldn't resolve syscalls");
  }
}

ShmQueue::~ShmQueue()
{
  click_chatter ("ShmQueue destructor is called");
  
  if (int result = msgctl (_shmq_id, IPC_RMID, NULL) < 0)
  {
    click_chatter(
        "ShmQueue : Unable to remove message queue from system, return code is %d\n", result );
  }
}

int
ShmQueue::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("ACTIVE", _active)
        .read_m("SHMQ_KEY", _shmq_key)
        .complete() >= 0)
  {
  }

  _shmq_id = msgget( _shmq_key, 0666 | IPC_CREAT );

  if( _shmq_id < 0 )
  {
    click_chatter ("ShmQueue : Unable to obtain shared memory queue id, it's %d\n", _shmq_id);
    return -1;
  }

  return 0;
}

void
ShmQueue::push(int port, Packet *p)
{
  if (!_active)
  {
      return;
  }

  const MsgTrade* msg_trade = reinterpret_cast<const MsgTrade*>(p->data());

  click_chatter ("KB: ShmQueue - received a packet");
  click_chatter ("KB: msg trade, symbol is %s, size is %d",
                  msg_trade->_symbol, msg_trade->_size);

  struct shmq_msg buf;
  const size_t    msg_size = sizeof (MsgTrade);

  memset (&buf, '\0', sizeof(buf));
  buf._mtype = SHMQ_MSG_KERNEL_TYPE;

  const size_t    copy_size = (msg_size >= SHMQ_MSG_MAX_DATA_LEN ? SHMQ_MSG_MAX_DATA_LEN - 1 : msg_size);

  // first save the size
  buf._msg_data_struct._size = copy_size;
  // then write the actual data
  memcpy (buf._msg_data_struct._data, msg_trade, copy_size);

  // send the msg trade to a shared memory queue
  int result = msgsnd (_shmq_id, (struct shmq_msg*)&buf, sizeof(buf._msg_data_struct), 0);

  if (result < 0)
  {
    click_chatter ("ShmQueue - was not able to send a msg, err code is %d", result);
  }

  SynapseElement::discard_packet (*p);
}

void
ShmQueue::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ShmQueue)

