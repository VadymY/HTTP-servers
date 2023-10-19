#include <sys/resource.h>
#include "http_poll_ext_server.h"

Http_poll_ext_server::Http_poll_ext_server(int port):
    m_port(port),
    m_maxClients(2000)
{

}

int Set_limit(int expect_limit)
{
    struct rlimit rl;

    // Get the current descriptor limit
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        cout << "Current soft limit: " << rl.rlim_cur << endl;
        cout << "Current hard limit: " << rl.rlim_max << endl;
    } else {
        perror("getrlimit");
        return 1;
    }

    // Set the current descriptor limit
    rl.rlim_cur = expect_limit;
    if (setrlimit(RLIMIT_NOFILE, &rl) == 0) {
        cout << "New soft limit: " << rl.rlim_cur << endl;
    } else {
        perror("setrlimit");
        return 1;
    }

}

int Http_poll_ext_server::Start()
{
    int res_lim = Set_limit(m_maxClients + 10);

    if (res_lim == 1)
    {
        cout << "Impossible to set the maximum connections = " << m_maxClients << endl;
        return 1;
    }
    else{
    // Create a server socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == -1) {
        perror("socket");
        return 1;
    }

    // Set up server address
    memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_port);
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    // Set socket options
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to address
    if (bind(m_serverSocket, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) == -1) {
        perror("bind");
        return 1;
    }

    // Start listening for client connections
    if (listen(m_serverSocket, m_maxClients) == -1) {
        perror("listen");
        return 1;
    }

    vector<pollfd> pollfds(m_maxClients + 1);
    pollfds[0].fd = m_serverSocket;
    pollfds[0].events = POLLIN;

    socklen_t clientLen = sizeof(m_clientAddr);

    while (true) {
        // Call poll() to wait for events
        int pollResult = poll(pollfds.data(), pollfds.size(), -1);
        if (pollResult == -1) {
            perror("poll");
            return 1;
        }

        // Process events
        for (int i = 0; i < pollfds.size(); ++i) {
            if (pollfds[i].revents & POLLIN) {
                if (i == 0) {
                    // This is a server socket, accept new connection
                    m_clientSocket = accept(m_serverSocket, (struct sockaddr*)&m_clientAddr, &clientLen);
                    if (m_clientSocket == -1) {
                        perror("accept");
                        continue;
                    }

                    // Add client socket to pollfd
                    for (int j = 1; j < pollfds.size(); ++j) {
                        if (pollfds[j].fd == 0) {
                            pollfds[j].fd = m_clientSocket;
                            pollfds[j].events = POLLIN;
                            break;
                        }
                    }
                } else {
                    // This is a client socket, process the request
                    thread(&Http_poll_ext_server::handleClient, this, pollfds[i].fd).detach();
                    pollfds[i].fd = -1; // Mark the socket for closure after processing
                }
            }
        }
    }

    close(m_serverSocket);
    }

    return 0;
}

// Client Handler
void Http_poll_ext_server::handleClient(int clientSocket)
{
    char buffer[1024];
    ssize_t bytesRead;

    // Read client request
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        string request(buffer, bytesRead);
        string method = request.substr(0, request.find(" "));
        string path = request.substr(request.find(" ") + 1);
        path = path.substr(0, path.find(" "));

        string response;
        if (method == "GET") {
            {
                lock_guard<mutex> lck(m_mtx);
                if (m_data_store.find(path) != m_data_store.end()) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: " + m_data_store[path].first;
                    response += "\r\n" + m_data_store[path].second + "\r\n";
                } else {
                    response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
                }
            }
        } else if (method == "POST") {
            string contentType = request.substr(request.find("Content-Type: ") + 14);
            contentType = contentType.substr(0, contentType.find("\r\n"));
            string body = request.substr(request.find("\r\n\r\n") + 4);
            int pos = body.find(":");
            if (pos < 0){
                pos = body.find(" ");
                if (pos >= 0){
                    body.replace(pos, 1, " : ");
                }
            }
            {
                lock_guard<mutex> lck(m_mtx);
                m_data_store[path] = {contentType, body};
            }
            response = "HTTP/1.1 200 OK\r\n\r\nOK";
        } else if (method == "DELETE") {
            {
                lock_guard<mutex> lck(m_mtx);
                m_data_store.erase(path);
            }
            response = "HTTP/1.1 200 OK\r\n\r\nOK";
        } else {
            response = "HTTP/1.1 405 Method Not Allowed\r\n\r\nMethod Not Allowed";
        }
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    close(clientSocket);
}
