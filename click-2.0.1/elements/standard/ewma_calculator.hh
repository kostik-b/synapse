#ifndef CLICK_EWMA_CALCULATOR_HH
#define CLICK_EWMA_CALCULATOR_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>

CLICK_DECLS


struct EwmaCalculator
{
    EwmaCalculator ()
      : _prev_ewma_value      (0)
      , _current_ewma_value   (0)
      , _prev_interval_num    (0)
      , _current_interval_num (0)
    {}

    fixedpt         _prev_ewma_value;
    fixedpt         _current_ewma_value;
    int64_t         _prev_interval_num;
    int64_t         _current_interval_num;
}; // class EwmaCalculator

class EwmaElement : public Element
{
  public:

    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, EwmaCalculator>             EwmaMap;


    EwmaElement();
    ~EwmaElement();

    const char *class_name() const		{ return "EwmaElement"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push (int port, Packet* p);

  private:

    EwmaMap         _ewma_map;
    fixedpt         _ewma_alpha_value;
    int64_t         _interval_len;
    bool            _debug;
    bool            _active;
#ifdef CLICK_LINUXMODULE
    bool            _cpu : 1;
#endif

}; // class EwmaElement

CLICK_ENDDECLS
#endif
