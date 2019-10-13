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

#include "dx_indicator.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

using Synapse::MsgIndicator;
using Synapse::SynapseElement;

DxIndicator::DxIndicator()
  : _pdi_port        (Synapse::NO_PORT)
  , _mdi_port        (Synapse::NO_PORT)
  , _debug          (false)
  , _active         (true)
{
}

DxIndicator::~DxIndicator()
{
}

int
DxIndicator::configure(Vector<String> &conf, ErrorHandler* errh)
{
  if (Args(conf)
        .read("ACTIVE", _active)
        .read("DEBUG", _debug)
        .read("NAME", _name)
        .read_m("PDI_PORT", _pdi_port)
        .read_m("MDI_PORT", _mdi_port)
        .complete() >= 0)
  {
    click_chatter ("DxIndicator:%s - MDI Port is %d, PDI Port is %d", _name.c_str(), _mdi_port, _pdi_port);
  }

  return 0;
}

Synapse::PacketPort
DxIndicator::detect_packet_port (
  const int packet_port)
{
  if ((_mdi_port < 0) || (_pdi_port < 0))
  {
    return Synapse::NO_PORT; // an error! these should have been configured!
  }

  if (packet_port == _mdi_port)
  {
    return Synapse::MDI_PORT;
  }
  else if (packet_port == _pdi_port)
  {
    return Synapse::PDI_PORT;
  }
  else
  {
    return Synapse::NO_PORT;
  }
}

void
DxIndicator::push (int port, Packet *p)
{
  if (!_active)
  {
    checked_output_push (port, p);
    return;
  }

  if (_debug)
  {
    click_chatter ("DxIndicator:%s received packet at port %d", _name.c_str(), port);
  }

  // 1. get the time period message
  if (p->get_packet_app_type () != Synapse::MSG_INDICATOR)
  {
    click_chatter ("DxIndicator - incorrect msg type");
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
  // 3. if stored packet's timestamp is greater than new one - discard the new one
  //    if new packet's timestamp is greater than stored - discard stored, clone and store the new one
  //    if they are the same proceed to next step

  const MsgIndicator* msg = reinterpret_cast<const MsgIndicator*>(_saved_packet._packet->data());
  uint64_t saved_packet_tstamp  = msg->_timestamp;

  msg = reinterpret_cast<const MsgIndicator*>(p->data());
  uint64_t new_packet_tstamp    = msg->_timestamp;
/*
  if (_debug)
  {
    click_chatter ("DxIndicator:%s ch.1 stored packet tstamp.#%lld, new packet tstamp.#%lld", _name.c_str(),
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
    click_chatter ("DxIndicator:%s ch.2 stored packet tstamp.#%lld, new packet tstamp.#%lld", _name.c_str(),
                    saved_packet_tstamp, new_packet_tstamp);
  }
*/
  // 4. use local vars and a detection function to detect which packet is which
  Packet* pdi_packet = (_saved_packet._port == Synapse::PDI_PORT) ? _saved_packet._packet : p;
  Packet* mdi_packet = (_saved_packet._port == Synapse::MDI_PORT) ? _saved_packet._packet : p;

  const MsgIndicator* pdi_msg = reinterpret_cast<const MsgIndicator*>(pdi_packet->data());
  const MsgIndicator* mdi_msg = reinterpret_cast<const MsgIndicator*>(mdi_packet->data());
  // 5. do the calculation: http://en.wikipedia.org/wiki/Average_directional_movement_index
  fixedpt dx = 0;
  fixedpt part_two  = fixedpt_add (pdi_msg->_indicator, mdi_msg->_indicator);
  if (part_two != 0) // a special case
  {
    fixedpt part_one  = fixedpt_abs (fixedpt_sub (pdi_msg->_indicator, mdi_msg->_indicator));
    fixedpt part_three= fixedpt_div (part_one, part_two);
    dx                = fixedpt_mul(fixedpt_fromint (100), part_three);
  }
  
  if (_debug)
  {
    click_chatter ("DxIndicator:%s mdi msg indicator is %d, pdi indicator is %d, dx is %d", _name.c_str(),
                    mdi_msg->_indicator, pdi_msg->_indicator, dx);
    click_chatter ("------------------------");
  }

  // 6. store the result in new packet and send it onwards
  char symbol_copy [Synapse::ORDER_SYMBOL_LEN];
  memcpy (symbol_copy, mdi_msg->_symbol, Synapse::ORDER_SYMBOL_LEN);
  if (!SynapseElement::populate_msg_indicator(*p, symbol_copy, dx, mdi_msg->_timestamp))
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
DxIndicator::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DxIndicator)
