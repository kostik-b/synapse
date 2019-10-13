// Copyright QUB 2017

#include <ev.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ext/hash_map>
#include <errno.h>
#include <unistd.h>
#include "trix.h"
#include "dmi.h"
#include "vortex.h"
#include "fixedpt_cpp.h"
#include "msg_trade.h"
#include "msg_parsing.h"
#include "running_stat.hh"
#include "array_wrapper.hh"
#include "indicator_manager.h"

typedef ArrayWrapper<char, ORDER_SYMBOL_LEN>  SymbolArrayWrapper;
typedef __gnu_cxx::hash_map<SymbolArrayWrapper, std::pair<TimeStats,IndicatorManager*> >  SymbolsMap;

SymbolsMap    gSymbols;
int           gSocket = 0;
bool          g_debug = false;

ev_timer      gTimerWatcher;

const size_t  BUFLEN  = 512;

static void parse_symbol_list (
  const char*               in_str,
  std::vector<std::string>& symbols)
{
  char* str = strdup (in_str);

  char* t = strtok (str, ",");
  while (t != NULL)
  {
    symbols.push_back (t);
    t = strtok (NULL, ",");
  }

  free (str);
}

static void parse_cmds (int argc, char** argv, Params& p)
{
  if (argc < 2)
  {
    printf ("Not enough options, exiting\n");
    exit (1);
  }

  extern char*  optarg;
  extern int    optind;
  extern int    opterr;

  opterr = 0; // don't print the error param to the stdout
  optind = 1; // reset getopt

  int c = '\0';
  // now read in all the parameters on the command line that override
  // the config file
  // -t - TRIX
  // -d - DMI
  // -v - Vortex
  // -s "..." - a string of comma separated symbols 
  // -n ... - num of periods (for ewma)
  // -i ... - interval len in seconds
  // -p ... - port to listen on
  // -e debug

  printf ("Trying to parse params\n");

  while ((c = getopt(argc, argv, "tdvs:n:i:p:ewa")) != -1)
  {
    switch (c) {
      case 't':
        p._is_trix = true; 
        break;
      case 'd':
        p._is_dmi = true; 
        break;
      case 'v':
        p._is_vortex = true; 
        break;
      case 'a':
        p._is_adline = true;
        break;
      case 's':
        parse_symbol_list (optarg, p._symbols);
        break;
      case 'n':
        p._ewma_periods = atoi (optarg);
        break;
      case 'i':
        p._interval_len_secs = atoi (optarg);
        break;
      case 'p':
        p._port = atoi (optarg);
        break;
      case 'e':
        g_debug = true;
        break;
      case 'w':
        p._indicator_debug = true;
        break;
      case '?':
        printf ("Unrecognized option, exiting\n");
        exit (0);
        break;
      default:
        break;
    }
  }
}

