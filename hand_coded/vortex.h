// Copyright QUB 2017

#ifndef synapse_vortex_h
#define synapse_vortex_h

#include "fixedpt_cpp.h"
#include "circ_array.hpp"
#include <utility>
#include "hc_types.h"

typedef CircArray<FixedPt> CircArrayFixedPt;

class SumRingBuffer
{
public:
  SumRingBuffer (const size_t capacity);

  FixedPt get_total_sum       ();
  void    push_back_new_value (const FixedPt& new_value);
  bool    is_initialized      () { return _initialized; }

  CircArrayFixedPt  _ring_buffer;
private:
  FixedPt           _last_sum;

  bool              _initialized;
}; // struct SumRingBufffer

class Vortex
{
public:
  Vortex (const size_t  sum_len,
          const bool    debug);

  bool    initialize      ( const FixedPt&    update_close,
                            const FixedPt&    update_high,
                            const FixedPt&    update_low);

  std::pair<FixedPt, FixedPt>
          calculate_value ( const FixedPt&    update_close,
                            const FixedPt&    update_high,
                            const FixedPt&    update_low,
                            const UpdateType  update_type);

private:
  FixedPt       _prev_close;
  FixedPt       _prev_high;
  FixedPt       _prev_low;

  SumRingBuffer _vmu_sum;
  SumRingBuffer _vmd_sum;
  SumRingBuffer _tr_sum;

  const bool    _debug;

}; // class Vortex

#endif
