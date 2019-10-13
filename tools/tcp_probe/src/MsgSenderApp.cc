
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "ProbeTool.h"
#include "TcpConnection.h"
#include "UdpConnection.h"
#include "PbFileReader.h"

#define E_9 1000000000

using ProbeTool::ReturnValue;

static bool         sShutDown         = false;

static void sig_handler(int signum)
{
  printf("Received signal %d\n", signum);

  sShutDown = true;
}

inline static timespec advance_time_by_period (
  timespec  time,
  uint64_t  period)
{
  printf ("KB: advancing time by period %lld\n", period);
  uint64_t next_time_nsecs = uint64_t(time.tv_sec) * uint64_t(1e9) + uint64_t(time.tv_nsec) + uint64_t(period);

  time.tv_sec   = next_time_nsecs / E_9;
  time.tv_nsec  = next_time_nsecs % E_9;

  return time;
}

static bool
timeout_cb (
    Connection*     connection,
    PbFileReader&   file_reader)
{
  timespec now_time;
  clock_gettime(CLOCK_MONOTONIC, &now_time);
  printf ("KB: sending msg tv.sec is %ld, tv.usec is %ld\n", now_time.tv_sec, now_time.tv_nsec);
  
  char*   line    = NULL;
  ssize_t linelen = 0;

  bool status = file_reader.read_line (line, linelen);

  if ((line == NULL) || (linelen == 0))
  {
    return false;
  }

  // even if status == false, we'll still send it as it is the
  // last line
  const int bytes_written = connection->send_msg (line, linelen);
  if (bytes_written < 0)
  {
    if (errno != 111)
    {
      return false;
    }
    else
    {
      // printf ("Send File From file, received error: %d:%s\n", errno, strerror(errno));
    }
  }

  return status;
}


int64_t compare_timespec (
  timespec first_time,
  timespec second_time) 
{
  int64_t first_time_nsecs  = int64_t(first_time.tv_sec)  * int64_t(E_9) + int64_t(first_time.tv_nsec);
  int64_t second_time_nsecs = int64_t(second_time.tv_sec) * int64_t(E_9) + int64_t(second_time.tv_nsec);

  return second_time_nsecs - first_time_nsecs;
}

int main (int argc, char** argv)
{
  // handle the graceful exit - not yet
  // signal (SIGINT, sig_handler);

  // handle configuration params
  enum Transport
  {
    None,
    TcpTport,
    UdpTport
  };

  extern char*  optarg;
  extern int    optind;

  char*         ipAddress   = NULL;
  int           port        = 0;
  char*         msgFileName = NULL;
  int           msg_rate    = 1;
  Transport     tport       = None;
  PbMode        pb_mode     = KB;
  
  int           c           = 0; 
  bool          wrong_arg   = false;

  while (((c = getopt(argc, argv, "r:f:p:i:t:a:")) != -1) && (!wrong_arg))
    switch (c) {
    case 'r':
      msg_rate = atoi(optarg);
      break;
    case 't':
      if ((optarg[0] == 'u') || (optarg[0] == 'U'))
      {
        tport = UdpTport;
      }
      else if ((optarg[0] == 't') || (optarg[0] == 'T'))
      {
        tport = TcpTport;
      }
      else
      {
        wrong_arg = true;
      }
      break;
    case 'p':
      port = atoi (optarg);
      break;
    case 'f':
      msgFileName = optarg;
      break;
    case 'i':
      ipAddress = optarg;
      break;
    case 'a':
      if (optarg[0] == 'K')
      {
        pb_mode = KB;
      }
      else if (optarg[0] == 'C')
      {
        pb_mode = CHIX;
      }
      break;
    case '?':
      wrong_arg = true;
      break;
    }

  if (wrong_arg)
  {
    printf ("MsgSenderApp: some wrong argument, exiting ...\n");
    exit(1);
  }

  printf ("Port number is %d, file name is %s, msg_rate multiplier is %d, mode is %d\n", port, msgFileName, msg_rate, pb_mode);

  PbFileReader file_reader (msgFileName, false, pb_mode);

  Connection* connection = NULL;
  if (tport == TcpTport)
  {
    connection = new TcpConnection (ipAddress, port);
  }
  else if (tport == UdpTport)
  {
    connection = new UdpConnection (ipAddress, port);
  }
  else
  {
    printf ("Wrong transport specified, exiting\n");
    exit (1);
  }

  if (msg_rate < 1)
  {
    printf ("MsgSenderApp: msgrate cannot be 0 or less! Exiting\n");
    exit (1);
  }

  // 1. get time now
  // 3. set var to time now - 1
  // 4. loop
  // 5. -- get time now
  // 6. -- does time now > var?
  // 7. ---Y: send msg and set var to new interval - breakable
  // 8. ---N: continue

  timespec now_time;
  clock_gettime(CLOCK_MONOTONIC, &now_time);

  // make sure that trigger time is less than now_time
  timespec trigger_time = now_time;
  
  while(1)
  {
    // call gettime
    clock_gettime(CLOCK_MONOTONIC, &now_time);

    if (compare_timespec (trigger_time, now_time) > 0)
    {
      if (!timeout_cb (connection, file_reader))
      {
        break;
      }

      trigger_time = advance_time_by_period (trigger_time, (uint64_t(file_reader.get_interval ())*uint64_t(1e3))/uint64_t(msg_rate));
      file_reader.advance ();
    }
   
    // clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
    //     &time, NULL);
  }

  delete connection;

  return 0;
}

