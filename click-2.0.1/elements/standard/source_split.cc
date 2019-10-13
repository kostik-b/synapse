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

#include "source_split.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

#ifndef CLICK_LINUXMODULE
  #include <click/tracer.h>
#endif

#include <click/master.hh>

using Synapse::SynapseElement;
using Synapse::MsgValue;
using Synapse::MsgSource;

SourceSplit::SourceSplit()
  : _debug        (false)
  , _close_port   (-1)
  , _high_port    (-1)
  , _low_port     (-1)
  , _open_port    (-1)
  , _volume_port  (-1)
{
  _active = true;
}

SourceSplit::~SourceSplit()
{
}

int
SourceSplit::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("DEBUG",  _debug)
        .read("CLOSE",  _close_port)
        .read("HIGH",   _high_port)
        .read("LOW",    _low_port)
        .read("OPEN",   _open_port)
        .read("VOLUME", _volume_port)
        .complete() >= 0)
  {
    click_chatter ("SourceSplit: HLOC V: %d%d%d%d %d",
                    _high_port, _low_port, _open_port, _close_port, _volume_port);
  }

  return 0;
}

void SourceSplit::sendMsg (
  Packet&         packet,
  const int       out_port,
  const fixedpt&  value,
  const uint64_t  timestamp,
  Synapse::PacketAppType msg_type)
{
  WritablePacket* p = packet.put (0);

  MsgValue msg_value;

  // make sure there is enough room for stats msg
  const size_t msg_size = sizeof (msg_value);
  const size_t data_len = p->length ();

  if (msg_size > data_len)
  {
    click_chatter ("Source Split - packet too small - cannot send!");
    SynapseElement::discard_packet (*p);
    return;
  }

  msg_value._value      = FixedPt::fromC (value);
  msg_value._timestamp  = timestamp;

  // copy the msg to the packet
  memcpy (p->data(), reinterpret_cast<char*>(&msg_value), msg_size);
  // set new packet type
  Synapse::PacketAppType new_type = Synapse::MSG_UPDATE;
  switch (msg_type)
  {
    case Synapse::MSG_ADD_SOURCE:
      new_type = Synapse::MSG_ADD;
      break;
    case Synapse::MSG_UPDATE_SOURCE:
      new_type = Synapse::MSG_UPDATE;
      break;
    case Synapse::MSG_INIT_SOURCE:
      new_type = Synapse::MSG_INIT;
      break;
    default:
      break;
  }
  p->set_packet_app_type (new_type);

  if (_debug)
  {
    click_chatter ("SourceSplit: sending value %s on port %d", msg_value._value.c_str (),
                                                               out_port);
  }

  checked_output_push (out_port, p);
  return;
}

void
SourceSplit::push (
  int     port,
  Packet* p)
{
  // get msg type
  // if not msg add or msg update, destroy
  if ((p->get_packet_app_type () != Synapse::MSG_ADD_SOURCE) &&
        (p->get_packet_app_type () != Synapse::MSG_UPDATE_SOURCE) &&
          (p->get_packet_app_type () != Synapse::MSG_INIT_SOURCE))  
  {
    click_chatter ("SourceSplit: wrong msg type, should have been"
                    "MSG_ADD_SOURCE or MSG_UPDATE_SOURCE or MSG_INIT_SOURCE");

    SynapseElement::discard_packet (*p);
    return;
  }

  const MsgSource* msg = reinterpret_cast<const MsgSource*>(p->data());

  MsgSource     copy      = *msg; // need to copy since the packet will be reused
  PacketAppType type_copy = p->get_packet_app_type ();
#ifndef CLICK_LINUXMODULE
  if (_debug)
  {
    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("High", FixedPt::fromC(copy._high));
    tracer.add_value ("Low",  FixedPt::fromC(copy._low));
    tracer.add_value ("Close",FixedPt::fromC(copy._close));

    if ((p->get_packet_app_type () == Synapse::MSG_ADD_SOURCE))
    {
      tracer.add_type ("ADD");
    }
    else if (p->get_packet_app_type () == Synapse::MSG_UPDATE_SOURCE)
    {
      tracer.add_type ("UPDATE");
    }
  }
#endif

  // if msg update or add, extract the values and
  // reuse the msg as container for MsgValue, send
  // the extracted values according to subscription
  if (_open_port > -1)
  {
    sendMsg (*(p->clone ()), _open_port, copy._open, copy._timestamp, type_copy);
  }
  if (_high_port > -1)
  {
    sendMsg (*(p->clone()), _high_port, copy._high, copy._timestamp, type_copy);
  }
  if (_close_port > -1)
  {
    sendMsg (*(p->clone()), _close_port, copy._close, copy._timestamp, type_copy);
  }
  if (_low_port > -1)
  {
    sendMsg (*(p->clone()), _low_port, copy._low, copy._timestamp, type_copy);
  }
  if (_volume_port > -1)
  {
    sendMsg (*(p->clone()), _volume_port, fixedpt_fromint(copy._volume), copy._timestamp, type_copy);
  }

  SynapseElement::discard_packet (*p);
}

void
SourceSplit::add_handlers()
{
  add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SourceSplit)
