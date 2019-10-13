// Copyright QUB 2018

#ifndef synapse__indicator_manager_h
#define synapse__indicator_manager_h

#include "hc_types.h"
#include "dmi.h"
#include "vortex.h"
#include "trix.h"
#include "running_stat.hh"
#include "ad_line.h"

/*
  In general one symbol corresponds to one indicator manager.
  I.e. indicator manager manages all the indicators for a particular symbol.
  For the purpose of my paper we do not need to run more
  than one indicator in any instance of time, so although we have
  the physical ability to run multiple indicators, we measure just
  the running of one indicator - see how we use RunningStat and friends.
*/

class IndicatorManager
{
public:
  IndicatorManager (const Params& pars,
                    const char (& symbol_buffer)[ORDER_SYMBOL_LEN]); // by reference
  ~IndicatorManager();

  inline
  bool initialized            () { return _initialized; }

  void initialize_indicators  (const FixedPt& high, const FixedPt& low,
                               const FixedPt& open, const FixedPt& close);

  void emit_add               (const FixedPt& high, const FixedPt& low,
                               const FixedPt& open, const FixedPt& close);

  void emit_update            (const FixedPt& high, const FixedPt& low,
                               const FixedPt& open, const FixedPt& close,
                               const uint64_t  cycles_start);

private:
  IndicatorManager (const IndicatorManager& copy); // no copy constructor
  IndicatorManager& operator= (const IndicatorManager& rhs); // no assignment operator

  void configure_indicators   (const Params& pars);

private:
  Trix*       _trix;
  Dmi*        _dmi;
  Vortex*     _vortex;
  ADLineTick* _ad_line_tick;

  // these are used for measurements
  RunningStat _running_stats;
  FixedPt*    _all_values;
  int         _all_values_counter;

  // symbol
  char        _symbol [ORDER_SYMBOL_LEN];

  // status
  bool        _initialized;
}; // class IndicatorManager

#endif
