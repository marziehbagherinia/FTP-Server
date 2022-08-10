#include <iostream>
#include <cstring>
#include <cstdlib>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "client.h"
#include "../includes/const.h"

using namespace std;

void Client::run(const char *host, int port) 
{
    connect(host, port);
    if (is_connected)
        login();
    loop();
    close();
}

void Client::connect(const char *host, int port) 
{
    this->host = string(host);

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(port);
    
    int ret = ::connect(client_fd, (struct sockaddr *) &addr, sizeof(addr));
    
    if (ret < 0) {
        cout << "Can't connect to " + this->host << endl;
        return;
    }
    
    Response res = get_response();
    if (res.code == SERVER_READY) {
        is_connected = true;
    } else cout << res.msg;
}


bool Client::login() 
{
    bool user_flag = false;
    bool pass_flag = false;
    bool login_flag = false;
    cout << "FTP is running! You need to login:" << endl;

    while (pass_flag == false)
    {
        if(user_flag == false)
        {
            login_flag = do_user();
            user_flag = login_flag;
        }

        if (login_flag == true)
        {
            pass_flag = do_pass();
        }                    
    }

    return true;    
}

void Client::loop() 
{
    string cmd, args;
    
    while (true) 
    {
        cout << "ftp> ";
        get_command(cmd, args);
        
        if (cmd == "pwd") 
        {
            do_pwd();
        }
        else if (cmd == "mkd") 
        {
            do_mkd(args);
        } 
        else if (cmd == "cwd") 
        {
            do_cwd(args);
        }
        else if (cmd == "ls") 
        {
            do_ls();
        }
        else if (cmd == "rename") 
        {
            do_rename(args);
        }
        else if (cmd == "dele") 
        {
            do_delete(args);
        }
        else if (cmd == "retr") 
        {
            do_get(args);
        } 
        else if (cmd == "help") 
        {
            show_help();
        }  
        else if (cmd == "quit") 
        {
            do_goodbye();
            break;
        } 
        else if (cmd == "user") {
            do_user();
        }
        else {
            cout << "501: Syntax error in parameters or arguments." << endl;
            continue;
        }
    }
}


void Client::close() 
{
    ::close(client_fd);
}

void Client::send_command(const string &str) 
{
    int res = send(client_fd, (str + "\r\n").c_str(), str.length() + 2, 0);
    if (res < 0) {
        cout << "Server quit!" << endl;
        exit(0);
    }
}

Response Client::get_response() 
{
    char recv_buf[5000];
    int length = recv(client_fd, recv_buf, 5000, 0);
    if (length < 0) {
        cout << "Server error. Quit." << endl;
        exit(0);
    }
    recv_buf[length - 2] = '\0';
    Response res;
    res.msg = string(recv_buf);
    int pos = res.msg.find(' ');
    int code = char_to_code(recv_buf[pos - 3], recv_buf[pos - 2], recv_buf[pos - 1]);
    res.code = code;
    cout << res.msg << endl;
    return res;
}

int Client::get_command(string &cmd, string &args) 
{
    string str;

    if (!getline(cin, str))
        return -1;
    
    strip(str);
    
    int pos = str.find_first_of(" ");
    
    if (pos == string::npos) 
    {
        cmd = str;
        args = "";
    } 
    else 
    {
        cmd = str.substr(0, pos);
        args = str.substr(pos + 1, str.size() - pos - 1);
        strip(cmd);
        strip(args);
    }

    for (int i = 0; i < cmd.size(); ++i)
        if (cmd[i] >= 'A' && cmd[i] <= 'Z')
            cmd[i] += 'a' - 'A';
    
    return 0;
}


bool Client::do_user() 
{
    string username, command, space;
    vector <string> word;

    if (!getline(cin, command))
        return false;

    istringstream ss(command);
    
    while (ss >> space) 
        word.push_back(space);

    if (word.size() > 1 && word[0] == "user")
        username = word[1];

    strip(username);

    send_command("USER " + username);
    Response res = get_response();

    if (res.code == RIGHT_USERNAME || res.code == LOGIN_SUCCESS)
        return true;

    if (res.code == INVALID_USER_PASS)
        return false;

    return false;
}