static void setup_conn (const Params& pars)
{
  struct sockaddr_in  si_me;

  if ((gSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
  {
    printf ("could not create socket\n");
    exit(1);
  }

  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(pars._port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(gSocket, (sockaddr*)&si_me, sizeof(si_me))==-1)
  {
    printf ("could not bind socket, error is %s\n", strerror (errno));
    exit(1);
  }
}

static bool update_hloc (
  const MsgTrade& msg_trade,
  TimeStats&      time_stats)
{
  // 1. if first update in an interval
  //    set the close, high, low to new trade's price
  // 2. if not first update, update as usual

  bool rc = false; // false = not updated. true = updated.

  if (!(time_stats._first_time_updated))
  {
    time_stats._high  = msg_trade._price;
    time_stats._low   = msg_trade._price;
    time_stats._close = msg_trade._price;
    time_stats._size += msg_trade._size;

    time_stats._first_time_updated  = true;
    rc                              = true;

    return true;
  }

  if (msg_trade._price > time_stats._high)
  {
    time_stats._high = msg_trade._price;
    rc               = true;
  }

  if (msg_trade._price < time_stats._low)
  {
    time_stats._low  = msg_trade._price;
    rc               = true;
  }

  time_stats._size += msg_trade._size;

  if (msg_trade._price != time_stats._close)
  {
    time_stats._close = msg_trade._price;
    rc                = true;
  }

  if (g_debug)
  {
    char high_str [25];
    char low_str  [25];
    char close_str[25];
    char open_str [25];

    fixedpt_str (time_stats._high, high_str, 4);
    fixedpt_str (time_stats._low, low_str, 4);
    fixedpt_str (time_stats._close, close_str, 4);
    fixedpt_str (time_stats._open, open_str, 4);

    printf ("TP: attempted to update time stats:\n");
    printf ("\thigh - %s, low - %s, close - %s, open - %s, size - %ld\n",
                    high_str, low_str, close_str, open_str, time_stats._size);

  }
  return rc;
}

extern "C"
{

void on_timer_cb (
  struct ev_loop*   loop,
  struct ev_timer*  watcher,
  int               revents)
{
  // the map is "key --> <timestats, indicator_manager*>"
  // for every symbol
  SymbolsMap::iterator iter = gSymbols.begin ();
  while (iter != gSymbols.end ())
  {
    TimeStats&        time_stats  = iter->second.first;
    IndicatorManager* ind_mgr     = iter->second.second;

    FixedPt close_price   = FixedPt::fromC(time_stats._close);
    FixedPt high_price    = FixedPt::fromC(time_stats._high);
    FixedPt low_price     = FixedPt::fromC(time_stats._low);

    // that's indicator manager
    ind_mgr->emit_add (high_price, low_price, FixedPt::fromInt (0)/*open*/, close_price);

    time_stats.rollover ();

    ++iter;
  }
}

static void udp_read_cb (
  struct ev_loop* loop,
  struct ev_io*   watcher,
  int             revents)
{

  char buffer [BUFLEN];
  struct sockaddr    clientAddress;
  socklen_t          clientAddrLen = sizeof (clientAddress);

  int bytesReceived = recvfrom(gSocket,
                               buffer,
                               BUFLEN,
                               0, // flags
                              (struct sockaddr *) &(clientAddress),
                              &clientAddrLen);

  if (bytesReceived < 0)
  {
    if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
    {
      fprintf (stderr, "EWOULDBLOCK: no UDP message\n");
      return;
    }
    else
    {
      fprintf (stderr, "Connection error, errno is %s\n", strerror (errno));
      return;
    }
  }

  if (bytesReceived > BUFLEN)
  {
    fprintf (stderr, "UDP packet size is greater than the buffer, continuing as is\n");
  }

  if (g_debug)
  {
    printf ("Read %d bytes from the socket\n", bytesReceived);
  }

  // parse into MsgTrade - borrow from trade processor
  MsgTrade msg;
  if (!populate_msg_trade (msg, buffer, bytesReceived))
  {
    printf ("Could not populate msg trade\n");
    return;
  }

  if (g_debug)
  {
    printf ("Populated msg trade. Symbol %s, price %s\n", msg._symbol, fixedpt_cstr (msg._price, 4));
  }
  
  // apply filtering - borrow from trade processor
  SymbolsMap::iterator iter = gSymbols.find (msg._symbol);
  if (iter == gSymbols.end ())
  {
    if (g_debug)
    {
      printf ("Symbol %s is not in our filter group\n", msg._symbol);
    }
    return;
  }

  // per symbol need to be initialized first
  // if our symbol, start the ev_timer, if not already started
  if (!ev_is_active (&gTimerWatcher))
  {
    printf ("Starting timer to generate ADDs\n");
    ev_timer_start(EV_DEFAULT, &gTimerWatcher);
  }

  IndicatorManager* ind_mgr     = iter->second.second;
  TimeStats&        time_stats  = iter->second.first;

  // update the hloc - borrow from trade processor
  if (!update_hloc (msg, time_stats))
  {
    // no update = no recalculation
    if (g_debug)
    {
      printf ("Did not update HLOC - no change in values\n");
    }
    return;
  }

  // we process indicators here
  FixedPt close_price = FixedPt::fromC(time_stats._close);
  FixedPt high_price  = FixedPt::fromC(time_stats._high);
  FixedPt low_price   = FixedPt::fromC(time_stats._low);

  if (!(ind_mgr->initialized ()))
  {
    ind_mgr->initialize_indicators (high_price, low_price, FixedPt::fromInt (0), close_price);
    return;
  }
  else
  {
    ind_mgr->emit_update (high_price, low_price, FixedPt::fromInt (0), close_price, msg._src_timestamp);
  }
}

static void
sigint_cb (struct ev_loop *loop, ev_signal *w, int revents)
{
  ev_break (loop, EVBREAK_ALL);
}

} // extern "C"

int main (int argc, char** argv)
{
  // parse the parameters - take from hh
  Params pars;
  parse_cmds (argc, argv, pars);

  // create hash map -> populate with symbols from "-s"
  char symbol_buffer [ORDER_SYMBOL_LEN];
  for (int i = 0; i < pars._symbols.size(); ++i)
  {
    memset (symbol_buffer, '\0', ORDER_SYMBOL_LEN);
    strcpy (symbol_buffer, pars._symbols[i].c_str ());
    SymbolArrayWrapper wrapper (symbol_buffer);

    printf ("Trying to insert symbol %s into the symbol map\n", wrapper.array_ptr ());

    gSymbols.insert (std::make_pair (wrapper, std::make_pair(TimeStats (), new IndicatorManager (pars, symbol_buffer))));
  }
  // create connection
  setup_conn (pars);

  printf ("Opened socket to listen on port %d\n", pars._port);
  
  // create the indicator objects
  // only trix for now
  // Trix trix (13);

  // create default loop
  ev_default_loop (EVFLAG_AUTO);
  // setup ev_io - to read from the socket
  ev_io io_watcher;
  io_watcher.data = NULL;
  ev_io_init  (&io_watcher, udp_read_cb, gSocket, EV_READ);
  ev_io_start (EV_DEFAULT, &io_watcher);

  // setup sig handler to terminate application
  ev_signal signal_watcher;
  ev_signal_init (&signal_watcher, sigint_cb, SIGINT);
  ev_signal_start (EV_DEFAULT, &signal_watcher);

  // initialize, but not start the timer watcher
  gTimerWatcher.data = NULL;
  ev_timer_init (&gTimerWatcher, on_timer_cb,
      pars._interval_len_secs/*first firing*/,
      pars._interval_len_secs/*repeat after these secs*/);

  printf ("Starting default EV loop\n");

  // run ev loop
  ev_run (EV_DEFAULT, 0);

  // printf indicator stats
  printf ("Left the default loop\n");

  // iterate over all the symbols and
  // delete the indicator managers
  SymbolsMap::iterator iter = gSymbols.begin ();
  while (iter != gSymbols.end ())
  {
    IndicatorManager* mgr = iter->second.second;
    if (mgr)
    {
      printf ("Deleting symbol %s\n", iter->first.array_ptr ());
      delete mgr;
    }
    ++iter;
  }

  // close socket
  close (gSocket);

  return 0;
}
