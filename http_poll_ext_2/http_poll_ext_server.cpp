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
                        //This is a server socket, accept new connection
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
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        string request(buffer, bytesRead);
        string method = request.substr(0, request.find(" "));
        string path = request.substr(request.find(" ") + 1);
        path = path.substr(0, path.find(" "));

        string response;
        if (method == "GET")
        {
            response = Get(path);
        }
        else if (method == "POST")
        {
            response = Post(request, path);
        }
        else if (method == "DELETE")
        {
            response = Delete(path);
        }
        else
        {
            response = "HTTP/1.1 405 Method Not Allowed\r\n\r\nMethod Not Allowed";
        }
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    close(clientSocket);
}

string Http_poll_ext_server::Delete(const string & path)
{
    string response;
    {
        lock_guard<mutex> lck(m_mtx);
        auto iter = m_data_store_2.find(path);
        if (iter != m_data_store_2.end())
        {
            iter->second.first++;
            response = "HTTP/1.1 200 OK\r\nX-Data-Version: " + to_string(iter->second.first) + "\r\n";
        }
        else
        {
            data_store dat{0, ""};
            deque<data_store> dq(1, dat);
            m_data_store_2[path] = {1, dq};
            response = "HTTP/1.1 200 OK\r\nX-Data-Version: 1\r\n";
        }
    }

    return response;

}

string Http_poll_ext_server::Post(const string &request, const string & path)
{
    string response;
    string body = request.substr(request.find("\r\n\r\n") + 4);
    int pos = body.find(":");
    if (pos < 0)
    {
        body = body.substr(0, body.find('0'));
        body += " : \r\n";
    }
    int version = 1;
    {
        lock_guard<mutex> lck(m_mtx);
        if (m_data_store_2.find(path) != m_data_store_2.end())
        {
            auto &cell = m_data_store_2[path];
            version = ++cell.first;
            data_store dat{version, body};
            cell.second.push_back(dat);
        }
        else
        {
            data_store dat{1, body};
            deque<data_store> deq(1, dat);
            m_data_store_2[path] = {1, deq};
        }
    }
    response = "HTTP/1.1 200 OK\r\nX-Data-Version: " + to_string(version) + "\r\n";

    return response;
}

string Http_poll_ext_server::Get(const string &str)
{
    string response;
    string path = str.substr(str.find(" ") + 1);
    path = path.substr(0, path.find(" "));
    int pos_q = path.find("?version=");
    if (pos_q >= 0)
    {
        string add = path.substr(pos_q + 9, path.find(" "));
        int vers = stoi(add);
        path = path.substr(0, pos_q);
        auto iter = m_data_store_2.find(path);
        if (iter != m_data_store_2.end())
        {
            auto item = iter->second;
            bool found = false;
            for (auto & cell : item.second)
            {
                if (cell.version == vers)
                {
                    response = "HTTP/1.1 200 OK\r\n" + cell.value + "\r\n";
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
            }
        }
        else
        {
            response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
        }
    }
    else
    {
        auto iter = m_data_store_2.find(path);
        if (iter != m_data_store_2.end())
        {
            auto cell = m_data_store_2[path];
            int version = cell.first;
            if (version > cell.second.size() || version <= 0)
            {
                response = "HTTP/1.1 404 Not Found\r\n";
            }
            else
            {
                auto last = cell.second[version - 1];
                response = "HTTP/1.1 200 OK\r\n" + last.value + "\r\n";
            }
        }
    }

    return response;
}
