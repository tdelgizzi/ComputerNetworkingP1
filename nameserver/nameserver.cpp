#include "nameserver.h"
#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>
#include <error.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <iomanip>
#include <cstdio>
#include <map>
#include <limits>
#include <sys/time.h>

using namespace std;

bool NameServer::SetupSocket() {
  // create a socket
  socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == -1) {
    cerr << "Error: bad socket\n";
    return false;
  }
  int ok = 1;
  // to reuse ports
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok)) == -1) {
    cerr << "Error: bad setsockopt\n";
    return false;
  }
  struct sockaddr_in addr;
  socklen_t length = (socklen_t)sizeof(addr);
  memset(&addr, 0, length);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);
  // bind address to our socket
  if (bind(socket_, (const struct sockaddr *)&addr, length) == -1) {
    cerr << "Error: bad bind\n";
    return false;
  }
  // listen for connections
  if (listen(socket_, 10) == -1) {
    cerr << "Error: bad listen\n";
    return false;
  }

  return true;
}

int NameServer::find_small_(vector<int>& visited, vector<int>& distance) {
  int cur_smallest = numeric_limits<int>::max();
  int cur_smallest_idx = numeric_limits<int>::max();
  int cur_idx = 0;
  while (cur_idx < distance.size()) {
    if (!visited[cur_idx] && distance[cur_idx] <= cur_smallest) {
      cur_smallest = distance[cur_idx];
      cur_smallest_idx = cur_idx;
    }
    cur_idx += 1;
  }
  return cur_smallest_idx;
}

void NameServer::ParseServersFile() {
  // round robin
  if (opt_ == 0) {
    cur_rr_idx_ = 0;
    if (servers_file_->is_open()) {
      string line;
      while (getline(*servers_file_, line)) {
        rr_list_.push_back(line);
      }
      servers_file_->close();
    }
  }
  // geo
  else if (opt_ == 1) {
    if (servers_file_->is_open()) {
      string line;
      getline(*servers_file_,line);
      int num_nodes = stoi(line.substr(line.find(":")+1, string::npos));
      int cur_idx = 0;
      vector<int> server_ids;
      vector<int> client_ids;
      map<int, string> id_ip;
      while (cur_idx < num_nodes) {
        getline(*servers_file_,line);
        int node_id = stoi(line.substr(0,line.find(" ")));
        string node_type = line.substr(line.find(" ")+1,6);
        int first_space_pos = line.find(" ");
        int second_space_pos = line.find(" ", first_space_pos+1);
        string ip = line.substr(second_space_pos + 1,string::npos);
        if (node_type == "SERVER") {
          server_ids.push_back(node_id);
        }
        if (node_type == "CLIENT") {
          client_ids.push_back(node_id);
        }
        id_ip[node_id] = ip;
        cur_idx++;
      }

      getline(*servers_file_,line);
      int num_links = stoi(line.substr(line.find(":")+1, string::npos));

      map<int, vector<int>> adj_list;
      int cost[num_nodes][num_nodes];
      int inf = numeric_limits<int>::max();
      for (int i = 0; i< num_nodes; i++) {
        for (int j = 0; j< num_nodes; j++) {
          cost[i][j] = inf;
        }
      }

      cur_idx = 0;
      while (cur_idx < num_links) {
        getline(*servers_file_,line);
        int first_space_pos = line.find(" ");
        int second_space_pos = line.find(" ", first_space_pos + 1);

        int src = stoi(line.substr(0,first_space_pos));
        int des = stoi(line.substr(first_space_pos+1,second_space_pos-first_space_pos-1));
        int cos = stoi(line.substr(second_space_pos+1,string::npos));
        cost[src][des] = cos;
        cost[des][src] = cos;
        adj_list[src].push_back(des);
        adj_list[des].push_back(src);
        cur_idx += 1;
      }

      // dijkstra
      for (int client_id : client_ids) {
        vector<int> visited(num_nodes, 0);
        vector<int> distance(num_nodes, inf);
        distance[client_id] = 0;
        int root = client_id;
        while (root != inf) {
          for (int child : adj_list[root]) {
            if (!visited[child]) {
              int new_dist = distance[root] + cost[root][child];
              if (new_dist < distance[child]) {
                distance[child] = new_dist;
              }
            }
          }
          visited[root] = 1;
          root = find_small_(visited, distance);
        }
        int smallest_dist_server_id = inf;
        int smallest_dist = inf;
        for (int server_id : server_ids) {
          if (distance[server_id] < smallest_dist) {
            smallest_dist_server_id = server_id;
            smallest_dist = distance[server_id];
          }
        }
        string clientip = id_ip[client_id];
        if (smallest_dist_server_id != inf) {
          string serverip = id_ip[smallest_dist_server_id];
          clientip_serverip_[clientip] = serverip;
        }
        else {
          string serverip = id_ip[server_ids[0]];
          clientip_serverip_[clientip] = serverip;
        }
      }
      servers_file_->close();
    }
  }
}

