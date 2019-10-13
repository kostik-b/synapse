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

#include "msg_filter_symbol.hh"
#include <click/appmsgs.hh>

#include <click/confparse.hh>

using Synapse::MsgTrade;

MsgFilterSymbol::MsgFilterSymbol()
  : _filter_mode    (RESTRICTIVE)
  , _active         (true)
{
}

MsgFilterSymbol::~MsgFilterSymbol()
{
}

int
MsgFilterSymbol::configure(Vector<String> &conf, ErrorHandler* errh)
{
  String  allow_symbols;
  String  restrict_symbols;

  if (Args(conf)
        .read("ACTIVE", _active)
        .read("ALLOW_SYMBOLS",  allow_symbols)
        .read("RESTRICT_SYMBOLS",  restrict_symbols)
        .complete() >= 0)
  {
    click_chatter ("MsgFilterSymbol: allow_symbols are %s, restrict symbols are %s", allow_symbols.c_str(), restrict_symbols.c_str());
  }

  for (int i = 0; i < conf.size(); ++i)
  {
    click_chatter ("conf line %d is %s", i, conf[i].c_str());
  }

  // we now need to populate the map with symbols and set the mode to allow or restrict

  click_chatter ("msg filter symbol : allow symbols are: %s", allow_symbols.c_str());

  // the vector to contain symbols
  Vector<String>  symbols_vector;
  cp_spacevec (allow_symbols, symbols_vector);

  if (!symbols_vector.empty ())
  {
    _filter_mode = ALLOWING;
  }
  else
  {
    cp_spacevec (restrict_symbols, symbols_vector);
    if (!symbols_vector.empty())
    {
      _filter_mode= RESTRICTIVE;
    }
  }

  for (int i = 0; i < symbols_vector.size (); ++i)
  {
    String& symbol = symbols_vector[i];
    
    SymbolArrayWrapper array_wrapper;
    array_wrapper = symbol.c_str();
    bool inserted = _symbol_map.insert (array_wrapper, true);

    if (!inserted)
    {
      click_chatter ("MsgFilterSymbol - could not insert symbol %s into the map", symbol.c_str());

      return -1;
    }
  }

  return 0;
}

void
MsgFilterSymbol::push (int port, Packet *p)
{
  if (!_active)
  {
      checked_output_push (port, p);
      return;
  }

  // check msg type
  if (!(p->get_packet_app_type () == Synapse::MSG_TRADE))
  {
    click_chatter ("MsgFilterSymbol - received an incorrect msg, passing on without modification");
    checked_output_push (port, p);
    return;
  }

  const MsgTrade*   msg_trade = reinterpret_cast<const MsgTrade*>(p->data());

  SymbolMap::Pair*  pair      = _symbol_map.find_pair (msg_trade->_symbol);

  if (_filter_mode == RESTRICTIVE)
  {
    // if we didn't find that symbol in restricting mode
    // this means this symbol is allowed
    if (!pair)
    {
      checked_output_push (port, p);
      return;
    }
    else
    {
      discard_packet(*p);
      return;
    }
  }
  else if (_filter_mode == ALLOWING)
  {
    // if the symbol was not found in allowed mode
    // this means that the symbol is actually restricted
    if (!pair)
    {
      discard_packet (*p);
      return;
    }
    else
    {
      checked_output_push (port, p);
      return;
    }
  }

  checked_output_push (port, p);
}

void MsgFilterSymbol::discard_packet (
  Packet& packet)
{
  packet.kill ();
}

void
MsgFilterSymbol::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MsgFilterSymbol)
