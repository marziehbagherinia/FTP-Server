#include <cstdlib>
#include "client.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) 
{
    Client client;
    if (argc == 3) 
    {
        client.run(argv[1], atoi(argv[2]));
    }
    else
    {
        cout << "Client: <host> <port>" << endl;
        exit(1);    
    }
    return 0;
}