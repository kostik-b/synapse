// a single header file is required
#include <ev.h>

#include <stdio.h> // for puts
#include <stdlib.h>
#include <unistd.h>

#include "UdpConnection.h"
#include "UdpReceiveConnection.h"

using Synapse::UdpReceiveConnection;

// every watcher type has its own typedef'd struct
// with the name ev_TYPE
ev_io     socket_watcher;
ev_timer  timeout_watcher;

static int                    s_msg_counter           = 0;
static int                    s_reporting_interval    = 1; // default
static UdpReceiveConnection*  s_connection            = NULL;

// all watcher callbacks have a similar signature
// this callback is called when data is readable on stdin
static void socket_cb (EV_P_ ev_io *w, int revents)
{
  ++s_msg_counter;

  static const size_t RECV_BUF_LEN = 200;
  static char         buffer [RECV_BUF_LEN];
  if (!s_connection->receive_msg (buffer, RECV_BUF_LEN))
  {
    // this causes all nested ev_run's to stop iterating
    ev_break (EV_A_ EVBREAK_ALL);
  }
}

// another callback, this time for a time-out
static void timeout_cb (EV_P_ ev_timer *w, int revents)
{
  printf ("MsgCounter: %d\n", s_msg_counter);
  s_msg_counter = 0;
}

int main (int argc, char** argv)
{
  extern char*  optarg;
  extern int    optind;

  int           c           = 0;
  bool          wrong_arg   = false;

  int           port        = 0;

  while (((c = getopt(argc, argv, "r:p:")) != -1) && (!wrong_arg))
    switch (c) {
    case 'r':
      s_reporting_interval = atoi(optarg);
      break;
    case 'p':
      port = atoi (optarg);
      break;
    case '?':
      wrong_arg = true;
      break;
    }

  s_connection = new UdpReceiveConnection (port);

  if (wrong_arg)
  {
    printf ("UdpStats: wrong argument, exiting ...\n");
    exit (1);
  }

  if (s_reporting_interval < 1)
  {
    printf ("UdpStats: reporting interval cannot be less than 1, exiting ...\n");
    exit (1);
  }

  printf ("Reporting interval is %d seconds\n", s_reporting_interval);

  // block on udp connection until we receive the first
  // message
  
  static const size_t RECV_BUF_LEN = 200;
  static char         buffer [RECV_BUF_LEN];
  s_connection->receive_msg (buffer, RECV_BUF_LEN);

  // use the default event loop unless you have special needs
  struct ev_loop *loop = EV_DEFAULT;

  // initialise an io watcher, then start it
  // this one will watch for stdin to become readable
  ev_io_init (&socket_watcher, socket_cb, s_connection->get_socket_fd(), EV_READ);
  ev_io_start (loop, &socket_watcher);

  // initialise a timer watcher, then start it
  // simple non-repeating 5.5 second timeout
  ev_timer_init (&timeout_watcher, timeout_cb, s_reporting_interval, s_reporting_interval);
  ev_timer_start (loop, &timeout_watcher);

  // now wait for events to arrive
  ev_run (loop, 0);

  delete s_connection;

  // break was called, so exit
  return 0;
}
