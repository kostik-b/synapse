// 2014 Queen's University Belfast

#ifndef SynapseUdpReceiveConnectionH
#define SynapseUdpReceiveConnectionH

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/time.h>
#include <string.h>


namespace Synapse
{

class UdpReceiveConnection
{
public:
  UdpReceiveConnection (const int port)
  {
    // set up the socket for listening
    if ((_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
      throw std::runtime_error ("UdpReceiveConnection: could not create socket");
    }

    memset((char *) &_my_address, 0, sizeof(_my_address));
    _my_address.sin_family      = AF_INET;
    _my_address.sin_port        = htons(port);
    _my_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(_socket_fd, (sockaddr*)&_my_address, sizeof(_my_address)) == -1)
    {
      throw std::runtime_error ("UdpReceiveConnection: could not bind socket");
    }
  }

  bool receive_msg (char*         buf,
                    const size_t  buf_len)
  {
    // read the message from the socket
    if (recv(_socket_fd, buf, buf_len, 0) == -1)
    {
        return false;
    }

    return true;
  }

  int get_socket_fd ()
  {
    return _socket_fd;
  }

private:
  int                _socket_fd;

  struct sockaddr_in _my_address;
}; // class UdpReceiveConnection

} // namespace Synapse

#endif
