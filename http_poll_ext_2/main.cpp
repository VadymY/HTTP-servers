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

#include "http_poll_ext_server.h"

int main() {
    Http_poll_ext_server server;
    return server.Start();
}

