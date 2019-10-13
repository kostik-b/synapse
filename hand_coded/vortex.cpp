// Copyright QUB 2017

#include "vortex.h"
#include "common.h"
#include "tracer.h"

SumRingBuffer::SumRingBuffer (const size_t capacity)
  : _ring_buffer (capacity)
  , _initialized (false)
{
}

FixedPt SumRingBuffer::get_total_sum ()
{
  return _last_sum;
}

void SumRingBuffer::push_back_new_value (const FixedPt& new_value)
{
  _initialized = true;

  _ring_buffer.push_back (new_value, true /*overwrite*/);

  // now need to update the total sum
  _last_sum = FixedPt::fromInt (0);
  for (int i = 0; i < _ring_buffer.occupancy (); ++i)
  {
    _last_sum = _last_sum + _ring_buffer[i];
  }
}

Vortex::Vortex (const size_t  sum_len,
                const bool    debug)
  : _vmu_sum (sum_len - 1)
  , _vmd_sum (sum_len - 1)
  , _tr_sum  (sum_len - 1)
  , _debug   (debug)
{
  
}

bool Vortex::initialize (
  const FixedPt& update_close,
  const FixedPt& update_high,
  const FixedPt& update_low)
{
  _prev_close = update_close;
  _prev_high  = update_high;
  _prev_low   = update_low;

  return true;
}

static void print_buffer (CircArrayFixedPt& ring_buffer)
{
  for (int i = 0; i < ring_buffer.occupancy (); ++i)
  {
    printf (" | %d -> %s ", i, ring_buffer[i].c_str ());
  }
  printf (" |\n");
}

std::pair<FixedPt,FixedPt> Vortex::calculate_value (
  const FixedPt&    update_close,
  const FixedPt&    update_high,
  const FixedPt&    update_low,
  const UpdateType  update_type)
{
  if (_debug)
  {
    printf ("Calculating Vortex."); printf ("Update close is %s. ", update_close.c_str());
    printf ("Update high is %s. ", update_high.c_str()); printf ("Update low is %s. ", update_low.c_str());
    printf ("Update type is %s\n", (update_type == UPDATE) ? "UPDATE" : "ADD");

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("High", update_high);
    tracer.add_value ("Low",  update_low);
    tracer.add_value ("Close",update_close);
    tracer.add_type  ((update_type == UPDATE) ? "UPDATE" : "ADD");
  }
  // vmu, vmd, tr
  FixedPt vmu = abs (update_high - _prev_low);
  FixedPt vmd = abs (update_low - _prev_high);
  FixedPt tr  = SynapseHC::max (_prev_close - update_low,
                                SynapseHC::max (update_high - update_low,
                                                update_high - _prev_close)
                );

  // catering for the first update situation, otherwise the sums will be 0
  // until the first add arrives
  if ((update_type == UPDATE) && (_vmu_sum.is_initialized () == false))
  {
    _vmu_sum.push_back_new_value (vmu);
    _vmd_sum.push_back_new_value (vmd);
    _tr_sum .push_back_new_value (tr);
  }

  if (_debug)
  {
    printf ("vmu is %s. ", vmu.c_str());
    printf ("vmd is %s. ", vmd.c_str());
    printf ("tr is %s\n", tr.c_str());

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("Vmu", vmu);
    tracer.add_value ("Vmd", vmd);
    tracer.add_value ("NewTrueRange", tr);
  }

  // sum (21) for each - will be using the circ buffer
  FixedPt vmu_total = _vmu_sum.get_total_sum ();
  FixedPt vmd_total = _vmd_sum.get_total_sum ();
  FixedPt tr_total  = _tr_sum.get_total_sum  ();
  
  // viu, vid
  FixedPt viu = (vmu_total + vmu)/(tr_total + tr);
  FixedPt vid = (vmd_total + vmd)/(tr_total + tr);

  if (_debug)
  {
    printf ("vmu_total is %s. ", (vmu_total + vmu).c_str());
    printf ("vmd_total is %s. ", (vmd_total + vmd).c_str());
    printf ("tr_total is %s\n", (tr_total + tr).c_str());

    printf ("viu is %s. ", viu.c_str()); printf ("vid is %s.\n", vid.c_str());
    printf ("------------------\n");

    Tracer& tracer = Tracer::get_instance ();
    tracer.add_value ("Viu", viu);
    tracer.add_value ("Vid", vid);
    tracer.add_value ("Sum", vmu_total + vmu); // KB
  }
  // finally update the buffers
  if (update_type == ADD)
  {
    _vmu_sum.push_back_new_value (vmu);
    _vmd_sum.push_back_new_value (vmd);
    _tr_sum .push_back_new_value (tr);

    _prev_close = update_close;
    _prev_low   = update_low;
    _prev_high  = update_high;
  }

  return std::make_pair (viu, vid);
  
}
