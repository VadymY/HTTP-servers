# Start Here

1. Use this template repository to create a private fork (click the green `Use this template` button and not the `Fork` button).
1. Follow the instructions in `INSTRUCTIONS.md`.
1. Give [@tes-mersive](https://github.com/tes-mersive) (`estephens@mersive.com`) collaborator access when complete.
1. Inform your Mersive contact that the assignment is complete.

# Your Documentation

 I completed the task in 2 versions: main and bonus. Accordingly, these 2 projects are located in different folders: http_poll_ext and http_poll_ext_2/.

I created an http server based on POLL multiplexing to poll multiple clients. This technology removes the limitation of 1023 client connections that were in the previous multiplexing technology SELECT. The server supports the technology "keep-alive" with clients.

Applications were created in QT Creator.

Also, the application (an example is given for the basic version) can be compiled without using QT Creator using commands from the terminal:

DEBUG:

g++ -c -pipe -g -std=gnu++1z -Wall -W -fPIC -DQT_QML_DEBUG -I../http_poll -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++ -o http_poll_ext_server.o ../http_poll/http_poll_ext_server.cpp

g++ -c -pipe -g -std=gnu++1z -Wall -W -fPIC -DQT_QML_DEBUG -I../http_poll -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++ -o main.o ../http_poll/main.cpp

g++ -o http_poll_ext http_poll_ext_server.o main.o -pthread

AND
RELEASE:

g++ -c -pipe -O2 -std=gnu++1z -Wall -W -fPIC -I../http_poll -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++ -o http_poll_ext_server.o ../http_poll/http_poll_ext_server.cpp

g++ -c -pipe -O2 -std=gnu++1z -Wall -W -fPIC -I../http_poll -I. -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++ -o main.o ../http_poll/main.cpp

g++ -Wl,-O1 -o http_poll_ext http_poll_ext_server.o main.o -pthread

On this server, I used multithreading to process client requests.

    When the server starts, a function is called that sets the limit on the number of descriptors to 2000. The server is designed for the same number of clients.
    
    Next, I do the standard server socket initialization and also set the address and port the server is running on to reusable mode so that when the server is shut down, 
    
    the new server can run on the same descriptor.
    
    The server supports GET, POST, DELETE requests. Different processing and response logic is implemented for different options (main and bonus).
