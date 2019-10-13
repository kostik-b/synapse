// Copyright QUB 2017

#include <click/fixedpt_cpp.h>

CLICK_DECLS

FixedPt operator+ (const int lhs, FixedPt& rhs)
{
  return FixedPt::fromInt (lhs) + rhs;
}

FixedPt operator- (const int lhs, FixedPt& rhs)
{
  return FixedPt::fromInt (lhs) - rhs;
}

FixedPt operator* (const int lhs, const FixedPt& rhs)
{
  return FixedPt::fromInt (lhs) * rhs;
}

bool operator>    (const FixedPt& lhs, const FixedPt& rhs)
{
  return (lhs.getC() > rhs.getC());
}

bool operator<    (const FixedPt& lhs, const FixedPt& rhs)
{
  return (lhs.getC() < rhs.getC());
}

FixedPt fpt_abs (const FixedPt& value)
{
  if (value.getC () < 0)
  {
    return FixedPt::fromC (-value.getC());
  }

  return value;
}
CLICK_ENDDECLS
