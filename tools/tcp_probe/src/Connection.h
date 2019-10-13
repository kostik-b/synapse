// Copyright QUB 2013

#ifndef ConnectionH
#define ConnectionH

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdexcept>

#include "ProbeTool.h"

using ProbeTool::ReturnValue;

class Connection
{
public:
  Connection (
    const char* ip_address,
    const int   port,
    const int   socket_type,
    const int   protocol)
  {
    _socket_fd = socket(AF_INET, socket_type, protocol);

    if (_socket_fd < 0)
    {
      printf ("Connection: ERROR creating socket\n");
      throw std::runtime_error ("Error creating a socket");
    }

    _destination_address.sin_family       = AF_INET;
    _destination_address.sin_addr.s_addr  = inet_addr(ip_address);
    _destination_address.sin_port         = htons(port);

    if (connect(_socket_fd,(struct sockaddr *) &_destination_address, sizeof(_destination_address)) < 0)
    {
      printf ("Connection: ERROR connecting to destination socket\n");
      throw std::runtime_error ("Error creating a socket");
    }
  }

  virtual ~Connection ()
  {
    close (_socket_fd);
  }

  int send_msg (const char* str,
                const int   strlen)
  {
    // we don't check the return value at the moment
    return send(_socket_fd, str, strlen, 0);
  }

  int get_socket_fd ()
  {
    return _socket_fd;
  }

private:
  struct sockaddr_in _destination_address;
  int                _socket_fd;

}; // class Connection

#endif
