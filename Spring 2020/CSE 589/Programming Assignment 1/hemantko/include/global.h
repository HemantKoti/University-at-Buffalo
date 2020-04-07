#ifndef GLOBAL_H_
#define GLOBAL_H_

// Include references
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <map>
#include <utility>

// Preprocessor constants
#define HOSTNAME_LEN 128
#define PATH_LEN 256

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 512
#define BUFFER_SIZE 256
#define MSG_SIZE 256
#define MAX_COMMANDS 3
#define MAX_HOST_SIZE 256
#define MAX_PORT_SIZE 10

using namespace std;

// Classes/Structures
class ClientList {
public:
	int fdsocket;
	string hostname;
	string ip_addr;
	int portnum;
	int messagesReceived = 0;
	int messagesSent = 0;
	bool isLoggedOut = false;
	bool operator==(string rhs) const {
		return this->hostname == rhs || this->ip_addr == rhs;
	}
	bool operator==(const int rhs) const {
		return this->fdsocket == rhs;
	}
	string toString() {
		string str(to_string(this->fdsocket) + "," + this->hostname + "," +
					this->ip_addr + "," + to_string(this->portnum));
		return str;
	}
};

class BufferMessages {
public:
	string ip_dest;
	map <string, vector<string>> ip_msg;
	bool operator==(string ip) const {
		return this->ip_dest == ip;
	}
};

// Constants for client and server
const char cmd_author[] = "AUTHOR";
const char cmd_ip[] = "IP";
const char cmd_port[] = "PORT";
const char cmd_list[] = "LIST";
const char ubitname[] = "hemantko";

const char cmd_login[] = "LOGIN";
const char cmd_refresh[] = "REFRESH";
const char cmd_send[] = "SEND";
const char cmd_broadcast[] = "BROADCAST";
const char cmd_block[] = "BLOCK";
const char cmd_unblock[] = "UNBLOCK";
const char cmd_logout[] = "LOGOUT";
const char cmd_exit[] = "EXIT";
const char cmd_received[] = "RECEIVED";

const char cmd_statistics[] = "STATISTICS";
const char cmd_blocked[] = "BLOCKED";
const char cmd_relayed[] = "RELAYED";

const char clientListDelim[] = ",";
const char newLineDelim[] = "\n";
const char commandDelim[] = " ";
const char bufferDelim[] = "|";

// Functions
int server(char *port);
int client(char *port);
vector<string> splitString(char *str, const char *delims, bool fromCmdMenu /* = false */);
int connect_to_host(char *server_ip, char *server_port);
string serializeClientList(vector<ClientList>& clientlist);
vector<ClientList> deSerializeClientList(char message[]);
int sendAll(int &sockfd, char *buf, int *len);
char* str_to_char(string str);
void printError(char *error);
void sendListToClients(int &fdaccept, vector<ClientList> clientList);
bool validateIP(char *ip);
bool validatePort(char *port);

#endif
