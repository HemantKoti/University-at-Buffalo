/**
 * @client
 * @author  Hemant Koti <hemantko@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 * Handles all the operations required by a client
 *
 */

#include "../include/logger.h"
#include "../include/global.h"

vector<ClientList> clientlist;
vector<string> blockedclients;

/**
 * Process all the server messages received one by one
 *
 * @param Socket
 * @param Buffer length
 * @param Listening port
 */
void processServerMessages(int &fdaccept, char *buffer, char *hostname, int listeningPort) {
	cout << "Enter processServerMessages()" << endl;
	vector<string> commands = splitString(buffer, bufferDelim, false);

	if (strcmp(commands[0].c_str(), cmd_list) == 0) {
		clientlist = deSerializeClientList(str_to_char(commands[1]));
		vector<ClientList>::iterator it = find(clientlist.begin(), clientlist.end(), hostname);
		if (it != clientlist.end() && strcmp(it->hostname.c_str(), hostname) == 0 && it->portnum != listeningPort) {
			cout << "Incorrect port number sent by server. Updating client list and sending back again" << endl;
			it->portnum = listeningPort;
			sendListToClients(fdaccept, clientlist);
		}
	} else if (strcmp(commands[0].c_str(), cmd_received) == 0) {
		// Successful
		cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());
		cse4589_print_and_log("msg from:%s\n[msg]:%s\n", commands[1].c_str(), commands[2].c_str());
		cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
	}

	cout << "Exit processServerMessages()" << endl;
}

/**
 * Connects to the server
 *
 * @param Server IP
 * @param Server Port
 */
int connect_to_host(string server_ip, string server_port) {
	cout << "Enter connect_to_host()" << endl;

	int fdsocket = -1;
	struct addrinfo hints, *res;

	// Set up hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// Fill up address structures
	if (getaddrinfo(server_ip.c_str(), server_port.c_str(), &hints, &res) != 0) {
		cerr << "getaddrinfo failed" << endl;
		return -1;
	}

	// Socket
	fdsocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fdsocket < 0) {
		cerr << "Failed to create socket" << endl;
		return -1;
	}

	// Connect
	if (connect(fdsocket, res->ai_addr, res->ai_addrlen) < 0) {
		cerr << "Connect failed" << endl;
		return -1;
	}

	freeaddrinfo(res);

	cout << "Exit connect_to_host()" << endl;
	return fdsocket;
}

/**
 * Sends messages to a particular client
 *
 * @param Socket
 * @param Commands list
 */
bool sendMessageToClients(int &client_socket, vector<string> &commands) {
	cout << "Enter sendMessageToClients()" << endl;

	string message = "";
	if (strcmp(commands[0].c_str(), cmd_send) == 0) {
		if (!validateIP(str_to_char(commands[1]))) {
			cerr << "Invalid IP address" << endl;
			return false;
		}

		if(find(clientlist.begin(), clientlist.end(), commands[1].c_str()) == clientlist.end()) {
			cerr << "Client is not in the local logged in list" << endl;
			return false;
		}

		message = commands[0] + "|" + commands[1] + "|" + commands[2];
	} else {
		message = commands[0] + "|255.255.255.255|" + commands[1];
	}

	int msg_len = message.size();
	if (sendAll(client_socket, str_to_char(message), &msg_len) == 0)
		cout << "Actual length = " << message.size() << "| Sent Length = "
				<< msg_len << "| Message = " << message << endl << flush;
	else {
		cerr << "Failed to deliver message to the client" << endl;
		return false;
	}

	cout << "Exit sendMessageToClients()" << endl;
	return true;
}

/**
 * Client function
 *
 * @param Port number on which the client is listening
 * @references: https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
 */
