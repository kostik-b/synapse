
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

static const size_t NUM_SIMULTANEOUS_INCOMING_CONNECTIONS = 1;
static const size_t READ_BUFFER_LEN                       = 256;

static int createServerSocket (const int port)
{
  struct sockaddr_in  server_addr;
  int                 serverSocketFD = -1;

  serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocketFD < 0)
  {
    printf("ERROR opening socket\n");
    return -1;
  }

  int reuseAddr = 1;
  int optionSet = setsockopt (serverSocketFD, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

  if (optionSet < 0)
  {
    printf ("couldn't sent socket option\n");
  }

  memset((void*)&server_addr, 0, sizeof (server_addr));

  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port        = htons(port);

  // 2. Bind the socket to an address using the bind() system call. For a server socket on the Internet, an address consists of a port number on the host machine.
  if (bind(serverSocketFD, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
  {
    printf("ERROR on binding\n");
    return -1;
  }

  // 3. Mark the socket as listening for connections with the listen() system call
  listen(serverSocketFD, NUM_SIMULTANEOUS_INCOMING_CONNECTIONS);

  return serverSocketFD;

}

static int acceptOneConnection (const int serverSocketFD)
{
  struct sockaddr_in  client_addr;
  size_t              clientAddrLen   = sizeof(client_addr);
  int                 clientSocketFD  = -1;

  clientSocketFD = accept(serverSocketFD, (struct sockaddr *) &client_addr, &clientAddrLen);

  if (clientSocketFD < 0)
  {
    printf("ERROR on accept, message is %s\n", strerror(errno));
    return -1;
  }

  return clientSocketFD;
}

static void logConnectionMsgs (const int clientSocketFD)
{
  char buffer[READ_BUFFER_LEN];
  memset (buffer, 0, READ_BUFFER_LEN);

  int bytesRead = 0;

  do
  {
    bytesRead = read (clientSocketFD, buffer, READ_BUFFER_LEN - 1);

    if (bytesRead < 0)
    {
      printf("ERROR reading from socket\n");
    }
    else if (bytesRead == 0)
    {
      printf ("Enf of file encountered\n");
    }
    else
    {
      printf("Here is the message: %s\n", buffer);
    }
  } while (bytesRead > 0);
}

enum RunMode
{
  ONCE,
  FOREVER
};

int main (int argc, char** argv)
{
  if (argc != 3)
  {
    printf ("Error: you must specify port number and run mode\n");
    exit (1);
  }

  // port number
  int port = atoi (argv[1]);
  if (port < 0)
  {
    printf ("ERROR - incorrect port\n");
    exit (1);
  }

  // run mode - forever or not
  RunMode runMode = ONCE; // default is once
  if (strcmp (argv[2], "once") == 0)
  {
    runMode = ONCE;
  }
  else if (strcmp (argv[2], "forever") == 0)
  {
    runMode = FOREVER;
  }

  int serverSocketFD = createServerSocket (port);

  if (serverSocketFD < 0)
  {
    printf ("Couldn't create server socket, exiting ...\n");
    exit (1);
  }

  int acceptedSocketFD = acceptOneConnection (serverSocketFD);

  if (runMode == ONCE)
  {
    // we don't want any more connections for now
    close (serverSocketFD);
    logConnectionMsgs (acceptedSocketFD);
    close (acceptedSocketFD);
  }
  else if (runMode == FOREVER)
  {
    while (true)
    {
      close             (serverSocketFD);
      logConnectionMsgs (acceptedSocketFD);
      close             (acceptedSocketFD);

      serverSocketFD    = createServerSocket (port);
      if (serverSocketFD < 0)
      {
        exit (1);
      }
      acceptedSocketFD  = acceptOneConnection (serverSocketFD);
      if (acceptedSocketFD < 0)
      {
        exit (1);
      }
    }
  }

  return 0;
}
