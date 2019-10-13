// Copyright QUB 2017

#ifndef FIXEDPT_CPP_H
#define FIXEDPT_CPP_H

#include <click/config.h>
#include <click/glue.hh>
#include "fixedptc.h"

CLICK_DECLS

static const size_t MAX_FIXEDPT_STR_LEN = 20;

class FixedPt
{
public:
  FixedPt ()
  {
    _value = 0;
    _valid = true;
  }

  inline static FixedPt fromInt (const int value)
  {
    FixedPt ret;
    ret._value = fixedpt_fromint (value);

    return ret;
  }

  inline static FixedPt fromC  (const fixedpt& value)
  {
    FixedPt ret;
    ret._value = value;

    return ret;
  }

  const char* c_str (const int max_dec = 4) const
  {
    return fixedpt_cstr (_value, max_dec);
  }

  inline FixedPt operator+ (const FixedPt& rhs) const
  {
    return FixedPt::fromC (fixedpt_add (_value, rhs._value));
  }

  inline FixedPt operator- (const FixedPt& rhs) const
  {
    return FixedPt::fromC (fixedpt_sub (_value, rhs._value));
  }

  inline FixedPt operator* (const FixedPt& rhs) const
  {
    return FixedPt::fromC (fixedpt_mul (_value, rhs._value));
  }

  inline FixedPt operator/ (const FixedPt& rhs) const
  {
    return FixedPt::fromC (fixedpt_div (_value, rhs._value));
  }

  inline void set_valid (const bool valid)
  {
    _valid = valid;
  }

  inline const bool get_valid ()
  {
    return _valid;
  }

  inline const fixedpt    getC () const
  {
    return _value;
  }

private: // the below is very dangerous since we may not be able to distinguish between
         // int and fixedpt types, so we forbid it altogether
  FixedPt& operator= (fixedpt value);
  FixedPt (const int value);

private:
  fixedpt _value;
  bool    _valid;

private:
}; // class FixedPt

FixedPt operator+ (const int lhs, FixedPt& rhs);

FixedPt operator- (const int lhs, FixedPt& rhs);

FixedPt operator* (const int lhs,      const FixedPt& rhs);

bool    operator> (const FixedPt& lhs, const FixedPt& rhs);

bool    operator< (const FixedPt& lhs, const FixedPt& rhs);

FixedPt fpt_abs   (const FixedPt& value);

CLICK_ENDDECLS
#endif
