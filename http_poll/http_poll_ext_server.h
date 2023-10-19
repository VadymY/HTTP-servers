#ifndef HTTP_POLL_EXT_SERVER_H
#define HTTP_POLL_EXT_SERVER_H

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <map>
#include <mutex>

using namespace std;

class Http_poll_ext_server
{
public:
    Http_poll_ext_server(int port = 8000);
    int Start();

private:
    int m_port;
    map<string, pair<string, string>> m_data_store;
    mutex m_mtx;
    const int m_maxClients;
    int m_serverSocket;
    int m_clientSocket;
    struct sockaddr_in m_serverAddr;
    struct sockaddr_in m_clientAddr;

    void handleClient(int clientSocket);
};

#endif // HTTP_POLL_EXT_SERVER_H
