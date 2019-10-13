// Copyright QUB 2017

#ifndef msg_trade_hh
#define msg_trade_hh

#include "fixedpt_cpp.h"
#include <cstring>

const size_t ORDER_SYMBOL_LEN = 10;

struct TimeStats
{
  TimeStats ()
  {
    _first_time_updated = false;
    _size               = 0;
    _high = _low = _open = _close = 0;
  }

  fixedpt  _high;
  fixedpt  _low;
  fixedpt  _open;
  fixedpt  _close;
  int64_t _size;

  bool    _first_time_updated;

  void rollover ()
  {
    _open               = _close;
    _size               = 0;
    // high and low remain the same

    _first_time_updated = false;
  }
}; // struct TimeStats

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


#endif
