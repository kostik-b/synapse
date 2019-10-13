#ifndef CLICK_INDICATOR_BASE_HH
#define CLICK_INDICATOR_BASE_HH

#include <click/element.hh>
#include <click/string.hh>
#include <click/circ_array.hpp>
#include <click/global_sizes.hh>
#include <click/fixedpt_cpp.h>
#include <click/buffers.hh>

CLICK_DECLS

enum OpMode
{
  STARTUP,
  NORMAL,
  NAIVE
};

enum CacheType
{
  FIXEDPT_CACHE,
  RING_BUFFER_CACHE,
  INCREMENT
};

typedef CircArray<FixedPt>    RingBuffer;

struct CacheStruct
{
  CacheType     _type;
  FixedPt       _value;
  RingBuffer    _ring_buffer;
};

typedef Vector<CacheStruct>   VectorCache;

class IndicatorBase : public Element
{
  public:
    IndicatorBase  ();
    ~IndicatorBase ();

    const char *port_count() const		{ return "1-/-"; }
    int configure         (Vector<String>&, ErrorHandler*);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push           (int                 port,
                         Packet*             p);

    void send_msg_value (Packet&             packet,
                         const FixedPt&      value,
                         const uint64_t      timestamp,
                         const PacketAppType msg_type);

    virtual FixedPt       initialize_element  (Buffers&         buffers)      = 0;
    // takes in a buffer, returns 1 value
    virtual FixedPt       process_naive       (Buffers&         buffers)      = 0;
    // takes in a buffer, returns 1 value + cache
    virtual VectorCache&  process_ext         (Buffers&         buffers)      = 0;
    // takes in an increment + cache, returnes 1 value + cache
    virtual VectorCache&  process_opt_ext     (Vector<FixedPt>& increments,
                                               VectorCache&     cache)        = 0;

  protected:
    bool                        _debug;
    bool                        _active;
    OpMode                      _op_mode;

  private:
    Buffers*                    _buffers;
    VectorCache                 _cache;
    bool                        _initialized;

#ifdef CLICK_LINUXMODULE
    bool                        _cpu : 1;
#endif

}; // class IndicatorBase

CLICK_ENDDECLS
#endif
