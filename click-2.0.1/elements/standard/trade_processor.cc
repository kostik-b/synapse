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

#include "trade_processor.hh"
#include <click/appmsgs.hh>
#include "synapseelement.hh"

static const size_t s_whole_price_len       = 10;
static const size_t s_fractional_price_len  = 10;

static const size_t s_mac_ip_udp_len        = 42;

static bool         s_debug                 = false;

using Synapse::MsgTrade;
using Synapse::SynapseElement;
using Synapse::MsgSource;

static bool update_stats (TimeStats&      time_stats,
                          const MsgTrade& msg_trade);

static int denominator_for_len (size_t len)
{
  switch (len)
  {
    case 1:
      return 10;
      break;
    case 2:
      return 100;
      break;
    case 3:
      return 1000;
      break;
    case 4:
      return 10000;
      break;
    case 5:
      return 100000;
      break;
    case 6:
      return 1000000;
      break;
  }
}

// the function to parse the price
static fixedpt parse_price_as_fixedpt (
  const char*   str,
  const size_t  strlen)
{
  const char* dot_ptr = NULL;
  for (int i = 0; i < strlen; ++i)
  {
    if (str[i] == '.')
    {
      dot_ptr = str + i;
      break;
    }
  }

  // convert the whole and decimal parts now
  if (dot_ptr == NULL) // only whole part
  {
    char buffer [strlen + 1];
    memcpy (buffer, str, strlen);
    buffer[strlen] = '\0';
    return fixedpt_fromint (strtol (buffer, NULL, 10));
  }
  else if (dot_ptr == str + 1) // only fractional part
  {
    char buffer [strlen];
    memcpy (buffer, str + 1, strlen - 1);
    buffer[strlen - 1] = '\0';

    fixedpt price = 0;
    
    long    fr    = strtol (buffer, NULL, 10);
    // divide the fractional part by 10^(fr part len), e.g. 23/100 or 435/1000. Then add to the price
    fixedpt_add (price, fixedpt_div (fixedpt_fromint (fr), 
                                     fixedpt_pow(FIXEDPT_ONE, strlen - 1)));
    return price;
  }
  else
  {
    char whole_part       [s_whole_price_len];
    char fractional_part  [s_fractional_price_len];

    memset (whole_part,     '\0', s_whole_price_len);
    memset (fractional_part,'\0', s_fractional_price_len);

    size_t whole_part_len       = dot_ptr - str;
    size_t fractional_part_len  = strlen - whole_part_len - 1;

    strncpy (whole_part,      str,          whole_part_len);
    strncpy (fractional_part, dot_ptr + 1,  fractional_part_len);

    // divide the fractional part by 10^(fr part len), e.g. 23/100 or 435/1000. Then add to the price
    // whole part is easy
    fixedpt price = fixedpt_fromint (strtol (whole_part, NULL, 10));
    // convert to int
    long    fr                          = strtol (fractional_part, NULL, 10);
    fixedpt frac_fixedpt = fixedpt_div (fixedpt_fromint (fr), 
                                     fixedpt_fromint (denominator_for_len (fractional_part_len)));

    
    price = fixedpt_add (price, frac_fixedpt);

    return price;
  }
}

TradeProcessor::TradeProcessor()
  : _timer                    (this)
  , _aggregation_interval_sec (10)
  , _debug                    (false)
  , _active                   (true)
  , _total_msgs               (0)
{

  _w_packet = NULL;
}

TradeProcessor::~TradeProcessor()
{
  click_chatter ("TP: total msgs count is %llu", _total_msgs);
}

