#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <netdb.h>
#include "TCPStandard.h"

using namespace std;

class TCPClient
{
  private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;

  public:
    TCPClient();
    bool setup(string address, int port);
    bool Send(string data);
    bool Send(void* data, size_t len);
    string receive(int size = 4096);
    char* receive_exact(int size);
    string receive_name();
    long receive_size_of_data();
    string read();
    void exit();
};

#endif
