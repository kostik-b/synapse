// Copyright QUB 2017

#ifndef synapse_ewma_hc_kb_hh
#define synapse_ewma_hc_kb_hh

#include "hc_types.h"
#include "fixedpt_cpp.h"

class EwmaHc
{
public:
  EwmaHc ();

  void    set_alfa        (const FixedPt& alfa) { _alfa       = alfa; }

  void    set_last_value  (const FixedPt& value);

  FixedPt get_last_value  ()                    { return _last_value; }

  FixedPt calculate_new_value (const FixedPt&   update);

private:
  bool      _initialized;
  FixedPt   _last_value;
  FixedPt   _alfa;
}; // class EwmaHc


#endif
