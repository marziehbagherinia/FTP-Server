#include <iostream>
#include <fstream>
#include <string.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "handler.h"
#include "../includes/const.h"
#include "../includes/util.h"

using namespace std;

Handler::Handler(pthread_t *p_thread, int id, int fd, unsigned int ip) : p_client_thread(p_thread), local_ip(ip),
                                                                         client_id(id),
                                                                         client_fd(fd) {
    init_users();
    is_passive = false;
    is_logined = false;
    curr_dir = "./";
    cout << "Client id : " << client_id << endl;
}

Handler::~Handler() 
{
    ::close(client_fd);
    delete p_client_thread;
}

void Handler::init_users() 
{
    nlohmann::json config;
    ifstream file_reader("includes/config.json", std::ifstream::binary);
    file_reader >> config;

    dataChannelPort = config["dataChannelPort"];
    
    vector <string> client_inf(3, "");

    for (int i = 0; i < config["users"].size(); i++)
    {
        client_inf[0] = config["users"][i]["password"];
        client_inf[1] = config["users"][i]["admin"];
        client_inf[2] = config["users"][i]["size"];

        users[config["users"][i]["user"]] = client_inf;
    }

    for (int i = 0; i < config["files"].size(); i++)
    {   
        files.push_back(config["files"][i]);
    }

    need_login = users.size() > 0;
    need_username = true;    
}

void Handler::process() 
{
    int res = send_response(SERVER_READY, "FTP Server is ready.");
    if (res < 0) {
        cout << "Client " << client_id << " quit!" << endl;
        return;
    }
    string str, cmd, args;
    
    while (true) 
    {
        char buf[256];
        int length = recv(client_fd, buf, 256, 0);
        if (length <= 0) {
            cout << "Client " << client_id << " quit!" << endl;
            return;
        }
        if (buf[length - 2] == '\r' && buf[length - 1] == '\n') {
            buf[length - 2] = '\0';
            str = string(buf);
        
            parse_command(str, cmd, args);
            //cout << str << endl;
            int is_end = exec(cmd, args);
            if (is_end == 1) {
                cout << "Client " << client_id << " quit!" << endl;
                break;
            }
        } else {
            cout << "Bad Command." << endl;
        }
    }
}

int Handler::send_response(int code, const std::string &msg) 
{
    string str = to_string(code) + ' ' + msg;
    int res = send(client_fd, (str + "\r\n").c_str(), str.length() + 2, 0);
    return res;
}

int Handler::exec(std::string &cmd, std::string &args) 
{
    cout << "Client " << client_id << " : [" << cmd << ' ' << args << ']' << endl;
    if (cmd == "USER") 
    {
        if (need_login) 
        {
            username = args;
            if (users[username].size() > 0)
            {
                need_username = false;
                send_response(RIGHT_USERNAME, "User name okay, need password.");
            }
            else
            {
                send_response(INVALID_USER_PASS, "Invalid username or password.");
            }
        } 
        else 
        {
            send_response(LOGIN_SUCCESS, "User logged in, proceed. Logged out if appropriate.");
        }
    } 
    else if (cmd == "PASS") 
    {
        if(need_username)
        {
            is_logined = false;
            send_response(BAD_SEQ, "Bad sequence of commands.");
        }
        else if (users[username].size() <= 0 || users[username][0] == "" || users[username][0] != args) 
        {
            is_logined = false;
            send_response(INVALID_USER_PASS, "Invalid username or password.");
        } 
        else if(users[username].size() > 0 && users[username][0] == args)
        {
            is_logined = true;
            need_login = false;
            send_response(LOGIN_SUCCESS, "User logged in, proceed. Logged out if appropriate.");
        }
    } 
    else if (cmd == "PWD") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            char tmp[256];
            getcwd(tmp, 256);
            send_response(PATHNAME_CREATED, tmp);
        }
    } 
    else if (cmd == "MKDIR") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        }    
        else if (mkdir(args.c_str(), 0777) == -1)
        {
            send_response(PATHNAME_CREATED, "Directory exists");
        }
        else
        {
            string response = args + " created";
            send_response(PATHNAME_CREATED, response);
        }
    }
    else if (cmd == "CWD") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            handle_cd(args);
        }
    }
    else if (cmd == "LIST") 
    {
        if (!is_logined) {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            if (handle_ls(curr_dir) < 0)
                cout << "Send List Data Error" << endl;
        }
    }
    else if (cmd == "PASV") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            int ret = passive_mode();
            if (ret < 0) 
            {
                cout << "Enter PASV Mode Error." << endl;
                return 1;
            }
        }
    } 
    else if (cmd == "RENAME") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            handle_rename(args);
        }
    } 
    else if (cmd == "DEL") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            handle_delete(args);
        }
    }
    else if (cmd == "RETR") 
    {
        if (!is_logined) 
        {
            send_response(NOT_LOGINED, "Need account for login");
        } 
        else 
        {
            handle_get(args);
        }
    }
    else if (cmd == "QUIT") 
    {
        send_response(GOODBYE, "Goodbye, closing session.");
        return 1;
    } 
    else if (cmd == "HELP") 
    {
        send_response(HELP_COMMAND,"\nUSER <username>: used to login by entering your username.\nPASS <password>: Enter your password after submitting your username.\nPWD: Returns the path of your working directory.\nMKD <directory path>: Creates a directory in the path given.\nDELE -f <file name>: Deletes the file asked.\nDELE -d <directory path>: Deletes the directory in the path given.\nLS: Lists the files in your cworking directory.\nCWD <path>: Change your directory to the path given. If the argument is empty, it will vhange your working directory to the root directory.\nRETR <name>: Downloads the file if it exists and the user can access it.\nHELP: Lists all the commands and a breif description.\nQUIT: Quits the client from the server.");
    }
    else 
    {
        send_response(NOT_IMPLEMENTED, "Syntax error in parameters or arguments.");
    }
    return 0;
}