int client(char *port) {
	cout << "Enter client()" << endl;

	int client_socket, head_socket, selret, sock_index;
	fd_set master_list, watch_list;

	/* ---------------------------------------------------------------------------- */

	struct hostent *host_entry;
	char hostname[MAX_HOST_SIZE];
	if (gethostname(hostname, sizeof(hostname)) == -1)
		cerr << "Cannot retrieve host name: " << hostname << endl;

	if ((host_entry = gethostbyname(hostname)) == NULL)
		cerr << "Cannot retrieve host entry" << endl;

	char *ip_addr = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
	cout << "Host Name: " << hostname
		 <<" IP Address: " << ip_addr << endl;

	/* ---------------------------------------------------------------------------- */

	// Zero select FD sets
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	// Register STDIN
	FD_SET(STDIN, &master_list);

	head_socket = STDIN;

	while (TRUE) {
		memcpy(&watch_list, &master_list, sizeof(master_list));

		// select() system call. This will BLOCK
		selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
		if (selret < 0)
			cerr << "select failed." << endl;

		// Check if we have sockets/STDIN to process
		if (selret > 0) {
			// Loop through socket descriptors to check which ones are ready
			for (sock_index = 0; sock_index <= head_socket; sock_index += 1) {

				if (FD_ISSET(sock_index, &watch_list)) {

					// Check if new command on STDIN
					if (sock_index == STDIN) {
						char *cmd = (char*) malloc(sizeof(char) * CMD_SIZE);

						memset(cmd, '\0', CMD_SIZE);
						if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
						{
							cerr << "Cannot read command from standard input" << endl;
							exit(-1);
						}

						cout << "COMMAND: " << cmd << endl;
						vector<string> commands = splitString(cmd, commandDelim, true);
						if(commands.size() == 0) {
							cerr << "No commands entered" << endl;
							continue;
						}

						// Process PA1 commands here
						// AUTHOR
						if (strcmp(commands[0].c_str(), cmd_author) == 0) {
							cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());
							cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubitname);
							cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// IP
						else if (strcmp(commands[0].c_str(), cmd_ip) == 0) {
							// Successful
							cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());
							cse4589_print_and_log("IP:%s\n", ip_addr);
							cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// PORT
						else if (strcmp(commands[0].c_str(), cmd_port) == 0) {
							// Successful
							cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());
							cse4589_print_and_log("PORT:%d\n", stoi(port));
							cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// LIST
						else if (strcmp(commands[0].c_str(), cmd_list) == 0) {
							sort(clientlist.begin(), clientlist.end(),[](const ClientList& lhs, const ClientList& rhs)
							{
							    return lhs.portnum < rhs.portnum;
							});

							if (clientlist.size() > 0)
								cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());

							int i = 0;
							for (vector<ClientList>::iterator it = clientlist.begin(); it != clientlist.end(); ++it) {
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", (i + 1), it->hostname.c_str(),
										it->ip_addr.c_str(), it->portnum);
								i++;
							}

							if (clientlist.size() > 0)
								cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// LOGIN
						else if (strcmp(commands[0].c_str(), cmd_login) == 0) {
							if (validateIP(str_to_char(commands[1])) && validatePort(str_to_char(commands[2]))
								&& ((client_socket = connect_to_host(commands[1], commands[2])) != -1)) {
								// Register the listening socket
								FD_SET(client_socket, &master_list);
								if (client_socket > head_socket)
									head_socket = client_socket;
							}
							else
								// Error
								printError(str_to_char(commands[0]));
						}
						// REFRESH
						else if (strcmp(commands[0].c_str(), cmd_refresh) == 0) {
							string message = commands[0];
							int msg_len = message.size();
							if (sendAll(client_socket, str_to_char(message), &msg_len) == 0) {
								cout << "Actual length = " << message.size()
										<< "| Sent Length = " << msg_len
										<< "| Message = " << message << endl << flush;
							}
							else
								// Error
								cerr << "Failed to refresh list from server" << endl;
						}
						// SEND or BROADCAST
						else if (strcmp(commands[0].c_str(), cmd_send) == 0 ||
								strcmp(commands[0].c_str(), cmd_broadcast) == 0) {
							if (!sendMessageToClients(client_socket, commands))
								printError(str_to_char(commands[0]));
						}
						// BLOCK
						else if (strcmp(commands[0].c_str(), cmd_block) == 0) {
							if(validateIP(str_to_char(commands[1])) &&
								find(clientlist.begin(), clientlist.end(), commands[1].c_str()) != clientlist.end() &&
								find(blockedclients.begin(), blockedclients.end(), commands[1]) == blockedclients.end()) {
								blockedclients.push_back(commands[1]);
								string message = commands[0] + "|" + commands[1];
								int msg_len = message.size();
								if (sendAll(client_socket, str_to_char(message), &msg_len) == 0) {
									cout << "Actual length = " << message.size()
											<< "| Sent Length = " << msg_len
											<< "| Message = " << message << endl << flush;
								}
								else
									// Error
									cerr << "Failed to send block list to the server" << endl;
							}
							else
								// Error
								printError(str_to_char(commands[0]));
						}
						// UNBLOCK
						else if (strcmp(commands[0].c_str(), cmd_unblock) == 0) {
							if(validateIP(str_to_char(commands[1])) &&
								find(blockedclients.begin(), blockedclients.end(), commands[1]) != blockedclients.end() &
								find(clientlist.begin(), clientlist.end(), commands[1].c_str()) != clientlist.end()) {

								blockedclients.erase(remove(blockedclients.begin(), blockedclients.end(), commands[1]), blockedclients.end());
								string message = commands[0] + "|" + commands[1];
								int msg_len = message.size();
								if (sendAll(client_socket, str_to_char(message), &msg_len) == 0) {
									cout << "Actual length = " << message.size()
											<< "| Sent Length = " << msg_len
											<< "| Message = " << message << endl << flush;
								}
								else
									// Error
									cerr << "Failed to send block list to the server" << endl;
						}
						else
							// Error
							printError(str_to_char(commands[0]));
						}
						// LOGOUT
						else if (strcmp(commands[0].c_str(), cmd_logout) == 0) {
							string message = commands[0];
							int msg_len = message.size();
							if (sendAll(client_socket, str_to_char(message), &msg_len) == 0) {
								cout << "Actual length = " << message.size()
										<< "| Sent Length = " << msg_len
										<< "| Message = " << message << endl << flush;
							}
							else
								// Error
								cerr << "Failed to logout from the server" << endl;
						}
						// EXIT
						else if (strcmp(commands[0].c_str(), cmd_exit) == 0) {
							string message = commands[0];
							int msg_len = message.size();
							if (sendAll(client_socket, str_to_char(message), &msg_len) == 0) {
								cout << "Actual length = " << message.size()
										<< "| Sent Length = " << msg_len
										<< "| Message = " << message << endl << flush;
							}
							else
								// Error
								cerr << "Failed to exit from the system" << endl;

							close(sock_index);
							clientlist.clear();
							blockedclients.clear();
							cout << "Exiting the system" << endl;
							FD_CLR(sock_index, &master_list);
							exit(EXIT_SUCCESS);
						}

						free(cmd);
					}
					// Read from an existing server
					else {
						cout << "Attempting to read messages from server" << endl;

						/* Initialize buffer to receive response */
						char *buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE * 32);
						memset(buffer, '\0', BUFFER_SIZE * 32);

						if(recv(sock_index, buffer, BUFFER_SIZE * 32, 0) <= 0){
							close(sock_index);
							cout << "Remote Host terminated connection" << endl;

							// Remove from watched list
							FD_CLR(sock_index, &master_list);
						}
						else {
							cout << "Server sent me: " << buffer <<
									" | Size: " << strlen(buffer) << endl << flush;

							// Process specific commands from the client
							processServerMessages(sock_index, buffer, hostname, stoi(port));
						}

						free(buffer);
					}
				}
			}
		}
	}

	cout << "Exit client()" << endl;
	return 1;
}
