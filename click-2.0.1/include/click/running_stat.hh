// Copyright Queen's University of Belfast 2012

#ifndef Synapse_RunningStat_H
#define Synapse_RunningStat_H

#include <click/config.h>
#include <click/glue.hh>

CLICK_DECLS


namespace Synapse
{

struct Stats
{
  Stats ()
    : _N                (0)
    , _aggregate_cycles (0)
    , _highest          (0)
    , _lowest           (0)
    , _s1               (0)
    , _s2               (0)
  {}

  uint32_t _N;
  uint64_t _aggregate_cycles;
  uint64_t _highest;
  uint64_t _lowest;
  uint64_t _s1;
  uint64_t _s2;
};// class Stats

class RunningStat
{
public:
  RunningStat ()
  {
  }
  ~RunningStat() {}

  void          update_counter         (const uint64_t value);
  const Stats&  get_running_stats      ();

private:
  Stats _stats;

}; // class RunningStat

} // namespace Synapse

CLICK_ENDDECLS
#endif

