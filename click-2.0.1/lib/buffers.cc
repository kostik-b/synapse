// Copyright QUB 2017

#include <click/buffers.hh>

#define CLICK_DEBUG if (_debug) click_chatter

CLICK_DECLS

Buffers::Buffers (const int num_ports, const int buf_len, const bool debug)
  : _tstamp (0)
  , _num_ports (num_ports)
  , _debug     (debug)
{
  _buffers = new RingBuffer* [num_ports];
  for (int i = 0; i < num_ports; ++i)
  {
    _buffers[i] = new RingBuffer (buf_len);
  }

  _updated      = new char [num_ports];

  _temp_updates = new FixedPt [num_ports];

  reset ();

  _state = INITIAL;
}

void Buffers::reset ()
{
  memset (_updated, 'N', _num_ports);
}

static bool check_updated (char* updated, const int len)
{
  for (int i = 0; i < len; ++i)
  {
    if (updated[i] != 'Y')
    {
      return false;
    }
  }

  return true;
}

bool Buffers::new_value_check (const int port, const uint64_t tstamp)
{
  if ((port < 0) || (port > _num_ports - 1))
  {
    CLICK_DEBUG ("Buffers: incorrect port - outside of range");
    return false;
  }

  CLICK_DEBUG ("Buffers: check tstamp %lld (prev. tstamp is %lld)", tstamp, _tstamp);

  if (tstamp > _tstamp)
  {
    CLICK_DEBUG ("Buffers KB: a higher tstamp has arrived");
    if ((_state == ACCUMULATING) || (_state == ACCUMULATING_POP))
    {
      CLICK_DEBUG ("Theresa Rubel! We've got a new timestamp"
                     " before accumulating all the updates!");
      // TODO: we really need to abort here, how to do this safely in kernel space???
    }
    reset ();

    _tstamp = tstamp;

    // restore all the saved values
    if ((_state == ACCUMULATED_TEMP) || (_state == ACCUMULATED_TEMP_POP))
    {
      for (int i = 0; i < _num_ports; ++i)
      {
        _buffers[i]->pop_front (); // remove the temp vars
      }
      
      if (_state == ACCUMULATED_TEMP_POP)
      {
        // restore the saved values
        for (int i = 0; i < _num_ports; ++i)
        {
          _buffers[i]->push_back (_temp_updates[i], true);
        }
      }
    }

    _state  = ACCUMULATING;
  }

  return true;
}

void Buffers::add_new_value (
  const FixedPt&  value,
  const uint64_t  tstamp,
  const int       port)
{
  CLICK_DEBUG ("Add New Value: value %s, port %d",
                  value.c_str (), port);
  if (!new_value_check (port, tstamp))
  {
    return;
  }

  _buffers[port]->push_front (value, true);
  _updated[port] = 'Y';

  if (check_updated (_updated, _num_ports))
  {
    _state = ACCUMULATED;
  }
}

void Buffers::add_new_value_temp (
  const FixedPt&  value,
  const uint64_t  tstamp,
  const int       port)
{
  CLICK_DEBUG ("Add New Value Temp: value %s, port %d",
                  value.c_str (), port);
  if (!new_value_check (port, tstamp))
  {
    return;
  }
  // I want to pop back the value for that buffer (if any) and save them
  if (_buffers[port]->occupancy () == _buffers[port]->capacity ())
  {
    _temp_updates[port] = _buffers[port]->pop_back ();
    _state = ACCUMULATING_POP;
  }
  // then I add  the new value
  _buffers[port]->push_front (value, true);
  // updated works as usual
  _updated[port] = 'Y';

  if (check_updated (_updated, _num_ports))
  {
    if (_state == ACCUMULATING)
    {
      _state = ACCUMULATED_TEMP;
    }
    else
    {
      _state = ACCUMULATED_TEMP_POP;
    }
  }
}

RingBuffer& Buffers::operator[] (const int index)
{
  assert (index >= 0);
  assert (index < _num_ports);
/*
  if ((index < 0) || (index > _num_ports - 1))
  {
    invalid_index_exception exc;
    throw exc;
  }
*/
  return *(_buffers[index]);
}

CLICK_ENDDECLS
