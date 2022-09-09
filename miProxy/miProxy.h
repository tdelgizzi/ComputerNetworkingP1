#ifndef MIPROXY
#define MIPROXY

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "DNSHeader.h"

class MiProxy {
 public:
  bool SetupSocket();
  void Start();
  MiProxy(int port, std::string www_ip, double alpha, char* log_file_p, int opt);
  MiProxy(int port, std::string dns_ip, int dns_port, double alpha, char* log_file_p, int opt);
  ~MiProxy();
 private:
  int port_;
  int socket_;
  std::string www_ip_;
  std::string dns_ip_;
  int dns_port_;
  double alpha_;
  char* log_file_p_;
  std::ofstream* log_file_;
  int opt_;
  std::vector<double> bitrates_;
  std::map<std::string, double> client_ip_avg_throughput;
  std::map<std::string, int> client_ip_video_socket;
  std::map<std::string, std::string> client_ip_video_server_ip;
  int GetVideoServerSocket_(std::string client_ip);
  std::string QueryDNS(std::string dnsIP, int dnsPort);
  std::string ModRequest(char* buffer, std::string client_ip);
  double find_small_(std::vector<double>& bitrates, double cur_throughput);
};
#endif
