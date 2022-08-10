#include <iostream>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "server.h"
#include "handler.h"
#include "../includes/util.h"

using namespace std;

Server::Server() 
{
    client_id = -1;
    host = get_ip();
}
void Server::run(int port) 
{
    init_socket(port);
    loop();
}

void Server::init_socket(int port) 
{
    int backlog = 5;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd < 0)
        return;
    
    sockaddr_in server_addr;
    struct sockaddr *server_addr_ptr = (sockaddr *) &server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    int optval = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (const char *) &optval, sizeof(optval)))
        exit(-1);
    
    if (::bind(server_fd, server_addr_ptr, sizeof(server_addr)) < 0) 
    {
        if (port < 1024)
            cout << "Bind Error." << endl;
        else
            cout << "Port is being used." << endl;
        exit(-1);
    }

    if (listen(server_fd, backlog))
        exit(-1);

    cout << "Server is running." << endl;
}
void Server::loop() 
{
    while (true) 
    {
        ++client_id;
        accept();
        if (check_status())
            break;
    }
}

void *handle_client(void *_handler) 
{
    Handler *handler = (Handler *) _handler;
    handler->process();
    delete handler;
    return nullptr;
}

void Server::accept() 
{
    sockaddr_in client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int client_fd = ::accept(server_fd, (sockaddr *) &client_addr, &sin_size);
    
    if (client_fd < 0) 
    {
        cout << "Accept Error." << endl;
        return;
    }
    
    cout << "Connection from: " << \
    inet_ntoa(client_addr.sin_addr) << ":" << \
    ntohs(client_addr.sin_port) << \
    endl;
    
    pthread_t *client_thread = new pthread_t;
    Handler *handler = new Handler(client_thread, client_id, client_fd, host);
    pthread_create(client_thread, NULL, handle_client, (void *) handler);
    pthread_detach(*client_thread);
}

bool Server::check_status() 
{
    return false;
}