int
TradeProcessor::configure(Vector<String> &conf, ErrorHandler* errh)
{
  // TODO: read in the array of subscription symbols
  //       the array of intervals (not implemented yet)
  //       ... 
  String symbols_ports;
  if (Args(conf)
        .read("AGGREGATION_INTERVAL_SEC", _aggregation_interval_sec)
        .read("SYMBOLS_ROUTING",          symbols_ports)
        .read("DEBUG",                    _debug)
        .complete() >= 0)
  {
    click_chatter ("Aggregation interval is %d", _aggregation_interval_sec);
  }

  s_debug = _debug;

    // the vector to contain symbols
  Vector<String>  symbols_ports_vector;
  cp_spacevec (symbols_ports, symbols_ports_vector);

  for (int i = 0; i < symbols_ports_vector.size (); i += 2)
  {
    String& symbol = symbols_ports_vector[i];
    String& port   = symbols_ports_vector[i + 1];

    SymbolArrayWrapper array_wrapper;
    array_wrapper = symbol.c_str();

    int port_int = strtol (port.c_str(), NULL, 10);

    bool inserted = _subscriptions_map.insert (array_wrapper, port_int);

    if (!inserted)
    {
      click_chatter ("MsgFilterSymbol - could not insert symbol %s into the map",
                        symbol.c_str());

      return -1;
    }
    else
    {
      click_chatter ("TradeProcessor - added %s-%d mapping", symbol.c_str(), port_int);
    }
  }

  return 0;
}

int
TradeProcessor::initialize (
  ErrorHandler*)
{
  _timer.initialize(this);
  // we scheduler timer after we receive the first update, in this way
  // we can compare the results
  /* 
  if (_active)
  {
    _timer.schedule_after_sec(_aggregation_interval_sec);
  }
  */
  return 0;

}

static void print_as_hex_inline (const char* str, const size_t str_len)
{
  const int buf_len = 1024;
  char  buffer [buf_len];
  memset (buffer, '\0', buf_len);
  char* buf_ptr = buffer;
  size_t i = 0;
  for (; (i < str_len) && ((buf_ptr - buffer) < (buf_len - 10)); ++i)
  {
    int written = sprintf (buf_ptr, "%2X ", (uint8_t)(str[i]));
    buf_ptr += written;
  }
  click_chatter ( "1:%s", buffer);

  memset (buffer, '\0', buf_len);
  buf_ptr = buffer;

  i = 0;
  for (; (i < str_len) && ((buf_ptr - buffer) < (buf_len - 10)); ++i)
  {
    int written = 0;
    if ((str[i] < '0') || (str[i] > 'z'))
    {
      written = sprintf (buf_ptr, "   ");
    }
    else
    {
      written = sprintf (buf_ptr, " %c ", str[i]);
    }
    buf_ptr += written;
  }
  click_chatter ("2:%s", buffer);
}

static bool populate_msg_trade (MsgTrade& msg, Packet& p)
{
  // how do we filter out the wrong messages?
  // -> probably by simply parsing the msg bit by by bit

  const uint32_t  packet_len          = p.length();

  if (s_debug)
  {
    click_chatter ("------------\nTP: packet len is %d", packet_len);
  }

  // needs to make sure that we can at least reach
  // the market data msg type field
  if (packet_len < s_mac_ip_udp_len + 1)
  {
    if (s_debug)
    {
      click_chatter ("TP-populate_msg_trade - packet too short to parse");
    }
    return false;
  }


  const unsigned  tstamp_len      = 8;
  const char*     md_msg          = reinterpret_cast<const char*>(p.data() + 
                                                                  s_mac_ip_udp_len + tstamp_len);
  const uint32_t  md_len          = packet_len - s_mac_ip_udp_len - tstamp_len;

  int                   component_start = 0;
  int                   counter         = 0;

  for (int i = 0; i < md_len; ++i)
  {
    if (md_msg[i] == '|')
    {
      ++counter;

      switch (counter)
      {
        case 0: // should never happen
        case 1: // do nothing 
        case 2: // it's the timestamp delta from previous message - ignore
          break;
        case 3: // it's a symbol
          {
            size_t symbol_len = i - component_start;

            if (symbol_len > Synapse::ORDER_SYMBOL_LEN - 1)
            {
              click_chatter ("ERROR: Symbol is too long!");
              return false;
            }
            strncpy (msg._symbol, md_msg + component_start, symbol_len);
          }
          break;
        case 4: // it's a price, the size is immediately after
          {
            size_t price_len = i - component_start;
            msg._price = parse_price_as_fixedpt (md_msg + component_start, price_len);
            if ((i + 1) >= md_len)
            {
              click_chatter ("No size detected in the trade msg!!!");
              return false;
            }
            // now parse the size
            const char*   size_ptr = md_msg + i + 1;
            size_t        size_len = md_len - i - 1;

            char buffer [size_len + 1];
            memcpy (buffer, size_ptr, size_len);
            buffer [size_len] = '\0';

            msg._size = strtol (buffer, NULL, 10);
          }
          break;
        default:
          // too many vertical bars - return false
          click_chatter ("TP: Too many vertical bars, could not parse a message!!!");
          return false;
          break;
      }

      component_start = i + 1; // the next char after the vertical bar
    }
  }

  if (counter < 1)
  {
    return false; // incorrect message basically - no delimiters
  }

  // this timestamp is produced by timestamper - if we get here, the msg is the correct one
  msg._src_timestamp              = *(reinterpret_cast<const uint64_t*>(p.data () + s_mac_ip_udp_len));
  //click_chatter ("KB: timestamp is %lld", msg._src_timestamp);
  if (s_debug)
  {
    click_chatter ("TP: populated MsgTrade");
    click_chatter ("TP: the packet was:");
    print_as_hex_inline (md_msg, md_len);
    click_chatter ("TP: the trade msg is: symbol - %s, price - %s, size - %d",
                    msg._symbol, fixedpt_cstr (msg._price, 4), msg._size);
  }

  return true;
}

