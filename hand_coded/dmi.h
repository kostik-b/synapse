// Copyright QUB 2017

#ifndef synapse_dmi_h
#define synapse_dmi_h

#include "fixedpt_cpp.h"
#include "hc_types.h"
#include "ewma.h"

class Dmi
{
public:
  Dmi (const int  ewma_periods,
       const bool debug);

  bool    initialize     (const FixedPt&          update_close,
                          const FixedPt&          update_high,
                          const FixedPt&          update_low);

  FixedPt calculate_value(const FixedPt&          update_close,
                          const FixedPt&          update_high,
                          const FixedPt&          update_low,
                          const UpdateType  update_type);

private:
  FixedPt calculate_pdm (const FixedPt& update_high,
                         const FixedPt& update_low);

  FixedPt calculate_ndm (const FixedPt& update_high,
                         const FixedPt& update_low);

  FixedPt calculate_tr  (const FixedPt& update_close,
                         const FixedPt& update_high,
                         const FixedPt& update_low);
private:
//  FixedPt _pdm_smoothed_last;
//  FixedPt _ndm_smoothed_last;
//  FixedPt _tr_smoothed_last;

  FixedPt _prev_close;
  FixedPt _prev_high;
  FixedPt _prev_low;

  EwmaHc _ewma_pdm;
  EwmaHc _ewma_ndm;
  EwmaHc _ewma_tr;

  EwmaHc _ewma_dx;
// ---------

  const bool _debug;

}; // class Dmi

#endif
