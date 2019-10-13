#ifndef CLICK_DIRECTIONAL_MOVEMENT_HH
#define CLICK_DIRECTIONAL_MOVEMENT_HH

#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>
#include <click/array_wrapper.hh>

#include <click/global_sizes.hh>

CLICK_DECLS

struct HighLow
{
  HighLow ()
    : _prev_low             (0)
    , _current_low          (0)
    , _prev_high            (0)
    , _current_high         (0)
    , _current_interval_num (-1)
    , _prev_interval_num    (-1)
  {}

  fixedpt   _prev_low;
  fixedpt   _current_low;
  fixedpt   _prev_high;
  fixedpt   _current_high;
  int64_t   _current_interval_num;
  int64_t   _prev_interval_num;
}; // struct HighLow

class DirectionalMovement : public Element
{
  public:
    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, HighLow>                    HighLowMap;

    DirectionalMovement();
    ~DirectionalMovement();

    const char *class_name() const		{ return "DirectionalMovement"; }
    const char *port_count() const		{ return "1/2"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push(int port, Packet *p);

  private:

    void             calculate_dm          (fixedpt&      plus_dm,
                                            fixedpt&      minus_dm,
                                            const fixedpt previous_high,
                                            const fixedpt previous_low,
                                            const fixedpt current_high,
                                            const fixedpt current_low);

  private:
    HighLowMap                  _high_low_map;
    int64_t                     _interval_len; // interval in tsc

    bool                        _debug;
    bool                        _active;
#ifdef CLICK_LINUXMODULE
    bool                        _cpu : 1;
#endif

}; // class DirectionalMovement

CLICK_ENDDECLS
#endif
