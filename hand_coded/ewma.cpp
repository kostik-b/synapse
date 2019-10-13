// Copyright QUB 2017

#include "ewma.h"

EwmaHc::EwmaHc ()
{
  _initialized= false;
  _last_value = FixedPt::fromInt (0);
  _alfa       = FixedPt::fromInt (0);
}

void EwmaHc::set_last_value  (const FixedPt& value)
{
  _last_value   = value;
  _initialized  = true;
}

FixedPt EwmaHc::calculate_new_value (
  const FixedPt&    update)
{
  if (!_initialized)
  {
    _last_value  = update;
    _initialized = true;
    return _last_value;
  }
  FixedPt result;
  // all other cases
  result = (1 - _alfa)*update + _alfa*_last_value;

  return result;
}
