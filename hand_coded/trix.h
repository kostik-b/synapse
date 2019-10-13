// Copyright QUB 2017

#ifndef synapse_trix_h
#define synapse_trix_h

#include "hc_types.h"
#include "fixedpt_cpp.h"
#include "ewma.h"

class Trix
{
public:
  Trix (const int  ewma_periods,
        const bool debug);

  bool initialize         (const FixedPt&   update);

  FixedPt calculate_value (const FixedPt&   update,
                           const UpdateType update_type);

private:
  EwmaHc      _ewma_1;
  EwmaHc      _ewma_2;
  EwmaHc      _ewma_3;

  const bool  _debug;

}; // class Trix

#endif
