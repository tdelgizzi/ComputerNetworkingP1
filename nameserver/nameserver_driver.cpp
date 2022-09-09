#include "nameserver.h"

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main(int argc, char *argv[]){
  int port = stoi(argv[2]);
  ifstream servers_file (argv[3]);
  ofstream log_file (argv[4],ios::trunc);
  int opt = -1;
  if (argv[1] == string("--rr")) {
    opt = 0;
  }
  else if (argv[1] == string("--geo")) {
    opt = 1;
  }
  NameServer nameserver(port, &servers_file, &log_file, opt);
  if (!nameserver.SetupSocket()) {
    return -1;
  }
  nameserver.ParseServersFile();
  nameserver.Start();
  return 0;
}
