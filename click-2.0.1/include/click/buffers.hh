// Copyright QUB 2017

#ifndef Synapse_Buffers_H
#define Synapse_Buffers_H

#include <click/config.h>
#include <click/glue.hh>
#include "circ_array.hpp"
#include "fixedpt_cpp.h"

CLICK_DECLS

typedef CircArray<FixedPt>    RingBuffer;

enum BufferStates
{
  INITIAL,
  ACCUMULATING,
  ACCUMULATING_POP,
  ACCUMULATED,
  ACCUMULATED_TEMP,
  ACCUMULATED_TEMP_POP
};

class Buffers
{
public:
  Buffers (const int num_ports, const int buf_len, const bool debug);
  ~Buffers ();

  RingBuffer& operator[]    (const int index);
  
  void  add_new_value       (const FixedPt& value,
                             const uint64_t tstamp,
                             const int      port);

  void  add_new_value_temp  (const FixedPt& value,
                             const uint64_t tstamp,
                             const int      port);

  bool  new_value_check     (const int port, const uint64_t tstamp);

  void  reset               ();

  bool  is_update_complete  () { return ((_state == ACCUMULATED) ||
                                         (_state == ACCUMULATED_TEMP) ||
                                         (_state == ACCUMULATED_TEMP_POP)); }

  int   get_num_ports       () { return _num_ports; }

private:
  RingBuffer**  _buffers;
  char*         _updated;
  FixedPt*      _temp_updates;
  uint64_t      _tstamp;
  const int     _num_ports;
  BufferStates  _state;
  const bool    _debug;

private:
  Buffers (); // no default ctor
  Buffers (const Buffers& copy); // no copy ctor
  Buffers& operator= (const Buffers& rhs); // no assignment operator
}; // class Buffers

CLICK_ENDDECLS
#endif
