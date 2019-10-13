// Copyright Queen's University of Belfast 2013

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <shmq/shmq_msg.h>
#include <pcap/pcap.h>
#include <sys/time.h>

static int      s_shutdown    = 0;
static pcap_t*  s_pcap_handle = NULL; 

static void sig_handler(int signum)
{
  printf("Received signal %d\n", signum);

  s_shutdown = 1;
}

static pcap_dumper_t* open_file (const char* msg_file_name)
{
  s_pcap_handle                 = pcap_open_dead (DLT_EN10MB, 256);

  pcap_dumper_t*  dumper_handle = pcap_dump_open (s_pcap_handle, msg_file_name);

  return dumper_handle;
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
  if (argc != 3)
  {
    printf ("Need 2 arguments, exiting!\n");
    exit (1);
  }

  const char* file_name = argv[1];
  const int   key       = atoi (argv[2]);

  // open the shared memory queue
  int msq_id = open_msg_queue (key);
  if (msq_id < 0)
  {
    printf ("Could not connect to the queue\n");
    exit (1);
  }
  // open the file to write to
  pcap_dumper_t* write_file = open_file (file_name);
  if (write_file == NULL)
  {
    printf ("Could not open file\n");
    exit (1);
  }

  // for unknown reason if this is not set
  // nothing is getting written to a file
  // if msgrcv is removed then everything is
  // working
  setbuf (write_file, NULL);

  struct shmq_msg shmq_msg;

  // in a loop
  while (!s_shutdown)
  {
    /// recv msg
    int result = msgrcv( msq_id, (struct msgbuf *)&shmq_msg, sizeof (shmq_msg._msg_data_struct), 0, IPC_NOWAIT|MSG_NOERROR);
    printf ("result is %d\n", result);
    if (result > 0)
    {
      // 1. populate the packet header
      struct timeval timestamp;
      gettimeofday (&timestamp, NULL);

      struct pcap_pkthdr packet_header = {
        .ts     = timestamp,      /* time stamp */
        .caplen = shmq_msg._msg_data_struct._size,     /* length of portion present */
        .len    = shmq_msg._msg_data_struct._size        /* length this packet (off wire) */
      };

      // 2. dump the packet to a file
      pcap_dump (write_file, &packet_header, shmq_msg._msg_data_struct._data);
    }
    else
    {
      sleep (1);
    }
  }


  pcap_dump_close (write_file);
  pcap_close      (s_pcap_handle);

  return 0; // success
}
