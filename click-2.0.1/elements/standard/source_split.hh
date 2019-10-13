#ifndef CLICK_SOURCE_SPLIT_HH
#define CLICK_SOURCE_SPLIT_HH


#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>
#include <click/array_wrapper.hh>

#include <click/global_sizes.hh>

CLICK_DECLS

class SourceSplit : public Element
{
  public:
    SourceSplit();
    ~SourceSplit();

    const char *class_name() const		{ return "SourceSplit"; }
    const char *port_count() const		{ return "1/-"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push(int port, Packet *p);

private:
    void sendMsg (Packet&         packet,
                  const int       outPort,
                  const fixedpt&  value,
                  const uint64_t  timestamp,
                  Synapse::PacketAppType msg_type);

private:
    bool                        _debug;
    int                         _close_port;
    int                         _high_port;
    int                         _low_port;
    int                         _open_port;
    int                         _volume_port;
    
    bool                        _active;
#ifdef CLICK_LINUXMODULE
    bool                        _cpu : 1;
#endif

}; // class SourceSplit

CLICK_ENDDECLS
#endif
