// Copyright Queen's University of Belfast 2012

#include <stdio.h>
#include "cycles_counter.hh"

void Synapse::CyclesCounter::set_reporting_interval (
  const int      reporting_interval)
{
  _reporting_interval = reporting_interval;
}



bool Synapse::CyclesCounter::update_counter (
  const uint64_t value)
{
  _cycles_counter   += value;
  ++_current_interval;

  if (_current_interval >= _reporting_interval)
  {
    _current_interval = 0;
    update_prev_counters ();
    return true;
  }
  else
  {
    return false;
  }
}

uint64_t Synapse::CyclesCounter::get_counter ()
{
  return _cycles_counter;
}

// TODO: this should be removed
void Synapse::CyclesCounter::reset_counter ()
{
  _cycles_counter = 0;
}

void Synapse::CyclesCounter::update_prev_counters ()
{
  _prev_counters [_num_prev_counters % MAX_PREV_COUNTERS] = _cycles_counter;

  ++_num_prev_counters;
}

void Synapse::CyclesCounter::get_x_last_counters_str (
  char*          buffer,
  const size_t   buf_len,
  const int      num_of_intervals)
{
  int remaining_size = (int)buf_len;

  for (int i = 0; (i < num_of_intervals) && (int(_num_prev_counters) - i - 1) >= 0; ++i)
  {
    int bytes_written = snprintf (buffer + buf_len - remaining_size, remaining_size, "%llu,", _prev_counters [(_num_prev_counters - i - 1) % MAX_PREV_COUNTERS]);
    remaining_size -= bytes_written;

    if (remaining_size < 1)
    {
      break;
    }
  }
}

