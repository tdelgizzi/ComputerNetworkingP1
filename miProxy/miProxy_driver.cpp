#include "miProxy.h"

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main(int argc, char *argv[]){
  MiProxy* miProxy_p;
  if (argv[1] == string("--nodns")) {
    int opt = 0;
    int port = stoi(argv[2]);
    string www_ip = argv[3];
    double alpha = stod(argv[4]);
    char* log_file = argv[5];
    MiProxy miProxy = MiProxy(port, www_ip, alpha, log_file, opt);
    miProxy_p = &miProxy;
  }
  else if (argv[1] == string("--dns")) {
    int opt = 1;
    int port = stoi(argv[2]);
    string dns_ip = argv[3];
    int dns_port = stoi(argv[4]);
    double alpha = stod(argv[5]);
    char* log_file = argv[6];
    MiProxy miProxy = MiProxy(port, dns_ip, dns_port, alpha, log_file, opt);
    miProxy_p = &miProxy;
  }
  if (!miProxy_p->SetupSocket()) {
    return -1;
  }
  miProxy_p->Start();
  return 0;
}