int Handler::passive_mode() 
{
    if (is_passive)
        ::close(data_listen_fd);
    
    if (init_data_socket() < 0) 
        return -1;

    int port = dataChannelPort;
    
    if (port < 0)
        return -1;
    
    char addr_port_buf[128];
    int addr = local_ip;

    sprintf(addr_port_buf, "(%d,%d,%d,%d,%d,%d)",
            (addr >> 24) & 0xFF,
            (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF,
             addr & 0xFF,
            (port >> 8) & 0xFF,
             port & 0xFF);
             
    send_response(ENTER_PASSIVE, "Entering Passive Mode " + string(addr_port_buf));
    
    is_passive = true;
    return 0;
}

int Handler::handle_ls(string &path) 
{
    data_conn_fd = get_data_fd();
    
    if (data_conn_fd < 0) {
        cout << "Data Connection Accept Error" << endl;
        return 0;
    }
    int pipe_fd[2];
    
    if (pipe(pipe_fd))
        return -1;
    
    pid_t chpid = fork();
    
    if (chpid < 0)
        return -1;
    
    if (!chpid) 
    {
        close(pipe_fd[0]);
        if (dup2(pipe_fd[1], fileno(stdout)) < 0)
            return -1;
        if (dup2(pipe_fd[1], fileno(stderr)) < 0)
            return -1;
        execl("/bin/ls", "ls", "-l", path.c_str(), NULL);
        exit(0);
    }

    close(pipe_fd[1]);

    
    int buf_max_size = 1024;
    char buf[buf_max_size + 1];
    
    while (true) 
    {
        int size = read(pipe_fd[0], buf, buf_max_size);
        
        buf[size] = '\0';
        
        if (size <= 0) 
        {
            int waiting = waitpid(chpid, NULL, 0);
            if (waiting < 0)
                return -1;
            if (waiting == chpid)
                break;
        } else 
        {
            send_with_crlf(string(buf));
        }
    }
    close(pipe_fd[0]);
    close(data_conn_fd);
    send_response(CLOSE_DATA_CONNECTION, "List transfer done.");
    return 0;
}

int Handler::send_with_crlf(const std::string &str) 
{
    int last_pos = 0;
    
    for (int i = 1; i < str.size(); ++i) 
    {
        if (str[i - 1] != '\r' && str[i] == '\n') 
        {
            string data = str.substr(last_pos, i - last_pos) + "\r\n";
            send(data_conn_fd, data.c_str(), data.length(), 0);
            last_pos = i + 1;
        }
    }
    if (last_pos < str.size()) 
    {
        string data = str.substr(last_pos, str.size() - last_pos);
        send(data_conn_fd, data.c_str(), data.length(), 0);
    }
    return 0;
}

int Handler::handle_rename(std::string &args) 
{
    string src_file, dest_file, space;
    vector <string> word;

    istringstream ss(args);

    while (ss >> space)
        word.push_back(space);

    if (word.size() >= 2)
    {
        int value = rename(word[0].c_str(), word[1].c_str());
        if(!value)
        {
            send_response(ACTION_DONE, "Successful change.");
        }
        else
        {
            perror("Error");   
        }
    }
    else
    {
        perror("Error");
    }

    return 0;
}

int Handler::handle_delete(std::string &args) 
{
    string src_file, dest_file, space;
    vector <string> word;

    istringstream ss(args);

    while (ss >> space)
        word.push_back(space);

    if (word.size() >= 2)
    {
        if(word[0] == "-f" || word[0] == "-d")
        {
            if (remove(word[1].c_str()) == 0)
            {
                send_response(ACTION_DONE, word[1] + " deleted.");
            }
            else
            {
                perror("Error");
            }
        }   
    }
    else
    {
        perror("Error");
    }

    return 0;
}

int Handler::get_data_fd() 
{
    if (!is_passive) {
        send_response(CANNOT_OPEN_DATA_CONNECTION, "Not in PASV mode");
        return -1;
    }
    socklen_t sin_size = sizeof(sockaddr_in);
    sockaddr_in data_addr;
    int data_fd = accept(data_listen_fd, (sockaddr *) &data_addr, &sin_size);
    if (data_fd < 0)
        return -1;
    ::close(data_listen_fd);
    is_passive = false;
    send_response(OPENING_DATAPORT, "Data Connection Open");
    return data_fd;
}

int Handler::send_file(FILE *file) 
{
    int buf_max_size = 1024;
    char buf[buf_max_size + 1];
    
    while (true) 
    {
        int size = fread(buf, 1, buf_max_size, file);
        
        cout << "sizeeeeeeeeee " << size << endl;
        if (size <= 0)
            break;
        
        buf[size] = '\0';
        if (send(data_conn_fd, buf, size, 0) < 0) 
        {
            cout << "Send Error" << endl;
            return -1;
        }
    }
    return 0;
}

int Handler::handle_get(string &args) 
{
    if (users[username].size() > 0)
    {
        if (users[username][1] == "false")
        {
            for (int i = 0; i < files.size(); i++)
            {
                if (files[i] == args)
                {
                    send_response(ACTION_FAILED, "You do not have permission to access this file.");
                    return 0;
                }
                
            }
   
        } 
    }
    fstream file;
    file.open(args.c_str(), ios::in | ios::binary);
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file.is_open()) 
    {
        send_response(REQUESTED_ACTION_NOT_TAKEN, "Fail to Open Input File");
        return 0;
    }

    if(users[username].size() > 0)
    {
        if ((contents.length()/1000) > stoi(users[username][2]))
        {
            send_response(ACTION_FAILED, "You do not have enough space.");
            return 0;
        }
    }

    data_conn_fd = get_data_fd();
    
    if (data_conn_fd < 0) 
    {
        send_response(ACTION_FAILED, "Data Connection Accept Error");
        return 0;
    }

    cout << "Transfering..." << endl;

    int bytes_sent = send(data_conn_fd , contents.c_str() , contents.length() , 0);

    if(bytes_sent < 0)
    {
        cout << "Send File Data Error" << endl;
        return 0;
    }

    cout << "Transmitted Data Size " << bytes_sent << " Bytes.\n";
    file.close();
    close(data_conn_fd);
    send_response(CLOSE_DATA_CONNECTION, "Successful Download.");
    return 0;
}

