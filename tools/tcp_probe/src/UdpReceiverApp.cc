#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/time.h>
#include <string.h>

#define BUFLEN 512
#define NPACK 10

void diep(char *s)
{
  perror(s);
  exit(1);
}

int main(int argc, char** argv)
{
  struct sockaddr     si_other;
  struct sockaddr_in  si_me;
  int s, i;
  socklen_t slen=sizeof(si_other);
  char buf[BUFLEN];

  int      port      = atoi(argv[1]);
  unsigned msg_count = atoi(argv[2]);

  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    diep("socket");

  memset((char *) &si_me, 0, sizeof(si_me));
  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(port);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s, (sockaddr*)&si_me, sizeof(si_me))==-1)
    diep("bind");

  uint64_t msgs_counter  = 0;
  timeval  time_first;
  timeval  time_second;

  while (true)
  {
    if (recvfrom(s, buf, BUFLEN, 0, &si_other, &slen)==-1)
      diep("recvfrom()");

    msgs_counter++;

    if (msgs_counter % msg_count == 0)
    {
      gettimeofday (&time_second, NULL);

      uint64_t micsecsofday_first  = uint64_t(time_first.tv_sec) * uint64_t(1000000) + time_first.tv_usec;
      uint64_t micsecsofday_second = uint64_t(time_second.tv_sec) * uint64_t(1000000) + time_second.tv_usec;

      uint64_t time_taken = micsecsofday_second - micsecsofday_first;
      printf ("Time taken to process %d messages is %llu seconds and %llu msecs\n", msg_count, time_taken / 1000000, time_taken % 1000000);

      time_first = time_second;
    }
  }

  close(s);
  return 0;
}
