// Copyright QUB 2018

#include "ad_line.h"
#include <stdio.h>
#include <cstring>

ADLineTick::ADLineTick (
  const bool    debug,
  const char(&  symbol)[ORDER_SYMBOL_LEN])
  : _debug (debug)
{
  memcpy (_symbol, symbol, ORDER_SYMBOL_LEN);
}

void ADLineTick::initialize (const FixedPt&   close)
{
  _prev_close = close;
}

//std::pair<unsigned, unsigned>
FixedPt ADLineTick::calculate_value (
  const FixedPt&   close,
  const UpdateType update_type)
{
  // calculate the difference and
  // update the prev value if ADD
  FixedPt diff = close - _prev_close;

  Tick result = FLAT;

  if (diff > FixedPt::fromInt(0))
  {
    result = UP;
  }
  else if (diff < FixedPt::fromInt(0))
  {
    result = DOWN;
  }
  else
  {
    result = FLAT;
  }

  if (_debug)
  {
    printf ("prev close is %s. ", _prev_close.c_str());
    printf ("new value is %s. ", close.c_str());
    printf ("tick value is %d\n", result);
  }

  if (update_type == ADD)
  {
    _prev_close = close;
  }

  //std::pair<unsigned, unsigned>
  int rv = ADLine::get_instance ().calculate_value (result, _symbol);
  //printf ("total_up is %u, total down is %u\n", rv.first, rv.second);
  printf ("ad_line is %s\n", rv);
  printf (" --------- \n");

  return FixedPt::fromInt (rv);
}

// ------- --------- --------- -------- ---------- ---------

ADLine& ADLine::get_instance ()
{
  static ADLine* ad_line = new ADLine ();

  return *ad_line;
}

ADLine::ADLine ()
{
  _total_up   = 0;
  _total_down = 0;
}

//std::pair<unsigned, unsigned>
int ADLine::calculate_value (
  const Tick   tick,
  const char(& symbol)[ORDER_SYMBOL_LEN])
{
  // 1. look up the symbol in the hmap
  //   - if new, just insert entry, set tick to FLAT and return
  // 2. retrieve the previous tick
  //   - switch previous tick
  //   -- UP->UP - no change
  //   -- UP->DOWN - _total_up - 1; _total_down + 1;
  //   -- UP->FLAT - _total_up - 1;
  //   -- FLAT->UP - _total_up + 1;
  //   -- FLAT->DOWN - _total_down + 1
  //   -- FLAT->FLAT - no change
  //   -- DOWN->UP - _total_down - 1; _total_up + 1
  //   -- DOWN->FLAT - _total_down -1
  //   -- DOWN->DOWN - no change

  TickMap::iterator iter = _tick_map.find (symbol);
  if (iter == _tick_map.end ())
  {
    _tick_map.insert (std::make_pair (CharArrayWrapper (symbol), tick));
    if (tick == UP)
    {
      ++_total_up;
    }
    else if (tick == DOWN)
    {
      ++_total_down;
    }
    return int(_total_up) - int(_total_down);
  }

  Tick& current_tick = iter->second;

  /* could do this as well
  _total_up   += tick;
  _total_down -= tick;

  where DOWN = -1; UP = 1;

  FLAT would be processed separately
  */

  switch (current_tick)
  {
    case UP:
      {
        switch (tick)
        {
          case UP: // no change
            break;
          case DOWN:
            --_total_up;
            ++_total_down;
            break;
          case FLAT:
            --_total_up;
            break;
          default:
            break;
        }
      }
      break;
    case DOWN:
      {
        switch (tick)
        {
          case UP:
            --_total_down;
            ++_total_up;
            break;
          case DOWN: // no change
            break;
          case FLAT:
            --_total_down;
            break;
          default:
            break;
        }
      }
      break;
    case FLAT:
      switch (tick)
      {
        case UP:
          ++_total_up;
          break;
        case DOWN:
          ++_total_down;
        case FLAT: // no change
          break; 
        default:
          break;
      }
      break;
    default: // do nothing here
      break;
  }

  current_tick = tick;

//  return std::make_pair (_total_up, _total_down);
  return int(_total_up) - int(_total_down);
}