int Handler::handle_cd(std::string &args) 
{
    char tmp[256];
            
    if (chdir(args.c_str()) == 0) 
    {
        send_response(ACTION_DONE, "Successful change.");
        curr_dir = getcwd(tmp, 256);
    } 
    else 
    {
        send_response(ACTION_FAILED, "Failed to change directory.");
    }
    return 0;
}

int Handler::get_listen_port() 
{
    sockaddr *local_addr_ptr = (sockaddr *) &local_addr;
    socklen_t addrlen = sizeof(local_addr);
    memset(&local_addr, 0, addrlen);
    if (getsockname(data_listen_fd, local_addr_ptr, &addrlen))
        return -1;
    int port = ntohs(local_addr.sin_port);
    return port;
}

int Handler::init_data_socket() 
{
    data_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data_listen_fd < 0)
        return -1;

    sockaddr_in svrdata_addr;
    sockaddr *svrdata_addr_ptr = (sockaddr *) &svrdata_addr;
    
    memset(&svrdata_addr, 0, sizeof(svrdata_addr));
    svrdata_addr.sin_family = AF_INET;
    svrdata_addr.sin_addr.s_addr = INADDR_ANY;
    svrdata_addr.sin_port = htons(dataChannelPort);
    int optval = 1;
    
    if (setsockopt(data_listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof(optval)))
        return -1;
    if (::bind(data_listen_fd, svrdata_addr_ptr, sizeof(svrdata_addr)) < 0)
        return -1;
    if (listen(data_listen_fd, 5))
        return -1;
    return 0;
}