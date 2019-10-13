#ifndef CLICK_CHIX_TRADE_HANDLER_HH
#define CLICK_CHIX_TRADE_HANDLER_HH
#include <click/element.hh>
#include <click/string.hh>
#include <click/hashmap.hh>
#include <click/global_sizes.hh>
#include <click/cycles_counter.hh>

#include "chix_trade_handler_utils.hh"
#include "synapse_macros.h"

CLICK_DECLS

using Synapse::ORDER_SYMBOL_LEN;

class FpuGuard
{
public:
  FpuGuard ()
  {
    FPU_START();
  }

  ~FpuGuard ()
  {
    FPU_END ();
  }
}; // class FpuGuard

struct OrderInfo
{
  OrderInfo ()
    : _price (0.0f)
    , _shares (0)
  {
    memset (_symbol, '\0', ORDER_SYMBOL_LEN);
  }

  OrderInfo (const fixedpt price,
             const int     shares,
             const char(&  symbol)[ORDER_SYMBOL_LEN])
    : _price (price)
    , _shares(shares)
  {
    memcpy (_symbol, symbol, ORDER_SYMBOL_LEN);
  }

  fixedpt _price;
  int64_t _shares;
  char    _symbol[ORDER_SYMBOL_LEN];
};


class ChixTradeHandler : public Element
{
  public:
    typedef HashMap<int, OrderInfo>         OrderMap;

    enum OrderMsgType
    {
      SHORT,
      LONG
    };

    ChixTradeHandler();
    ~ChixTradeHandler();

    const char *class_name() const		{ return "ChixTradeHandler"; }
    const char *port_count() const		{ return "1/-"; }

    int configure(Vector<String> &, ErrorHandler *);
    bool can_live_reconfigure() const		{ return false; }
    void add_handlers();

    // overriding from base class
    void push(int port, Packet *p);

  private:

    void send_msg_trade     (Packet&              packet,
                             const char(&         symbol)[ORDER_SYMBOL_LEN],
                             const fixedpt        price,
                             const int64_t        shares,
                             const uint64_t       timestamp);

    void discard_packet     (Packet&              packet);

    void parse_add_order    (const unsigned char* md_msg,
                             const OrderMsgType   order_msg_type);

    void parse_cancel_order (const unsigned char* md_msg,
                             const OrderMsgType   order_msg_type);

    void parse_execute_order(bool&                is_packet_reused,
                             Packet&              packet,
                             const unsigned char* md_msg,
                             const OrderMsgType   order_msg_type);

    template <const int SymbolOffset,
              const int SymbolLen,
              const int SharesOffset,
              const int SharesLen,
              const int RefNumOffset,
              const int RefNumLen,
              const int PriceOffset,
              const int PriceLen,
              const int PriceWholeLen>
    void do_parse_add_order    (const unsigned char* md_msg);

    template <const int RefNumOffset,
              const int RefNumLen,
              const int CancelSharesOffset,
              const int CancelSharesLen>
    void do_parse_cancel_order (const unsigned char* md_msg);

    template <const int RefNumOffset,
              const int RefNumLen,
              const int ExecutedSharesOffset,
              const int ExecutedSharesLen>
    void do_parse_execute_order (bool&                is_packet_reused,
                                 Packet&              packet,
                                 const unsigned char* md_msg);

    void report_cycles          (const unsigned char* md_msg);

  private:
    OrderMap      _orderMap;

    bool          _debug;
    bool          _active;
#ifdef CLICK_LINUXMODULE
    bool          _cpu : 1;
#endif

    const size_t  _mac_ip_udp_len;

    Synapse::CyclesCounter _cycles_counter;
}; // class ChixTradeHandler

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/****************************************** TEMPLATE FUNCTIONS GO BELOW************************/
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

template <const int SymbolOffset,
          const int SymbolLen,
          const int SharesOffset,
          const int SharesLen,
          const int RefNumOffset,
          const int RefNumLen,
          const int PriceOffset,
          const int PriceLen,
          const int PriceWholeLen>
void ChixTradeHandler::do_parse_add_order    (
  const unsigned char* md_msg)
{
  char symbol [ORDER_SYMBOL_LEN];
  memset (symbol, '\0', ORDER_SYMBOL_LEN);
  memcpy (symbol, md_msg + SymbolOffset, SymbolLen);

  ChixTradeHandlerUtils::trim_alphanumeric_spaces<ORDER_SYMBOL_LEN> (symbol);

  // need to parse price
  const fixedpt price  = ChixTradeHandlerUtils::parse_add_order_price_as_fixedpt<PriceOffset, PriceLen, PriceWholeLen> (md_msg);
  // need to parse shares
  const long    shares = ChixTradeHandlerUtils::parse_numeric<const long, SharesOffset, SharesLen> (md_msg);

  // need a ref number - represent it as string
  const int    ref_num= ChixTradeHandlerUtils::parse_numeric<const int, RefNumOffset, RefNumLen>  (md_msg);

  // click_chatter ("price is %f, shares are %d, ref num is %d", price, shares, ref_num);

  _orderMap.insert (ref_num, OrderInfo (price, shares, symbol));
}

template <const int RefNumOffset,
          const int RefNumLen,
          const int CancelSharesOffset,
          const int CancelSharesLen>
void ChixTradeHandler::do_parse_cancel_order (
  const unsigned char*  md_msg)
{
  const int ref_num = ChixTradeHandlerUtils::parse_numeric<const int, RefNumOffset, RefNumLen> (md_msg);

  OrderMap::Pair* pair =_orderMap.find_pair (ref_num);

  if (pair)
  {
    const int64_t shares_to_cancel = ChixTradeHandlerUtils::parse_numeric<const int64_t, CancelSharesOffset, CancelSharesLen>(md_msg);

    if ((pair->value._shares - shares_to_cancel) < 1)
    {
      _orderMap.remove (ref_num);
    }
    else
    {
      pair->value._shares = pair->value._shares - shares_to_cancel;
    }
  }
}

template <const int RefNumOffset,
          const int RefNumLen,
          const int ExecutedSharesOffset,
          const int ExecutedSharesLen>
void ChixTradeHandler::do_parse_execute_order (
  bool&                is_packet_reused,
  Packet&              packet,
  const unsigned char* md_msg)
{
  const int ref_num = ChixTradeHandlerUtils::parse_numeric<const int, RefNumOffset, RefNumLen> (md_msg);

  OrderMap::Pair* pair =_orderMap.find_pair (ref_num);

  if (pair)
  {
    const int       executed_shares = ChixTradeHandlerUtils::parse_numeric<const int, ExecutedSharesOffset, ExecutedSharesLen> (md_msg);
    char timestampBuffer[9];
    memcpy (timestampBuffer, md_msg, 8);
    timestampBuffer[8] = '\0';
    // enable for the time produced by the timestamper kernel module
//    const uint64_t  timestamp       = *(reinterpret_cast<const uint64_t*>(md_msg));
    const uint64_t timestamp = atoi (timestampBuffer);

    send_msg_trade (packet, pair->value._symbol, pair->value._price, executed_shares, timestamp);
    is_packet_reused = true;

    if ((pair->value._shares - executed_shares) < 1)
    {
      _orderMap.remove (ref_num);
    }
    else
    {
      pair->value._shares = pair->value._shares - executed_shares;
    }
  }
}

CLICK_ENDDECLS
#endif
