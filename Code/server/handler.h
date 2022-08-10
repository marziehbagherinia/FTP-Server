#ifndef ZEROFTP_HANDLER_H
#define ZEROFTP_HANDLER_H

#include <pthread.h>
#include <string>
#include <netinet/in.h>
#include <map>
#include <vector>
#include <filesystem>

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "json.hpp"

using namespace std;

class Handler {
private:
    pthread_t *p_client_thread;
    unsigned int local_ip;
    int client_id;
    int client_fd;
    int data_listen_fd;
    int data_conn_fd;
    bool is_passive;
    bool need_login;
    bool need_username;
    bool is_logined;

    int dataChannelPort;

    sockaddr_in local_addr;
    std::string curr_dir;
    std::string username;
    //std::map<std::string, std::string> users;
    vector <string> files;
    map <string, vector<string>> users;

    int send_response(int code, const std::string &msg);

    int exec(std::string &cmd, std::string &args);

    int passive_mode();

    int get_listen_port();

    int init_data_socket();

    int get_data_fd();

    int send_with_crlf(const std::string &str);

    int send_file(FILE *file);

    void init_users();

public:
    void process();

    Handler(pthread_t *p_thread, int id, int fd, unsigned int ip);

    ~Handler();

    int handle_rename(std::string &filename);

    int handle_delete(std::string &filename);

    int handle_ls(std::string &dir);

    int handle_get(std::string &args);

    int handle_cd(std::string &args);

};

#endif 