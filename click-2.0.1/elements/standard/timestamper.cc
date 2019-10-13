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

#include "timestamper.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

#include <click/confparse.hh>

using Synapse::MsgValue;
using Synapse::SynapseElement;


Timestamper::Timestamper()
  : _active             (true)
  , _print_avg          (false)
  , _replace_tstamp     (true)
  , _total_cycles       (0)
  , _running_count      (0)
{
}

Timestamper::~Timestamper()
{
}

int
Timestamper::configure(Vector<String> &conf, ErrorHandler* errh)
{

  if (Args(conf)
        .read("PRINTAVG",      _print_avg)
        .read("REPLACETSTAMP", _replace_tstamp)
        .complete() >= 0)
  {
  }

  return 0;
}

void Timestamper::print_avg_latency (
  const uint64_t tstamp)
{
  const  uint64_t tstamp_2 = Synapse::gcc_rdtsc ();
  _total_cycles += (tstamp_2 - tstamp);
  ++_running_count;
  click_chatter ("TSTAMP avg latency %llu",
                  uint64_t(_total_cycles/_running_count));
}

Packet *
Timestamper::simple_action (Packet *p)
{
  if (!_active)
  {
    return p;
  }

  switch (p->get_packet_app_type ())
  {
    case Synapse::MSG_UPDATE_SOURCE:
      {
        WritablePacket* wp = p->put (0);
        Synapse::MsgSource* msg = reinterpret_cast<Synapse::MsgSource*>(wp->data());

        if (_print_avg)
        {
          print_avg_latency (msg->_timestamp);
        }
        if (_replace_tstamp)
        {
          msg->_timestamp = Synapse::gcc_rdtsc ();
        }
      }
      break;
    case Synapse::MSG_UPDATE:
      {
        WritablePacket* wp = p->put (0);
        Synapse::MsgValue* msg = reinterpret_cast<Synapse::MsgValue*>(wp->data ());

        if (_print_avg)
        {
          print_avg_latency (msg->_timestamp);
        }

        if (_replace_tstamp)
        {
          msg->_timestamp = Synapse::gcc_rdtsc ();
        }
      }
      break;
    default:
      // click_chatter ("Timestamper - received an incorrect msg, passing on without modification");
      break;
  }

  return p;
}

void
Timestamper::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Timestamper)
