// Copyright QUB 2017

#ifndef synapse_common_h
#define synapse_common_h

#include "fixedpt_cpp.h"
#include "hc_types.h"

namespace SynapseHC
{

static FixedPt max (const FixedPt& a, const FixedPt& b)
{
  return ((a > b) ? a : b);
}

} // namespace SynapseHC

#endif
