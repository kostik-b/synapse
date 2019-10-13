// Copyright Queen's University of Belfast 2012

#ifndef CLICK_SynapseElementHH
#define CLICK_SynapseElementHH

namespace Synapse
{

enum PacketPort
{
  NO_PORT,
  TR_PORT,
  DM_PORT,
  PDI_PORT,
  MDI_PORT
}; // enum PacketPort

struct SavedPacket
{
  SavedPacket ()
    : _packet (NULL)
    , _port   (Synapse::NO_PORT)
  {
  }

  Packet*              _packet;
  Synapse::PacketPort  _port;
}; // struct SavedPacket

class SynapseElement
{
public:

  template <typename MapType>
  static typename MapType::mapped_type*
  get_or_create (
    MapType&                        map,
    const char(&                    symbol)[Synapse::ORDER_SYMBOL_LEN],
    typename MapType::mapped_type&  default_value)
  {
    bool created = false;

    return get_or_create<MapType> (map, created, symbol, default_value);
  }

  template <typename MapType>
  static typename MapType::mapped_type*
  get_or_create (
    MapType&      map,
    const char(&  symbol)[Synapse::ORDER_SYMBOL_LEN])
  {
    bool created = false;
    typename MapType::mapped_type default_value;

    return get_or_create<MapType> (map, created, symbol, default_value);
  }

  template <typename MapType>
  static typename MapType::mapped_type*
  get_or_create (
    MapType&      map,
    bool&         entry_created,
    const char(&  symbol)[Synapse::ORDER_SYMBOL_LEN])
  {
    typename MapType::mapped_type default_value;
    return get_or_create<MapType> (map, entry_created, symbol, default_value);
  }

  template <typename MapType>
  static typename MapType::mapped_type*
  get_or_create (
    MapType&                        map,
    bool&                           entry_created,
    const char(&                    symbol)[Synapse::ORDER_SYMBOL_LEN],
    typename MapType::mapped_type&  default_value)
  {
    typename MapType::Pair* pair = map.find_pair (symbol);

    if (!pair)
    {
      bool inserted = map.insert (symbol, default_value);

      if (!inserted)
      {
        return NULL;
      }

      pair          = map.find_pair (symbol);
      entry_created = true;
    }
    else
    {
      entry_created = false;
    }

    return &(pair->value);
  }


  static void discard_packet (Packet& packet)
  {
    packet.kill ();
  }

  static bool
  populate_msg_indicator (
    Packet&         packet,
    const char(&    symbol)[Synapse::ORDER_SYMBOL_LEN],
    const fixedpt&  indicator,
    const uint64_t& timestamp)
  {
    if (sizeof(MsgIndicator) > packet.length ())
    {
      // TODO: resize it in the future
      click_chatter ("PopulateMsgIndicator - could not squeeze MsgIndicator into a Packet");
      return false;
    }

    WritablePacket* write_packet = packet.put (0);
    MsgIndicator*   msg_indicator= reinterpret_cast<MsgIndicator*>(write_packet->data ());

    // make sure that we are reading and writing from different
    // places
    assert (((char*)symbol < (char*)msg_indicator) ||
              ((char*)symbol > (char*)msg_indicator + sizeof (*msg_indicator)));
    assert (((char*)&indicator < (char*)msg_indicator) ||
              ((char*)&indicator > (char*)msg_indicator + sizeof (*msg_indicator)));

    memcpy (msg_indicator->_symbol, symbol, Synapse::ORDER_SYMBOL_LEN);

    msg_indicator->_indicator = indicator;

    msg_indicator->_timestamp = timestamp;

    write_packet->set_packet_app_type (Synapse::MSG_INDICATOR);

    return true;
  }


}; // class SynapseElement

#ifdef __x86_64__
__inline uint64_t gcc_rdtsc (void)
{
  uint64_t msr;

  asm volatile ( "rdtsc\n\t"    // Returns the time in EDX:EAX.
          "shl $32, %%rdx\n\t"  // Shift the upper bits left.
          "or %%rdx, %0"        // 'Or' in the lower bits.
          : "=a" (msr)
          :
          : "rdx");

  return msr;
}
#else
static __inline__ uint64_t bmk_rdtsc( void )
{
  uint64_t x;
  __asm__ volatile("rdtsc\n\t" : "=A" (x));
  return x;
}
#endif

} // namespace Synapse

#endif
