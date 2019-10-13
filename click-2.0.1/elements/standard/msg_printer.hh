#ifndef CLICK_MSG_PRINTER_HH
#define CLICK_MSG_PRINTER_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>

CLICK_DECLS


class MsgPrinter : public Element
{
  public:
    typedef Synapse::ArrayWrapper<char, Synapse::ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
    typedef HashMap<SymbolArrayWrapper, FILE*>                      FileStructMap;

    MsgPrinter();
    ~MsgPrinter();

    const char *class_name() const		{ return "MsgPrinter"; }
    const char *port_count() const		{ return "1/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    Packet* simple_action (Packet* p);

  private:
    FILE* get_file_ptr       (bool&         first_open,
                              const char(&  symbol)[Synapse::ORDER_SYMBOL_LEN]);
  
    void write_msg_indicator (Packet&       p);

    void write_msg_stats     (Packet&       p);

    void write_msg_trade     (Packet&       p);
  private:

    String        _file_extension;
    FileStructMap _file_struct_map;

    bool          _active;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

}; // class MsgPrinter

CLICK_ENDDECLS
#endif
