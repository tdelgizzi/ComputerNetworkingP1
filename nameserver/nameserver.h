#ifndef NAMESERVER
#define NAMESERVER

#include <fstream>
#include <vector>
#include <string>
#include <map>
#include "DNSHeader.h"

class NameServer {
 public:
  bool SetupSocket();
  void Start();
  void ParseServersFile();
  NameServer(int port, std::ifstream* severs_file, std::ofstream* log_file, int opt);
  ~NameServer();
 private:
  int port_;
  int socket_;
  std::ifstream* servers_file_;
  std::ofstream* log_file_;
  int opt_;
  std::vector<std::string> rr_list_;
  int cur_rr_idx_;
  std::map<std::string, std::string> clientip_serverip_;
  void SetHeader_(DNSHeader* header, DNSHeader* query_header);
  int find_small_(std::vector<int>& visited, std::vector<int>& distance);
  std::string GetResponseData_(struct sockaddr_in* client_addr);
};
#endif
