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

#include "indicator_base.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

#ifndef CLICK_LINUXMODULE
  #include <click/tracer.h>
#endif

#include <click/master.hh>

using Synapse::SynapseElement;
using Synapse::MsgValue;

IndicatorBase::IndicatorBase()
  : _debug        (false)
  , _active       (true)
  , _op_mode      (NAIVE)
  , _buffers      (NULL)
  , _initialized  (false)
{
}

IndicatorBase::~IndicatorBase()
{
}

int
IndicatorBase::configure(Vector<String> &conf, ErrorHandler* errh)
{
  int buffer_size   = 10;
  int in_ports      = ninputs ();
  int op_mode       = 1; // 1 is naive, 2 is startup
  if (Args(conf, errh)
        .read   ("DEBUG",     _debug)
        .read   ("BUF_SIZE",  buffer_size)
        .read_m ("OP_MODE",   op_mode)
        .execute() >= 0)
  {
    if (in_ports < 1)
    {
      click_chatter ("IndicatorBase: wrong number of intake ports");
      return -1;
    }
    _buffers = new Buffers (in_ports, buffer_size, _debug);

    if (op_mode == 1)
    {
      _op_mode = NAIVE;
    }
    else
    {
      _op_mode = STARTUP;
    }

    click_chatter ("IndicatorBase: BUF_SIZE is %d\n", buffer_size);
  }

  return 0;
}

void IndicatorBase::send_msg_value (
  Packet&             packet,
  const FixedPt&      value,
  const uint64_t      timestamp,
  const PacketAppType msg_type)
{   
  WritablePacket* p = packet.put (0);           
    
  MsgValue msg_value;                                                                                    
    
  // make sure there is enough room for stats msg
  const size_t msg_size = sizeof (msg_value);
  const size_t data_len = p->length ();

  if (_debug)
  {
    click_chatter ("IB: msg_size is %d, data_len is %d", msg_size, data_len);
  }
  if (msg_size > data_len)
  {               
    click_chatter ("IndicatorBase - packet too small - cannot send!");
    SynapseElement::discard_packet (*p);                                                                 
    return;                     
  } 
    
  msg_value._value      = value;
  msg_value._timestamp  = timestamp;

  // copy the msg to the packet 
  memcpy (p->data(), reinterpret_cast<char*>(&msg_value), msg_size);
  // set new packet type
  p->set_packet_app_type (msg_type);                                                         
  if (_debug)
  {
#ifndef CLICK_LINUXMODULE
    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value (class_name (), value);
#endif
    click_chatter ("IB-%s: %s sending msg value %s\n---", class_name (),
                                                          Timestamp::now_steady().unparse().c_str (), 
                                                          value.c_str ());
  }

  checked_output_push (0, p);                                                                     
  return;
}           

