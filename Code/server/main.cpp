#include <cstdlib>
#include "server.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) 
{
    Server server;
    
    if (argc == 2) 
    {
        server.run(atoi(argv[1]));
    }
    else
    {
        cout << "Server: <port>" << endl;
        exit(1);    
    }
    return 0;
}