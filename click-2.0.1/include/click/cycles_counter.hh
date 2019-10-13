// Copyright Queen's University of Belfast 2012

#ifndef Synapse_CyclesCounter_H
#define Synapse_CyclesCounter_H

#include <click/config.h>
#include <click/glue.hh>

CLICK_DECLS

#define  MAX_PREV_COUNTERS 100

namespace Synapse
{

class CyclesCounter
{
public:
  CyclesCounter ()
    : _cycles_counter     (0)
    , _reporting_interval (0)
    , _current_interval   (0)
    , _num_prev_counters  (0)
  {
    memset (_prev_counters, 0, sizeof(_prev_counters));
  }
  ~CyclesCounter() {}

  void      set_reporting_interval (const int      reporting_interval);
  bool      update_counter         (const uint64_t value);
  uint64_t  get_counter            ();
  void      reset_counter          ();

  void      update_prev_counters   ();
  void      get_x_last_counters_str(char*          buffer,
                                    const size_t   buf_len,
                                    const int      num_of_intervals);

private:
  uint64_t  _cycles_counter;
  int       _reporting_interval;
  int       _current_interval;

  unsigned int       _num_prev_counters;
  uint64_t  _prev_counters[MAX_PREV_COUNTERS];
}; // class CyclesCounter

} // namespace Synapse

CLICK_ENDDECLS
#endif

