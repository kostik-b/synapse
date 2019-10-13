// Copyright QUB 2017

#include "dmi.h"
#include <stdio.h>
#include "common.h"
#include "tracer.h"

FixedPt Dmi::calculate_pdm (const FixedPt& update_high, const FixedPt& update_low)
{
  if ((update_high - _prev_high) < (_prev_low - update_low))
  {
    return FixedPt::fromInt (0);
  }
  else if ((update_high - _prev_high) < FixedPt::fromInt (0))
  {
    return FixedPt::fromInt (0);
  }
  else
  {
    return (update_high - _prev_high);
  }
}

FixedPt Dmi::calculate_ndm (
  const FixedPt& update_high,
  const FixedPt& update_low)
{
  if ((_prev_low - update_low) < (update_high - _prev_high))
  {
    return FixedPt::fromInt (0);
  }
  else if ((_prev_low - update_low) < FixedPt::fromInt(0))
  {
    return FixedPt::fromInt (0);
  }
  else
  {
    return (_prev_low - update_low);
  }
}

FixedPt Dmi::calculate_tr (
  const FixedPt& update_close,
  const FixedPt& update_high,
  const FixedPt& update_low)
{
  return SynapseHC::max (_prev_close - update_low,
               SynapseHC::max (update_high - update_low, update_high - _prev_close)
             );
}

#if 0
static FixedPt calculate_pdi (FixedPt& pdm_sm, FixedPt& tr_sm)
{
  return pdm_sm / tr_sm;
}

static FixedPt calculate_ndi (FixedPt& ndm_sm, FixedPt& tr_sm)
{
  return ndm_sm / tr_sm;
}
#endif

Dmi::Dmi (const int   ewma_periods,
          const bool  debug)
  : _debug (debug)
{
  FixedPt alfa = FixedPt::fromInt (2) /
                  (FixedPt::fromInt (ewma_periods) + FixedPt::fromInt (1));
  printf ("DMI: ALFA is %s\n", alfa.c_str ());

  _ewma_pdm.set_alfa (alfa);
  _ewma_ndm.set_alfa (alfa);
  _ewma_tr.set_alfa  (alfa);
  _ewma_dx.set_alfa  (alfa);

}

bool Dmi::initialize (
   const FixedPt&   update_close,
   const FixedPt&   update_high,
   const FixedPt&   update_low)
{
  _prev_close = update_close;
  _prev_high  = update_high;
  _prev_low   = update_low;
}

FixedPt Dmi::calculate_value (
  const FixedPt&    update_close,
  const FixedPt&    update_high,
  const FixedPt&    update_low,
  const UpdateType  update_type)
{
  if (_debug)
  {
    printf ("Calculating DMI."); printf ("Update close is %s. ", update_close.c_str());
    printf ("Update high is %s. ", update_high.c_str()); printf ("Update low is %s. ", update_low.c_str());
    printf ("Update type is %s\n", (update_type == UPDATE) ? "UPDATE" : "ADD");

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("High", update_high);
    tracer.add_value ("Low",  update_low);
    tracer.add_value ("Close",update_close);
    tracer.add_type  ((update_type == UPDATE) ? "UPDATE" : "ADD");

  }
  // calculate pdm and ndm and tr
  FixedPt pdm = calculate_pdm (update_high, update_low);
  FixedPt ndm = calculate_ndm (update_high, update_low);

  FixedPt tr  = calculate_tr (update_close, update_high, update_low);
  if (_debug)
  {
    printf ("\tpdm is %s. ", pdm.c_str()); printf ("ndm is %s. ", ndm.c_str());
    printf ("tr is %s.\n", tr.c_str());

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("Pdm", pdm);
    tracer.add_value ("Ndm", ndm);
    tracer.add_value ("NewTrueRange", tr);
  }

  // then smooth all with ewma
  FixedPt pdm_smoothed = _ewma_pdm.calculate_new_value (pdm);
  FixedPt ndm_smoothed = _ewma_ndm.calculate_new_value (ndm);
  FixedPt tr_smoothed  = _ewma_tr.calculate_new_value  (tr);
  if (_debug)
  {
    printf ("\tpdm_smoothed is %s. ", pdm_smoothed.c_str());
    printf ("ndm_smoothed is %s. ",   ndm_smoothed.c_str());
    printf ("tr_smoothed is %s.\n",    tr_smoothed.c_str());
  }

  // then pdi and ndi
  FixedPt pdi          = pdm_smoothed / tr_smoothed;
  FixedPt ndi          = ndm_smoothed / tr_smoothed;
  // then dx
  if ((pdi + ndi).getC () == 0)
  {
    printf ("----> ABORT: avoiding the division by 0\n");
    return FixedPt::fromInt (0);
  }
  FixedPt dx           = 100*abs(pdi - ndi)/ (pdi + ndi);
  // smooth output with ewma
  FixedPt result       = _ewma_dx.calculate_new_value (dx);
  // result!
  if (_debug)
  {
    printf ("\tpdi is %s. ", pdi.c_str());
    printf ("ndi is %s. ", ndi.c_str());
    printf ("dx is %s. ",  dx.c_str());
    printf ("result is %s.\n", result.c_str());
    printf ("------------------\n");

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("Pdi", pdi);
    tracer.add_value ("Ndi", ndi);
    tracer.add_value ("Dx", dx);
  }

  if (update_type == ADD)
  {
    _prev_close = update_close;
    _prev_high  = update_high;
    _prev_low   = update_low;

    _ewma_pdm.set_last_value (pdm_smoothed);
    _ewma_ndm.set_last_value (ndm_smoothed);
    _ewma_tr .set_last_value (tr_smoothed);

    _ewma_dx .set_last_value (result);
  }

  return result;
}