void
TradeProcessor::push(
  int     port,
  Packet* p)
{
  if (!_active)
  {
    checked_output_push (port, p);
    return;
  }

  ++_total_msgs;

  // 1. parse the trade message into the MsgTrade structure
  //    and look up the symbol in the hash table
  // 2. then look at the subscriptions for this particular symbol
  //    and send out the updates for the subscribed components
  //    no updates - no msg
  // 3. don't forget to update the hash table in all of this!
  MsgTrade msg_trade;
  if (!populate_msg_trade (msg_trade, *p))
  {
    // destroy the msg and return. Do not pass it forward.
    SynapseElement::discard_packet  (*p);
    return;
  }

  if (_debug)
  {
    click_chatter ("TP: checking msg trade symbol %s", msg_trade._symbol);
  }

  const int* out_port = _subscriptions_map.findp (msg_trade._symbol);

  if (out_port == NULL)
  {
    SynapseElement::discard_packet  (*p);
    return;
  }

  static bool first_symbol = true;
  // start the timestamp now (do it only once)
  if (first_symbol)
  {
    _timer.schedule_after_sec(_aggregation_interval_sec);
    first_symbol = false;
  }

  bool time_stats_added = false;
  TimeStats* time_stats = SynapseElement::get_or_create<TimeStatsMap> (_time_stats_map,
                                                                       time_stats_added,
                                                                       msg_trade._symbol);

  if (!time_stats)
  {
    SynapseElement::discard_packet  (*p);
    return;
  }
  
  // no update - no msg
  if (!update_stats (*time_stats, msg_trade))
  {
    SynapseElement::discard_packet  (*p);
    return;
  }

  if (_debug)
  {
    click_chatter ("TP: about to send symbol %s on port %d", msg_trade._symbol, *out_port);
  }
  send_update_msg (*time_stats, msg_trade._src_timestamp, *out_port, *p, time_stats_added);
}

static bool update_stats (
  TimeStats&      time_stats,
  const MsgTrade& msg_trade)
{
  // 1. if first update in an interval
  //    set the close, high, low to new trade's price
  // 2. if not first update, update as usual

  bool rc = false; 

  if (!(time_stats._first_time_updated))
  {
    time_stats._high  = msg_trade._price;
    time_stats._low   = msg_trade._price;
    time_stats._close = msg_trade._price;
    time_stats._size += msg_trade._size;

    time_stats._first_time_updated  = true;
    rc                              = true;

    return true;
  }

  if (msg_trade._price > time_stats._high)
  {
    time_stats._high = msg_trade._price;
    rc               = true;
  }

  if (msg_trade._price < time_stats._low)
  {
    time_stats._low  = msg_trade._price;
    rc               = true;
  }

  time_stats._size += msg_trade._size;

  if (msg_trade._price != time_stats._close)
  {
    time_stats._close = msg_trade._price;
    rc                = true;
  }

  if (s_debug)
  {
    char high_str [25];
    char low_str  [25];
    char close_str[25];
    char open_str [25];

    fixedpt_str (time_stats._high, high_str, 4);
    fixedpt_str (time_stats._low, low_str, 4);
    fixedpt_str (time_stats._close, close_str, 4);
    fixedpt_str (time_stats._open, open_str, 4);

    click_chatter ("TP: attempted to update time stats:");
    click_chatter ("\thigh - %s, low - %s, close - %s, open - %s, size - %d",
                    high_str, low_str, close_str, open_str, time_stats._size);

  }
  return rc;
}

