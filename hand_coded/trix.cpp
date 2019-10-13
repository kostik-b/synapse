// Copyright QUB 2017

#include "trix.h"
#include "common.h"
#include <stdio.h>

Trix::Trix (const int   ewma_periods,
            const bool  debug)
  : _debug (debug)
{
  FixedPt alfa = FixedPt::fromInt (2) /
                  (FixedPt::fromInt (ewma_periods) + FixedPt::fromInt (1));
  printf ("TRIX: ALFA is %s\n", alfa.c_str ());

  _ewma_1.set_alfa (alfa);
  _ewma_2.set_alfa (alfa);
  _ewma_3.set_alfa (alfa);
}

bool Trix::initialize (const FixedPt& update)
{
  _ewma_1.set_last_value (update);
  _ewma_2.set_last_value (update);
  _ewma_3.set_last_value (update);

  return true;
}

FixedPt Trix::calculate_value (
  const FixedPt&    update, 
  const UpdateType  update_type)
{
  if (_debug)
  {
    printf ("Calculating TRIX value. Update type is %s, update value is %s\n",
                      (update_type == UPDATE) ? "UPDATE" : "ADD", update.c_str());
  }
  // calculate first ewma
  FixedPt ewma_1  = _ewma_1.calculate_new_value (update);
  if (_debug)
  {
    printf ("\tewma 1 is %s\n", ewma_1.c_str());
  }

  // calculate second ewma
  FixedPt ewma_2  = _ewma_2.calculate_new_value (ewma_1);
  if (_debug)
  {
    printf ("\tewma 2 is %s\n", ewma_2.c_str());
  }

  FixedPt ewma_3  = _ewma_3.calculate_new_value (ewma_2);
  if (_debug)
  {
    printf ("\tewma 3 is %s\n", ewma_3.c_str());
  }

  FixedPt last_ewma_3 = _ewma_3.get_last_value ();

  FixedPt result = 100*((ewma_3 - last_ewma_3)/last_ewma_3);

  if (_debug)
  {
    printf ("result is %s\n.", result.c_str());
    printf ("---------\n");
  }

  if (update_type == ADD)
  {
    _ewma_1.set_last_value (ewma_1);
    _ewma_2.set_last_value (ewma_2);
    _ewma_3.set_last_value (ewma_3);
  }

  return result;
}

