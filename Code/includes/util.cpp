#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <string.h>
#include <ifaddrs.h>
#include "util.h"

using namespace std;

void strip(string &str) {
    int lpos = str.find_first_not_of(" \t");
    int rpos = str.find_last_not_of(" \t");
    if (lpos == string::npos && rpos == string::npos) {
        str = "";
        return;
    }
    str = str.substr(lpos, rpos - lpos + 1);
}

void parse_command(string &str, string &cmd, string &args) {
    int pos = str.find_first_of(" ");
    if (pos == string::npos) {
        cmd = str;
        args = "";
    } else {
        cmd = str.substr(0, pos);
        args = str.substr(pos + 1, str.length() - pos - 1);
    }
}

unsigned int get_ip() {
    struct ifaddrs *if_addr_struct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmp_addr_ptr = NULL;
    unsigned int ip = 0;
    getifaddrs(&if_addr_struct);
    for (ifa = if_addr_struct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmp_addr_ptr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addr_buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmp_addr_ptr, addr_buf, INET_ADDRSTRLEN);
            ip = parse_ip(string(addr_buf));
            int first = ((ip >> 24) & 0xFF);
            if (first != 0 && first != 127 && first != 10)
                break;
        }
    }
    if (if_addr_struct != NULL)
        freeifaddrs(if_addr_struct);
    return ip;
}

unsigned int parse_ip(const string &ip) {
    if (ip == "")
        return 0;
    string ipstr = ip + '.';
    int h[4];
    int count = 0, lastsep = -1;
    for (int i = 0; i < ipstr.size(); ++i)
        if (ipstr[i] == '.') {
            string tmp = ipstr.substr(lastsep + 1, i - lastsep - 1);
            h[count] = atoi(tmp.c_str());
            lastsep = i;
            ++count;
        }
    unsigned ret = h[0];
    for (int i = 1; i < 4; ++i)
        ret = (ret << 8) + h[i];
    return ret;
}