void TradeProcessor::send_update_msg (
  const TimeStats&  stats,
  const uint64_t    timestamp,
  int               out_port,
  Packet&           packet,
  const bool        is_init)
{
  WritablePacket* p = packet.put (0);

  MsgSource msg_source;

  // make sure there is enough room for stats msg
  const size_t msg_size = sizeof (msg_source);
  const size_t data_len = p->length ();

  if (msg_size > data_len)
  {
    click_chatter ("Trade processor - packet too small - cannot send!");
    SynapseElement::discard_packet (*p);
    return;
  }

  msg_source._high      = stats._high;
  msg_source._low       = stats._low;
  msg_source._open      = stats._open;
  msg_source._close     = stats._close;

  msg_source._timestamp = timestamp;

  // copy the msg to the packet
  memcpy (p->data(), reinterpret_cast<char*>(&msg_source), msg_size);
  // set new packet type
  p->set_packet_app_type (is_init ? Synapse::MSG_INIT_SOURCE : Synapse::MSG_UPDATE_SOURCE);

  checked_output_push (out_port, p);
  return;
}

void TradeProcessor::send_add_msg (
  const TimeStats&          stats,
  const int                 out_port)
{
  // The resident packet
  MsgSource msg_source;

  msg_source._high   = stats._high;
  msg_source._low    = stats._low;
  msg_source._open   = stats._open;
  msg_source._close  = stats._close;

  msg_source._volume = stats._size;

  msg_source._timestamp = Synapse::gcc_rdtsc ();

  if (_debug)
  {
    char high_str [25];
    char low_str  [25];
    char close_str[25];
    char open_str [25];

    fixedpt_str (msg_source._high, high_str, 4);
    fixedpt_str (msg_source._low, low_str, 4);
    fixedpt_str (msg_source._close, close_str, 4);
    fixedpt_str (msg_source._open, open_str, 4);

    click_chatter ("\tTP send add msg: high - %s, low - %s, close - %s, open - %s, size - %d",
                    high_str, low_str, close_str, open_str, msg_source._volume);
  }

  _w_packet = Packet::make(Packet::default_headroom, 0, sizeof (MsgSource) + 1, 0); // each packet is for 1 time use only, i.e. it is discarded in the Discard element
  if (_w_packet->length () < sizeof (msg_source) + 1)
  {
    click_chatter ("TP: terrible error in send msg_source!!!!!!!");
    return;
  }
  // copy the msg to the packet
  memcpy (_w_packet->data(), reinterpret_cast<char*>(&msg_source), sizeof(msg_source));
  // set new packet type
  _w_packet->set_packet_app_type (Synapse::MSG_ADD_SOURCE);

  checked_output_push (out_port, _w_packet);
  return;
}

void
TradeProcessor::run_timer (
  Timer*  timer)
{
  // flush files periodically
  for (TimeStatsMap::iterator i = _time_stats_map.begin(); i.live(); ++i)
  {
    // get the port
    int* out_port = _subscriptions_map.findp (i.key ());
    if (!out_port)
    {
      click_chatter ("TP: could not find subscription for symbol %s",
                      i.key ().array_ptr ());
      continue;
    }
    // send msg
    if (_debug)
    {
      click_chatter ("TP: sending add message for symbol %s on port %d",
                        i.key ().array_ptr (), *out_port);
    }
    send_add_msg(i.value(), *out_port);

    i.value().rollover ();
  }

  _timer.reschedule_after_sec (_aggregation_interval_sec);
}

void
TradeProcessor::add_handlers()
{
    add_data_handlers("active", Handler::OP_READ | Handler::OP_WRITE | Handler::CHECKBOX | Handler::CALM, &_active);
}

CLICK_ENDDECLS
ELEMENT_MT_SAFE(TradeProcessor)
EXPORT_ELEMENT(TradeProcessor)