bool Client::do_pass() 
{
    string password, space, command;
    vector <string> word;

    //cout << "Password: ";

    if (!getline(cin, command))
        return false;

    istringstream ss(command);

    while (ss >> space)
        word.push_back(space);

    if (word.size() > 1 && word[0] == "pass")
        password = word[1];
    
    strip(password);
    send_command("PASS " + password);
    Response res = get_response();

    return res.code == LOGIN_SUCCESS;
}

void Client::do_pwd() 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("PWD");
    get_response();
}

void Client::do_mkd(std::string &args) 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("MKDIR " + args);
    get_response();
}

void Client::do_cwd(string &args) 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("CWD " + args);
    get_response();
}

void Client::do_ls() 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("PASV");
    Response res = get_response();

    if (res.code != ENTER_PASSIVE)
        return;
    
    get_pasv_ip_port(res);
    int data_fd = get_data_conn();
    
    if (data_fd < 0)
        return;
    
    send_command("LIST");
    res = get_response();

    if (res.code == OPENING_DATAPORT || res.code == TRANSFERING) 
    {
        string data;
        int buf_max_size = 1024;
        char buf[buf_max_size + 1];

        while (true) 
        {
            int size = recv(data_fd, buf, buf_max_size, 0);
            if (size <= 0) break;
            buf[size] = '\0';
            data += string(buf);
        }
        cout << data << endl;
    }

    get_response();
    ::close(data_fd);
}

void Client::do_rename(std::string &args) 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("RENAME " + args);
    get_response();
}

void Client::do_delete(std::string &args) 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("DEL " + args);
    get_response();
}

void Client::do_goodbye() 
{
    if (!is_connected) {
        exit(0);
    }
    send_command("QUIT");
    get_response();
}

int Client::get_data_conn() 
{
    int data_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in data_addr;
    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = inet_addr(host.c_str());
    data_addr.sin_port = htons(port);
    if (data_fd < 0) {
        cout << "Init Data Socket Error" << endl;
        return -1;
    }
    if (::connect(data_fd, (struct sockaddr *) &data_addr, sizeof(struct sockaddr))) {
        cout << "Connect Error" << endl;
        return -1;
    }
    return data_fd;
}

void Client::get_pasv_ip_port(Response &res) 
{
    string &msg = res.msg;
    int lpos = msg.find('(') + 1;
    int rpos = msg.rfind(')');
    int count = 0, last_sep = -1;
    for (int i = lpos; i <= rpos; ++i) {
        if (msg[i] == ',' || msg[i] == ')') {
            msg[i] = '.';
            ++count;
            if (count == 4)
                host = msg.substr(lpos, i - lpos);
            else if (count == 5)
                port = atoi(msg.substr(last_sep + 1, i - last_sep - 1).c_str());
            else if (count == 6) {
                int tmp = atoi(msg.substr(last_sep + 1, i - last_sep - 1).c_str());
                port = (port << 8) + tmp;
            }
            last_sep = i;
        }
    }
}

int Client::do_get(string &args) 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return -1;
    }

    fstream file;
    file.open(args.c_str(), ios::out | ios::trunc | ios::binary);

    if (!file.is_open()) 
    {
        cout << "Open output file Error!" << endl;
        return 0;
    }

    send_command("PASV");
    Response res = get_response();
    if (res.code != ENTER_PASSIVE)
        return 0;
    get_pasv_ip_port(res);
    int data_fd = get_data_conn();
    if (data_fd < 0)
        return 0;

    send_command("RETR " + args);
    res = get_response();

    if (res.code == 550) {
        unlink(args.c_str());
        return 0;
    }
    
    char buffer[1024] = {};
    int valread = read(data_fd , buffer, 1024);
    cout << "Data received "<< valread <<" bytes\n";
    cout << "Saving data to file.\n";
            
    file << buffer;
    cout << "File Saved.\n";

    file.close();
    get_response();
    ::close(data_fd);
    return 0;
}

void Client::show_help() 
{
    if (!is_connected) {
        cout << "Not connected." << endl;
        return;
    }
    send_command("HELP");
    get_response();
}