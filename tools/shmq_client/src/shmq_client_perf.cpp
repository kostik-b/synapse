// Copyright Queen's University of Belfast 2013

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <shmq/shmq_msg.h>
#include <sys/time.h>
#include <inttypes.h>
#include <string.h>

#include <click/appmsgs.hh>

#include "cycles_counter.hh"

using Synapse::MsgTrade;
using Synapse::CyclesCounter;

static int      s_shutdown    = 0;

static CyclesCounter s_counter;

static void sig_handler(int signum)
{
  printf("Received signal %d\n", signum);

  s_shutdown = 1;
}

__inline__ uint64_t bmk_rdtsc (void)
{
  uint64_t x;
  __asm__ volatile("rdtsc\n\t" : "=A" (x));
  return x;
}

void print_time_stamp (struct shmq_msg* shmq_msg)
{
  const MsgTrade* msg_trade = reinterpret_cast<MsgTrade*>(shmq_msg->_msg_data_struct._data);

  const uint64_t cycles_taken = bmk_rdtsc () - msg_trade->_timestamp;

  printf ("MsgTrade - cycles taken is %llu\n", cycles_taken);

  static int timestamps_reported = 0;

  timestamps_reported++;

  s_counter.update_counter (cycles_taken);
  s_counter.reset_counter ();

  if (timestamps_reported % 100 == 0)
  {
    const size_t buf_len = 100*15;
    char buffer [buf_len];

    s_counter.get_x_last_counters_str (buffer, buf_len, 100);

    printf ("The last 100 counters are %s\n", buffer);
  }
}

static int open_msg_queue (const int key)
{
  int msqid;
  msqid = msgget( key, 0666 );
  if( msqid == -1 )
  {
    perror( "msgget" );
    return -1;
  }
  return msqid;
}


int main (int argc, char** argv)
{
  if (argc != 2)
  {
    printf ("Need 1 argument, exiting!\n");
    exit (1);
  }

  const int   key       = atoi (argv[1]);

  // open the shared memory queue
  int msq_id = open_msg_queue (key);
  if (msq_id < 0)
  {
    printf ("Could not connect to the queue\n");
    exit (1);
  }

  struct shmq_msg shmq_msg;
  int    failure_counter = 0;

  // in a loop
  while (!s_shutdown)
  {
    /// recv msg
    int result = msgrcv( msq_id, (struct msgbuf *)&shmq_msg, sizeof (shmq_msg._msg_data_struct), 0, MSG_NOERROR);
    printf ("result is %d\n", result);
    if (result > 0)
    {
      print_time_stamp (&shmq_msg);
    }
    else
    {
      printf ("Couldn't fetch the msg from the message queue, ret value is %d\n", result);
      ++failure_counter;
      if (failure_counter > 10)
      {
        break;
      }
    }
  }

  return 0; // success
}
