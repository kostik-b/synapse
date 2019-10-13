#ifndef CLICK_DX_INDICATOR_HH
#define CLICK_DX_INDICATOR_HH
#include <click/element.hh>
#include <click/string.hh>

#include <click/hashmap.hh>
#include <click/array_wrapper.hh>
#include <click/global_sizes.hh>
#include "synapseelement.hh"

using Synapse::SavedPacket;

CLICK_DECLS

class DxIndicator : public Element
{
  public:
    DxIndicator();
    ~DxIndicator();

    const char *class_name() const		{ return "DxIndicator"; }
    const char *port_count() const		{ return "2/1"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    void push (int port, Packet* p);

  private:
    Synapse::PacketPort detect_packet_port (const int port);

  private:
    int           _pdi_port;
    int           _mdi_port;
    SavedPacket   _saved_packet;
    
    String        _name;
    bool          _debug;
    bool          _active;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

}; // class DxIndicator

CLICK_ENDDECLS
#endif
