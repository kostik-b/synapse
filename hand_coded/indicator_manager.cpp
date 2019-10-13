// Copyright QUB 2018

#include "indicator_manager.h"
#include <cstring>

static const int s_all_values_size = 5000*2; // vortex has 2 values

#define DELETE(x) \
  if (x)          \
  {               \
    delete x;     \
  }

static inline uint64_t gcc_rdtsc (void)
{
  uint64_t msr;

  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          :
          : "rdx");

  return msr;
}

IndicatorManager::IndicatorManager (
  const Params& pars,
  const char (& symbol_buffer)[ORDER_SYMBOL_LEN])
{
  _trix         = NULL;
  _dmi          = NULL;
  _vortex       = NULL;
  _ad_line_tick = NULL;

  _all_values         = new FixedPt [s_all_values_size];
  _all_values_counter = 0;

  memcpy (_symbol, symbol_buffer, ORDER_SYMBOL_LEN);

  _initialized = false;

  configure_indicators (pars);
}

IndicatorManager::~IndicatorManager ()
{
  DELETE (_trix);
  DELETE (_dmi);
  DELETE (_vortex);
  DELETE (_ad_line_tick);

  for (int i = 0; i < _all_values_counter; ++i)
  {
    printf ("\tValue %d is %s\n", i, _all_values[i].c_str());
  }

  delete [] _all_values;

  const Stats& stats = _running_stats.get_running_stats ();
  printf ("RunningStat:N is %d,\tAggr. cycles is %lu,\tHigh is %lu\n",
                  stats._N, stats._aggregate_cycles, stats._highest);
  printf ("RunningStat:Low is %lu,\ts1 is %lu,\ts2 is %lu\n",
                  stats._lowest, stats._s1, stats._s2);
}

void IndicatorManager::configure_indicators (const Params& pars)
{
  /*
    Since we only run one indicator at a time we simply reuse the
    "_ewma_periods" field as it would only pertain to one indicator at
    a time. For vortex we use it to mean "the length of the sum buffer"
  */
  if (pars._is_trix)
  {
    _trix = new Trix (pars._ewma_periods, pars._indicator_debug);
  }
  if (pars._is_dmi)
  {
    _dmi = new Dmi (pars._ewma_periods, pars._indicator_debug);
  }
  if (pars._is_vortex)
  {
    _vortex = new Vortex (pars._ewma_periods,  // should be "sum len"
                       pars._indicator_debug);     // but we don't want an
  }                                                // extra field in params
  if (pars._is_adline)
  {
    _ad_line_tick = new ADLineTick (pars._indicator_debug, _symbol);
  }
}

void IndicatorManager::initialize_indicators  (
  const FixedPt& high, const FixedPt& low,
  const FixedPt& open, const FixedPt& close)
{
  if (_trix)
  {
    _trix->initialize   (close);
  }
  if (_dmi)
  {
    _dmi->initialize    (close, high, low);
  }
  if (_vortex)
  {
    _vortex->initialize (close, high, low);
  }
  if (_ad_line_tick)
  {
    _ad_line_tick->initialize (close);
  }

  _initialized = true;
}

void IndicatorManager::emit_add (
  const FixedPt& high, const FixedPt& low,
  const FixedPt& open, const FixedPt& close)
{
  if (_trix)
  {
    _trix->calculate_value    (close, ADD);
  }
  if (_dmi)
  {
    _dmi->calculate_value     (close, high, low, ADD);
  }
  if (_vortex)
  {
    _vortex->calculate_value  (close, high, low, ADD);
  }
  if (_ad_line_tick)
  {
    _ad_line_tick->calculate_value (close, ADD);
  }
}

void IndicatorManager::emit_update (
  const FixedPt& high, const FixedPt& low,
  const FixedPt& open, const FixedPt& close,
  const uint64_t  cycles_start)
{
  // first calculate the value
  FixedPt                       result_trix;
  FixedPt                       result_dmi;
  std::pair<FixedPt, FixedPt>   result_vortex;
  // std::pair<unsigned, unsigned> result_adline;
  FixedPt                       result_adline;
  if (_trix)
  {
    result_trix = _trix->calculate_value  (close, UPDATE);
  }
  if (_dmi)
  { 
    result_dmi = _dmi->calculate_value   (close, high, low, UPDATE);
  }
  if (_vortex)
  {
    result_vortex = _vortex->calculate_value(close, high, low, UPDATE);
  }
  if (_ad_line_tick)
  {
    result_adline = _ad_line_tick->calculate_value (close, UPDATE);
  }

  // now add the value to the list of all values - we need this
  // to compare that Click-based calculations are the same
  if (_all_values_counter < (s_all_values_size - 1))
  {
    if (_trix)
    {
      _all_values[_all_values_counter] = result_trix;
    }
    if (_dmi)
    {
      _all_values[_all_values_counter] = result_dmi;
    }
    if (_vortex)
    {
      _all_values[_all_values_counter++]  = result_vortex.first;
      _all_values[_all_values_counter]    = result_vortex.second;
    }
    if (_ad_line_tick)
    {
//      _all_values[_all_values_counter++]  = FixedPt::fromInt(result_adline.first);
      _all_values[_all_values_counter]    = result_adline;
    }
    ++_all_values_counter;
  }

  // now update the running stats
  const uint64_t        cycles_now    = gcc_rdtsc ();

  _running_stats.update_counter (cycles_now - cycles_start);

  // printf ("KB: cycles_now is %lu cycles_start is %lu\n", cycles_now, cycles_start);

}

