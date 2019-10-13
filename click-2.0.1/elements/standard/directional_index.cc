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

#include "directional_index.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"


using Synapse::MsgIndicator;
using Synapse::SynapseElement;

DirectionalIndex::DirectionalIndex()
  : _dm_port        (-1)
  , _tr_port        (-1)
  , _debug          (false)
  , _active         (true)
{
}

DirectionalIndex::~DirectionalIndex()
{
}

int
DirectionalIndex::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("ACTIVE", _active)
        .read("DEBUG", _debug)
        .read("NAME", _name)
        .read_m("TR_PORT", _tr_port)
        .read_m("DM_PORT", _dm_port)
        .complete() >= 0)
  {
    click_chatter ("DirectionalIndex:%s - TR Port is %d, DM Port is %d", _name.c_str(), _tr_port, _dm_port);
  }

  return 0;
}

Synapse::PacketPort
DirectionalIndex::detect_packet_port (
  const int packet_port)
{
  if ((_tr_port < 0) || (_dm_port < 0))
  {
    return Synapse::NO_PORT; // an error! these should have been configured!
  }

  if (packet_port == _tr_port)
  {
    return Synapse::TR_PORT;
  }
  else if (packet_port == _dm_port)
  {
    return Synapse::DM_PORT;
  }
  else
  {
    return Synapse::NO_PORT;
  }
}

void
DirectionalIndex::push (int port, Packet *p)
{
  if (!_active)
  {
    checked_output_push (port, p);
    return;
  }

  if (_debug)
  {
    click_chatter ("DirectionalIndex:%s received packet at port %d", _name.c_str(), port);
  }

  // 1. get the time period message
  if (p->get_packet_app_type () != Synapse::MSG_INDICATOR)
  {
    click_chatter ("DirectionalIndex - incorrect msg type");
    checked_output_push (0, p);
    return;
  }

  // 1. detect packet's port
  Synapse::PacketPort packet_port = detect_packet_port (port);
  if (packet_port == Synapse::NO_PORT)
  {
    checked_output_push (port, p);
    return;
  }
  // 2. compare packet's port with the one of saved packet
  //    the same: kill stored packet (it has lower impulse num), clone the new one and return
  //    different: proceed to next step
  if (_saved_packet._port == packet_port)
  {
    if (_saved_packet._packet)
    {
      SynapseElement::discard_packet (*(_saved_packet._packet));
      _saved_packet._packet = p->clone ();
      return;
    }
  }
  else if (_saved_packet._port == Synapse::NO_PORT)
  {
    _saved_packet._port     = packet_port;
    _saved_packet._packet   = p->clone ();
    return;
  }
  // 3. if stored packet's impulse num is greater than new one - discard the new one
  //    if new packet's impulse num is greater than stored - discard stored, clone and store the new one
  //    if they are the same proceed to next step

  const MsgIndicator* msg = reinterpret_cast<const MsgIndicator*>(_saved_packet._packet->data());
  uint64_t saved_packet_tstamp  = msg->_timestamp;

  msg = reinterpret_cast<const MsgIndicator*>(p->data());
  uint64_t new_packet_tstamp    = msg->_timestamp;

/*
  if (_debug)
  {
    click_chatter ("DirectionalIndex:%s chp.1 stored packet tstamp.#%lld, new packet tstamp.#%lld", _name.c_str(),
                    saved_packet_tstamp, new_packet_tstamp);
  }
*/
  if (saved_packet_tstamp > new_packet_tstamp)
  {
    // an old msg has arrived
    SynapseElement::discard_packet (*p);
    return;
  }
  else if (new_packet_tstamp > saved_packet_tstamp)
  {
    // we've got an old message stored
    SynapseElement::discard_packet (*(_saved_packet._packet));
    _saved_packet._port   = packet_port;
    _saved_packet._packet = p->clone ();
    return;
  }
/*
  if (_debug)
  {
    click_chatter ("DirectionalIndex:%s chp.2 stored packet tstamp.#%lld, new packet tstamp.#%lld", _name.c_str(),
                    saved_packet_tstamp, new_packet_tstamp);
  }
*/
  // 4. use local vars and a detection function to detect which packet is which
  Packet* tr_packet = (_saved_packet._port == Synapse::TR_PORT) ? _saved_packet._packet : p;
  Packet* dm_packet = (_saved_packet._port == Synapse::DM_PORT) ? _saved_packet._packet : p;

  const MsgIndicator* tr_msg = reinterpret_cast<const MsgIndicator*>(tr_packet->data());
  const MsgIndicator* dm_msg = reinterpret_cast<const MsgIndicator*>(dm_packet->data());
  // 5. do the calculation: http://en.wikipedia.org/wiki/Average_directional_movement_index
  fixedpt di  = 0;
  if (tr_msg->_indicator != 0)// a special case
  {
    di = fixedpt_div(dm_msg->_indicator, tr_msg->_indicator);
  }
  
  if (_debug)
  {
    click_chatter ("DirectionalIndex:%s dm msg indicator is %d, tr indicator is %d, di is %d", _name.c_str(),
                    dm_msg->_indicator, tr_msg->_indicator, di);
  }

  // 6. store the result in new packet and send it onwards
  char symbol_copy [Synapse::ORDER_SYMBOL_LEN];
  memcpy (symbol_copy, tr_msg->_symbol, Synapse::ORDER_SYMBOL_LEN);
  if (!SynapseElement::populate_msg_indicator(*p, symbol_copy, di, tr_msg->_timestamp))
  {
    SynapseElement::discard_packet (*p);
  }
  else
  {
    checked_output_push (0, p);
  }

  // 7. discard the saved packet and set the saved port to no port
  SynapseElement::discard_packet (*(_saved_packet._packet));
  _saved_packet._packet = NULL;
  _saved_packet._port   = Synapse::NO_PORT;
}

void
DirectionalIndex::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DirectionalIndex)
