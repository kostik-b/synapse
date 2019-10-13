// Copyright QUB 2018

#ifndef synapse__ad_line_h
#define synapse__ad_line_h

#include "array_wrapper.hh"
#include "fixedpt_cpp.h"
#include <utility>
#include <ext/hash_map>
#include "hc_types.h"

enum Tick
{
  UP,
  DOWN,
  FLAT
};

typedef ArrayWrapper<char, ORDER_SYMBOL_LEN> CharArrayWrapper;

/*
  The below class exists per symbol.
*/
class ADLineTick
{
public:
  ADLineTick (const bool    debug,
              const char (& symbol)[ORDER_SYMBOL_LEN]);

  void initialize       (const FixedPt&   close);

  /* this value is returned for the ad-line indicator,
     i.e. it is NOT the value of adlinetick
  */
  // std::pair<unsigned, unsigned>
  FixedPt
       calculate_value  (const FixedPt&   close,
                         const UpdateType update_type);

private:
  FixedPt           _prev_close;

  const bool        _debug;
  char              _symbol[ORDER_SYMBOL_LEN];
}; // class ADLinetick

/*
  The below class exists for all symbols (i.e. it is a singleton)
  and it aggregates the outputs of all the ADLineTicks.
*/
class ADLine
{
public:
  static ADLine& get_instance ();

  //std::pair<unsigned, unsigned>
  int calculate_value (const Tick   tick,
                       const char(& symbol)[ORDER_SYMBOL_LEN]);

private: 
  ADLine (); // no default constructor
  ADLine (const ADLine& copy); // no copy constructor

  typedef __gnu_cxx::hash_map<CharArrayWrapper, Tick> TickMap;

  TickMap   _tick_map;

  unsigned  _total_up;
  unsigned  _total_down;
}; // class ADLine

#endif
