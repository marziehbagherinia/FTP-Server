#ifndef ZEROFTP_UTIL_H
#define ZEROFTP_UTIL_H

#include <string>

extern void strip(std::string &str);

extern void parse_command(std::string &str, std::string &cmd, std::string &args);

extern unsigned int get_ip();

extern unsigned int parse_ip(const std::string &ip);

struct Response {
    int code;
    std::string msg;
};

#endif