void NameServer::Start() {
  while (true) {
    struct sockaddr_in client_addr;
    socklen_t length = (socklen_t)sizeof(client_addr);
    // accept a connection
    int client_sock = accept(socket_, (struct sockaddr *)&client_addr, (unsigned int *)&length);
    if (client_sock == -1) {
      cerr << "Error: bad accept\n";
      continue;
    }
    while (true) {
      // buffer to get step 2 header size
      uint32_t s2_header_size;
      // buffer to get step 2 question size
      uint32_t s2_question_size;
      // recv step 2 header size from the connection
      int num_recvd = recv(client_sock, &s2_header_size, sizeof(uint32_t), MSG_NOSIGNAL);
      if (num_recvd == -1) {
        break;
      }
      else if (num_recvd == 0) {
        break;
      }
      s2_header_size = ntohl(s2_header_size);
      // buffer to get step 2 header
      char s2_header_b[s2_header_size];
      // recv step 2 header from connection
      num_recvd = recv(client_sock, s2_header_b, s2_header_size, MSG_NOSIGNAL);
      string s2_header_string(s2_header_b, s2_header_size);
      // decode s2 header
      struct DNSHeader s2_header = DNSHeader::decode(s2_header_string);

      // recv step 2 question size from connection
      num_recvd = recv(client_sock, &s2_question_size, sizeof(uint32_t), MSG_NOSIGNAL);
      s2_question_size = ntohl(s2_question_size);
      //buffer to get step 2 question
      char s2_question_b[s2_question_size];
      // recv step2 question from connection
      num_recvd = recv(client_sock, s2_question_b, s2_question_size, MSG_NOSIGNAL);
      string s2_question_string(s2_question_b, s2_question_size);
      struct DNSQuestion s2_question = DNSQuestion::decode(s2_question_string);

      // init response header
      struct DNSHeader s3_header;
      SetHeader_(&s3_header, &s2_header);
      if (s2_question.QNAME != string("video.cse.umich.edu")) {
        s3_header.RCODE = 3;
      }
      string s3_header_str = DNSHeader::encode(s3_header);
      char* s3_header_cstr = &s3_header_str[0];
      uint32_t s3_header_size = (uint32_t) s3_header_str.size();
      uint32_t s3_header_size_n = htonl(s3_header_size);
      // init response body
      struct DNSRecord s3_record;
      string domain_name("video.cse.umich.edu");
      memcpy(&s3_record.NAME, &domain_name[0], domain_name.size());
      s3_record.TYPE = 1;
      s3_record.CLASS = 1;
      s3_record.TTL = 0;
      // decide what record to use
      string response_data = GetResponseData_(&client_addr);
      memcpy(&s3_record.RDATA, &response_data[0], response_data.size());
      s3_record.RDLENGTH = response_data.size();
      string s3_record_str = DNSRecord::encode(s3_record);
      char *s3_record_cstr = &s3_record_str[0];
      uint32_t s3_record_size = (uint32_t) s3_record_str.size();
      uint32_t s3_record_size_n = htonl(s3_record_size);

      // send header size
      int a = send(client_sock, &s3_header_size_n, sizeof(int), 0);
      // send header
      int b = send(client_sock, s3_header_cstr, s3_header_size, 0);
      // send record size
      int c = send(client_sock, &s3_record_size_n, sizeof(int), 0);
      // send record
      int d = send(client_sock, s3_record_cstr, s3_record_size, 0);
      // logging
      if (log_file_->is_open()) {
        *log_file_ << inet_ntoa(client_addr.sin_addr) << " ";
        *log_file_ << s2_question.QNAME << " ";
        *log_file_ << response_data << endl;
      }
    }
    close(client_sock);
  }
}

void NameServer::SetHeader_(DNSHeader* header, DNSHeader* query_header) {
  memset(header,0,sizeof(*header));
  header->ID = query_header->ID;
  header->QR = 1;
  header->OPCODE = query_header->OPCODE;
  header->AA = 1;
  header->RD = 0;
  header->RA = 0;
  header->Z = 0;
  header->NSCOUNT = 0;
  header->ARCOUNT = 0;
  header->ANCOUNT = 1;
  header->QDCOUNT = 0;
}

string NameServer::GetResponseData_(struct sockaddr_in* client_addr) {
  if (opt_ == 0) {
    string result = rr_list_[cur_rr_idx_ % rr_list_.size()];
    cur_rr_idx_++;
    return result;
  }
  else if (opt_ == 1) {
    string clientip(inet_ntoa(client_addr->sin_addr));
    return clientip_serverip_[clientip];
    // string a ("10.0.0.1");
    // return a;
  }
}

NameServer::NameServer(int port, ifstream* servers_file, ofstream* log_file, int opt) :
 socket_(-1), port_(port), servers_file_(servers_file), log_file_(log_file), opt_(opt) { }

NameServer::~NameServer() {
  if (socket_ != -1) {
    close(socket_);
    log_file_->close();
  }
}
