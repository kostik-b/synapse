// Copyright Queen's Univeristy Belfast 2012

#ifndef SYNAPSE_APP_MSGS_HH
#define SYNAPSE_APP_MSGS_HH

#include <click/global_sizes.hh>
#include <click/fixedptc.h>
#include <click/fixedpt_cpp.h>

#pragma pack(1)

namespace Synapse
{

enum PacketAppType
{
  NO_APP_MSG = 0,
  MSG_TRADE,
  MSG_TIME_PERIOD_STATS,
  MSG_INDICATOR,
  MSG_UPDATE_SOURCE,
  MSG_ADD_SOURCE,
  MSG_UPDATE,
  MSG_ADD,
  MSG_INIT,
  MSG_INIT_SOURCE
};

struct MsgTrade
{
  MsgTrade ()
    : _price          (0)
    , _size           (0)
    , _timestamp      (0)
    , _src_timestamp  (0)
  {
    memset (_symbol, '\0', sizeof(_symbol));
  }

  fixedpt   _price;
  int64_t   _size;
  char      _symbol[ORDER_SYMBOL_LEN];
  uint64_t  _timestamp;
  uint64_t  _src_timestamp;

}; // struct MsgTrade

struct MsgTimePeriodStats
{
  MsgTimePeriodStats ()
    : _high (0.0f)
    , _low  (0.0f)
    , _open (0.0f)
    , _close(0.0f)
    , _size (0)
  {
    memset (_symbol, '\0', sizeof(_symbol));
  }

  double  _high;
  double  _low;
  double  _open;
  double  _close;

  int64_t _size; 

  char    _symbol[ORDER_SYMBOL_LEN];  
}; // struct MsgTimePeriodStats

struct MsgIndicator
{
  MsgIndicator ()
  {
    memset (_symbol, '\0', sizeof(_symbol));
    _indicator = 0;
    _timestamp = 0;
  }

  char      _symbol[ORDER_SYMBOL_LEN];
  fixedpt   _indicator;
  uint64_t  _timestamp;
}; // struct MsgIndicator

// types: MSG_ADD, MSG_UPDATE
struct MsgValue
{
  MsgValue ()
  {
    _timestamp = 0;
//    _volume    = 0;
  }

  FixedPt   _value;
//  int64_t   _volume;
  uint64_t _timestamp;
}; // struct MsgValue

// types: MSG_ADD_SOURCE, MSG_UPDATE_SOURCE
struct MsgSource
{
  MsgSource ()
    : _timestamp  (0)
    , _volume     (0)
  {
    _close  = fixedpt_fromint (0);
    _high   = fixedpt_fromint (0);
    _low    = fixedpt_fromint (0);
    _open   = fixedpt_fromint (0);
  }

  fixedpt   _close;
  fixedpt   _high;
  fixedpt   _low;
  fixedpt   _open;
  uint64_t  _timestamp;

  int64_t   _volume; 
}; // struct MsgSource


} // namespace Synapse

#pragma pack()

#endif

