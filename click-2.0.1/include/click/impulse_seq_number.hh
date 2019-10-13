// Copyright Queen's University Belfast 2012

#ifndef Synapse__ImpulseSeqNumberHH
#define Synapse__ImpulseSeqNumberHH

#include <click/config.h>

namespace Synapse
{

class ImpulseNumber
{
  public:
    static uint64_t  increment_and_get_next_number ()
    {
      // TODO: need to put lock here for multithreaded use

      // we start count with 0
      ++_impulse_seq_number;
      return _impulse_seq_number;
    }

  private:

    static uint64_t _impulse_seq_number;

}; // class ImpulseNumber


} // namespace Synapse

#endif
