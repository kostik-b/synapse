#ifndef CLICK_TRUE_RANGE_HH
#define CLICK_TRUE_RANGE_HH

#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>
#include <click/array_wrapper.hh>

#include <click/global_sizes.hh>

CLICK_DECLS

struct HighLowClose
{
  HighLowClose ()
    : _current_high (0)
    , _current_low  (0)
    , _prev_close   (0)
    , _current_close(0)
    , _current_interval_num (-1)
    , _prev_interval_num (-1)
  {}

  fixedpt  _current_high;
  fixedpt  _current_low;
  fixedpt  _prev_close;
  fixedpt  _current_close;
  int64_t  _current_interval_num;
  int64_t  _prev_interval_num;
}; // struct HighLowClose

class TrueRange : public Element
{
  public:
    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, HighLowClose>               HighLowCloseMap;

    TrueRange();
    ~TrueRange();

    const char *class_name() const		{ return "TrueRange"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push(int port, Packet *p);

  private:
    const fixedpt calculate_true_range(const fixedpt  high,
                                       const fixedpt  low,
                                       const fixedpt  previous_close);

  private:
    HighLowCloseMap             _high_low_close_map;
    int64_t                     _interval_len;
    bool                        _debug;
    bool                        _active;
#ifdef CLICK_LINUXMODULE
    bool                        _cpu : 1;
#endif

}; // class TrueRange

CLICK_ENDDECLS
#endif
