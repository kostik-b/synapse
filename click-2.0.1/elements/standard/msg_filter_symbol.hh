#ifndef CLICK_MSG_FILTER_SYMBOL_HH
#define CLICK_MSG_FILTER_SYMBOL_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>

CLICK_DECLS

class MsgFilterSymbol : public Element
{
  public:

    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    // bool is really a dummy value - ideally we should be using a hash set here
    typedef HashMap<SymbolArrayWrapper, bool>                       SymbolMap;

    enum FilterMode
    {
      ALLOWING,
      RESTRICTIVE
    };

    MsgFilterSymbol();
    ~MsgFilterSymbol();

    const char *class_name() const		{ return "MsgFilterSymbol"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push (int port, Packet* p);

  private:
    void   discard_packet (Packet& packet);

  private:
    SymbolMap     _symbol_map;

    FilterMode    _filter_mode;
    bool          _active;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

}; // class MsgFilterSymbol

CLICK_ENDDECLS
#endif