void
IndicatorBase::push (
  int     port,
  Packet* p)
{
  if (!_active)
  {
      checked_output_push (0, p);
      return;
  }

  if (_debug)
  {
    click_chatter ("IB-%s: packet length is %d", class_name (), p->length ());
  }

  // 1. make sure that the msg is the right one
  if ((p->get_packet_app_type () != Synapse::MSG_ADD) && 
      (p->get_packet_app_type () != Synapse::MSG_UPDATE) &&
      (p->get_packet_app_type () != Synapse::MSG_INIT))
  {
    click_chatter ("IndicatorBase - incorrect msg type: need ADD or UPDATE or INIT");
    // TODO: discard the msg
    checked_output_push (0, p);
    return;
  }

  // 1. switch-case on operation mode
  // 1a. startup - ignore an update, add ADD to ring buffer and pass it up
  // 1b. normal  - if ADD     -> prepend to the cache, pass it up. Use the return value to refresh the cache.
  //               if UPDATE  -> prepend to the cache, pass it up. Do not refresh the cache.
  // 1c. naive   - if ADD     -> prepend to the ring buffer, pass it up
  //               if UPDATE  -> temporarily prepend to the ring buffer, pass it up, remove

  // if we get an init message we do 2 things:
  // i)   we save the value in the buffers
  // ii)  we ask the element to calculate the return value
  //      which we then forward to the next element in the chain

  const MsgValue* msg = reinterpret_cast<const MsgValue*>(p->data());
  // first of all we need to check if this is an "initialize" msg.
  if ((p->get_packet_app_type () == Synapse::MSG_INIT) || (_initialized == false))
  {
    _buffers->add_new_value (msg->_value, msg->_timestamp, port);
    if (_buffers->is_update_complete ())
    {
      FixedPt init_value = initialize_element (*_buffers);
      if (_debug)
      {
        click_chatter ("IB: onInit the result to pass on is %s, valid is %d", 
                          init_value.c_str (), init_value.get_valid ());
      }
      if (init_value.get_valid ())
      {
        send_msg_value (*p, init_value, msg->_timestamp, p->get_packet_app_type ());
      }
      else
      {
        // discard as this will not reach the Discard element
        SynapseElement::discard_packet (*p);
      }
      _initialized = true;
      return;
    }
    else
    {
      // FIXME: do I have to discard here?
      SynapseElement::discard_packet (*p);
      return; // we will come back here later
    }
  }

  if (_op_mode == NAIVE)
  {
    if (p->get_packet_app_type () == Synapse::MSG_ADD)
    {
      _buffers->add_new_value (msg->_value, msg->_timestamp, port);
    }
    else
    {
      _buffers->add_new_value_temp (msg->_value, msg->_timestamp, port);
    }

    if (_buffers->is_update_complete ())
    {
      FixedPt result = process_naive (*_buffers);
      send_msg_value (*p, result, msg->_timestamp, p->get_packet_app_type ());
    }
    else
    {
      // FIXME: do I have to discard here?
      SynapseElement::discard_packet (*p);
      return; // we will come back later :-)
    }

    return; // just to be future-proof
  }

  if (_op_mode == STARTUP)
  {
    if (p->get_packet_app_type () == Synapse::MSG_ADD)
    {
      _buffers->add_new_value (msg->_value, msg->_timestamp, port);
    }
    else
    {
      _buffers->add_new_value_temp (msg->_value, msg->_timestamp, port);
    }

    if (_buffers->is_update_complete ())
    {
      if (p->get_packet_app_type () == Synapse::MSG_ADD)
      {
        _cache = process_ext (*_buffers);
        _op_mode = NORMAL;
        send_msg_value (*p, _cache[0]._value, msg->_timestamp, p->get_packet_app_type ());
      }
      else
      {
        VectorCache&  cache = process_ext (*_buffers);
        send_msg_value (*p, cache[0]._value, msg->_timestamp, p->get_packet_app_type ());
      }
      return; // we are done here
    }
    else
    {
      // FIXME: do I have to discard here?
      SynapseElement::discard_packet (*p);
      return; // we will come back here later
    }
  }

  if (_op_mode == NORMAL)
  {
    // we keep on using the buffers for the case of multiple input ports
    // we are not using add_new_value_temp, because we don't reuse the buffers
    // later, i.e. we don't care
    _buffers->add_new_value (msg->_value, msg->_timestamp, port);
    if (!(_buffers->is_update_complete ()))
    {
      // FIXME: do I have to discard here?
      SynapseElement::discard_packet (*p);
      return; // we will come back here later
    }
    // now assemble the increments into one vector
    Vector<FixedPt> increments;
    for (int i = 0; i < _buffers->get_num_ports (); ++i)
    {
      increments.push_back ((*_buffers)[i].pop_front ());
    }

    if (p->get_packet_app_type () == Synapse::MSG_ADD)
    {
      _cache = process_opt_ext (increments, _cache); // a copy here
      send_msg_value (*p, _cache[0]._value, msg->_timestamp, p->get_packet_app_type ());
      return;
    }
    else
    {
      VectorCache&  cache = process_opt_ext (increments, _cache); // no copy here
      send_msg_value (*p, cache[0]._value, msg->_timestamp, p->get_packet_app_type ());
      return;
    }
  }
}

void
IndicatorBase::add_handlers()
{
  add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(IndicatorBase)
