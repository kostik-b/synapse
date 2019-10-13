

#include <click/running_stat.hh>

void
Synapse::RunningStat::update_counter (
  const uint64_t value)
{
  // 0. update N
  ++_stats._N;
  // 1. update the total cycles
  _stats._aggregate_cycles += value;
  // 2. update highest
  if (value > _stats._highest)
  {
    _stats._highest = value;
  }
  // 3. update lowest
  if (_stats._lowest == 0) // start-up scenario
  {
    _stats._lowest = value;
  }
  else if (value < _stats._lowest)
  {
    _stats._lowest = value;
  }
  // 4. update s1
  _stats._s1 += value;
  // 5. update s2
  _stats._s2 += value*value;
}

const Synapse::Stats&
Synapse::RunningStat::get_running_stats ()
{
  return _stats;
}
