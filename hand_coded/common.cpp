// Copyright QUB 2017


#include "common.h"

FixedPt SynapseHC::calculate_ewma (
  FixedPt& value,
  FixedPt& last_ewma,
  FixedPt& alfa,
  const UpdateType update_type)
{
  // special case - the first value
  FixedPt result;
  if ((!last_ewma.get_valid ()) && (update_type != ADD))
  {
    return value;
  }
  // all other cases
  result = (1 - alfa)*value + alfa*last_ewma;


  if (update_type == ADD)
  {
    last_ewma = result;
    last_ewma.set_valid (true);
  }

  return result;
}

