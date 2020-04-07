/**
 * @server
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
 * Contains functions that are handled by the server
 * This file contains the server init and main while loop for that application.
 * Uses the select() API to multiplex between network I/O and STDIN.
 */

#include "../include/logger.h"
#include "../include/global.h"

// Global Variables
vector<ClientList> clientList;
map<string, vector<string>> blockedMap;
vector<BufferMessages> bufferList;

/**
 * Sends the updated client list to all the clients connected
 *
 * @param Socket
 * @param Client List
 */
void sendListToClients(int &fdaccept, vector<ClientList> clientList = clientList) {
	cout << "Enter sendListToClient()" << endl;

	string cmdListStr(cmd_list);
	string buffer(cmdListStr + "|" + serializeClientList(clientList));
	int msg_len = buffer.size();
	if (sendAll(fdaccept, str_to_char(buffer), &msg_len) == 0)
		cout << "Actual length = " << buffer.size() << "| Sent Length = "
				<< msg_len << "| Message = " << buffer << endl;
	else
		cerr << "Error while sending list to the client" << endl;

	cout << "Exit sendListToClient()" << endl;
}

/**
 * Adds all the new logged-in clients to a master list
 *
 * @param Socket
 * @param Client Structure
 * @param Address length
 */
void addClientToList(int &fdaccept, struct sockaddr_in &client_addr, int &caddr_len) {
	cout << "Enter addClientToList()" << endl;

	char hostname[MAX_HOST_SIZE];

	if (getnameinfo((struct sockaddr*) &client_addr, caddr_len, hostname, MAX_HOST_SIZE, NULL, NULL, 0) != 0)
		cerr << "Failure at getnameinfo(). Cannot retrieve host name" << endl;

	vector<ClientList>::iterator it = find(clientList.begin(), clientList.end(), hostname);
	if (it == clientList.end()) {
		ClientList client;
		client.fdsocket = fdaccept;
		client.hostname = hostname;
		client.ip_addr = inet_ntoa(client_addr.sin_addr);
		client.portnum = ntohs(client_addr.sin_port);
		clientList.push_back(client);

		sendListToClients(fdaccept);
	} else if (it != clientList.end() && it->isLoggedOut){
		it->fdsocket = fdaccept;
		it->hostname = hostname;
		it->ip_addr = inet_ntoa(client_addr.sin_addr);
		it->portnum = ntohs(client_addr.sin_port);
		it->isLoggedOut = false;

		sendListToClients(fdaccept);

		// Send all buffered messages to this destination
		vector<BufferMessages>::iterator bufferItr = find(bufferList.begin(), bufferList.end(), it->ip_addr);
		for (auto const& messages : bufferItr->ip_msg)
		{
			// Iterate IP addresses of the map
			vector<ClientList>::iterator itsrc = find(clientList.begin(), clientList.end(), messages.first);

			// Iterate messages sent by each IP
		    for (auto message : messages.second) {
		    	int msg_len = message.size();
		    	if (sendAll(it->fdsocket, str_to_char(message), &msg_len) == 0) {
					cout << "Actual length = " << message.size() << "| Sent Length = "
							<< msg_len << "| Message = " << message.c_str() << endl;

					// Successful
					cse4589_print_and_log("[%s:SUCCESS]\n", cmd_relayed);
					cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",
							itsrc->ip_addr.c_str(), it->ip_addr.c_str(), message.c_str());
					cse4589_print_and_log("[%s:END]\n", cmd_relayed);

					itsrc->messagesSent++;
					it->messagesReceived++;
		    	}
		    }
		}
		bufferList.erase(bufferItr);
	}

	cout << "Exit addClientToList()" << endl;
}

/**
 * Checks if the client IP is blocked
 *
 * @param Sender IP
 * @param Receiver IP
 */
bool isBlocked(string sender, string receiver) {
	return blockedMap.find(receiver) != blockedMap.end() &&
		find(blockedMap[receiver].begin(), blockedMap[receiver].end(), sender) != blockedMap[receiver].end();
}

/**
 * Processes all the incoming client messages
 *
 * @param Socket
 * @param Buffer String
 */
void processClientMessages(int &fdaccept, char *buffer) {
	cout << "Enter processClientMessages()" << endl;

	string message;
	vector<string> commands = splitString(buffer, bufferDelim, false);
	vector<ClientList>::iterator itsrc = find(clientList.begin(), clientList.end(), fdaccept);
	vector<BufferMessages>::iterator bufferItr = find(bufferList.begin(), bufferList.end(), itsrc->ip_addr);

	// LIST
	if (strcmp(commands[0].c_str(), cmd_list) == 0)
		clientList = deSerializeClientList(str_to_char(commands[1]));
	// REFRESH
	if (strcmp(commands[0].c_str(), cmd_refresh) == 0)
		sendListToClients(fdaccept);
	// SEND
	else if (strcmp(commands[0].c_str(), cmd_send) == 0) {
		string cmdReceivedStr(cmd_received);
		vector<ClientList>::iterator itdest = find(clientList.begin(),
				clientList.end(), commands[1].c_str());

		message = cmdReceivedStr + "|" + itsrc->ip_addr + "|" + commands[2];
		int msg_len = message.size();

		if (itdest != clientList.end() && !isBlocked(itsrc->ip_addr, commands[1])) {
			if (itdest->isLoggedOut) {
				cout << "Buffer messages for: " << itdest->ip_addr << endl;
				vector<BufferMessages>::iterator bufferItr = find(bufferList.begin(), bufferList.end(), itdest->ip_addr);
				if(bufferItr != bufferList.end()) {
					if (bufferItr->ip_msg.find(itsrc->ip_addr) == bufferItr->ip_msg.end())
						bufferItr->ip_msg.insert (pair<string,vector<string>> (itsrc->ip_addr, { commands[2] }));
					else
						bufferItr->ip_msg[itsrc->ip_addr].push_back(commands[2]);
				}
			}
			else if (sendAll(itdest->fdsocket, str_to_char(message), &msg_len) == 0) {
				cout << "Actual length = " << message.size()
						<< "| Sent Length = " << msg_len << "| Message = "
						<< message << endl;

				// Successful
				cse4589_print_and_log("[%s:SUCCESS]\n", cmd_relayed);
				cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",
						itsrc->ip_addr.c_str(), commands[1].c_str(), commands[2].c_str());
				cse4589_print_and_log("[%s:END]\n", cmd_relayed);

				itsrc->messagesSent++;
				itdest->messagesReceived++;
			}
			else
				cerr << "Error while sending message to the client: "
						<< itdest->ip_addr.c_str()  << endl;
		}
		else cerr << "Client blocked or no client found with IP: " << commands[1].c_str() << endl;
	}
	// BROADCAST
	else if (strcmp(commands[0].c_str(), cmd_broadcast) == 0) {
		string cmdReceivedStr(cmd_received);

		message = cmdReceivedStr + "|" + itsrc->ip_addr + "|" + commands[2];
		int msg_len = message.size();

		for (vector<ClientList>::iterator itdest = clientList.begin(); (itdest != clientList.end()) &&
			(itdest != itsrc) && !isBlocked(itsrc->ip_addr, itdest->ip_addr); ++itdest) {

			if (itdest->isLoggedOut) {
				cout << "Buffer messages for: " << itdest->ip_addr << endl;
				vector<BufferMessages>::iterator bufferItr = find(bufferList.begin(), bufferList.end(), itdest->ip_addr);
				if(bufferItr != bufferList.end()) {
					if (bufferItr->ip_msg.find(itsrc->ip_addr) == bufferItr->ip_msg.end())
						bufferItr->ip_msg.insert (pair<string,vector<string>> (itsrc->ip_addr, { commands[2] }));
					else
						bufferItr->ip_msg[itsrc->ip_addr].push_back(commands[2]);
				}
			}
			else if (sendAll(itdest->fdsocket, str_to_char(message), &msg_len) == 0) {
				cout << "Actual length = " << message.size() << "| Sent Length = " << msg_len <<
						"| Message = " << message << endl;

				// Successful
				cse4589_print_and_log("[%s:SUCCESS]\n", cmd_relayed);
				cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",
						itsrc->ip_addr.c_str(), commands[1].c_str(), commands[2].c_str());
				cse4589_print_and_log("[%s:END]\n", cmd_relayed);

				itsrc->messagesSent++;
				itdest->messagesReceived++;
			}
			else
				cerr << "Error while sending message to the client: "
						<< itdest->ip_addr.c_str() << endl;
		}
	}
	// BLOCK
	else if (strcmp(commands[0].c_str(), cmd_block) == 0) {
		cout << "The client with IP: " << itsrc->ip_addr.c_str() << " blocked: " << commands[1].c_str() << endl;
		if (blockedMap.find(itsrc->ip_addr) == blockedMap.end())
			blockedMap.insert (pair<string,vector<string>> (itsrc->ip_addr, { commands[1] }));
		else
			blockedMap[itsrc->ip_addr].push_back(commands[1]);
	}
	// UNBLOCK
	else if (strcmp(commands[0].c_str(), cmd_unblock) == 0) {
		cout << "The client with IP: " << itsrc->ip_addr.c_str() << " unblocked: " << commands[1].c_str() << endl;
		blockedMap[itsrc->ip_addr].erase(remove(blockedMap[itsrc->ip_addr].begin(),
				blockedMap[itsrc->ip_addr].end(), commands[1]), blockedMap[itsrc->ip_addr].end());
	}
	// LOGOUT
	else if (strcmp(commands[0].c_str(), cmd_logout) == 0) {
		cout << "The client: " << itsrc->hostname.c_str() << " logged out from server" << endl;
		itsrc->isLoggedOut = true;

		// Buffer messages sent to this destination until next login
		if (bufferItr == bufferList.end()) {
			BufferMessages bufMsg;
			bufMsg.ip_dest = itsrc->ip_addr;
			bufferList.push_back(bufMsg);
		}
	}
	// EXIT
	else if (strcmp(commands[0].c_str(), cmd_exit) == 0) {
		cout << "The client: " << itsrc->hostname.c_str() << " exited the system. Destroying it's state" << endl;
		itsrc->isLoggedOut = true;

		if (blockedMap.find(itsrc->ip_addr) != blockedMap.end())
			blockedMap.erase(itsrc->ip_addr);

		bufferList.erase(bufferItr);
		clientList.erase(itsrc);
	}

	cout << "Exit processClientMessages()" << endl;
}

/**
 * Server function
 *
 * @param Port number on which the server is listening
 * @references: https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
 */
int server(char *port) {
	cout << "Enter server()" << endl;

	int server_socket, head_socket, selret, sock_index, fdaccept = 0, caddr_len;
	struct sockaddr_in client_addr;
	struct addrinfo hints, *res;
	fd_set master_list, watch_list;

	// Set up hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// Fill up address structures
	if (getaddrinfo(NULL, port, &hints, &res) != 0)
		cerr << "getaddrinfo failed" << endl;

	// Socket
	server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_socket < 0)
		cerr << "Cannot create socket" << endl;

	// Bind
	if (bind(server_socket, res->ai_addr, res->ai_addrlen) < 0)
		cerr << "Bind failed" << endl;

	freeaddrinfo(res);

	// Listen
	if (listen(server_socket, BACKLOG) < 0)
		cerr << "Unable to listen on port" << endl;

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

	// Register the listening socket
	FD_SET(server_socket, &master_list);
	// Register STDIN
	FD_SET(STDIN, &master_list);

	head_socket = server_socket;

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

						if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
						{
							cerr << "Cannot read command from standard input" << endl;
							exit(-1);
						}

						cout << "COMMAND: " << cmd << endl;
						vector<string> commands = splitString(cmd, commandDelim,true);
						if(commands.size() == 0) {
							cerr << "No commands entered" << endl;
							continue;
						}

						// Process PA1 commands here ...
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
							sort(clientList.begin(), clientList.end(),[](const ClientList& lhs, const ClientList& rhs)
							{
							    return lhs.portnum < rhs.portnum;
							});

							if (clientList.size() > 0)
								cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());

							int i = 0;
							for (vector<ClientList>::iterator it = clientList.begin(); it != clientList.end()
								&& !it->isLoggedOut; ++it) {
								cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", (i + 1), it->hostname.c_str(),
										it->ip_addr.c_str(), it->portnum);
								i++;
							}

							if (clientList.size() > 0)
								cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// STATISTICS
						else if (strcmp(commands[0].c_str(), cmd_statistics) == 0) {
							sort(clientList.begin(), clientList.end(),[](const ClientList& lhs, const ClientList& rhs)
							{
								return lhs.portnum < rhs.portnum;
							});

							if (clientList.size() > 0)
								cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());

							int i = 0;
							for (vector<ClientList>::iterator it = clientList.begin(); it != clientList.end(); ++it) {
								cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", (i + 1), it->hostname.c_str(),
										it->messagesSent, it->messagesReceived,
										it->isLoggedOut ? "logged-out" : "logged-in");
								i++;
							}

							if (clientList.size() > 0)
								cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
						}
						// BLOCKED
						else if (strcmp(commands[0].c_str(), cmd_blocked) == 0) {
							if(validateIP(str_to_char(commands[1])) &&
								blockedMap.find(commands[1]) != blockedMap.end()) {
								sort(clientList.begin(), clientList.end(),[](const ClientList& lhs, const ClientList& rhs)
								{
									return lhs.portnum < rhs.portnum;
								});

								if (blockedMap[commands[1]].size() > 0)
									cse4589_print_and_log("[%s:SUCCESS]\n", commands[0].c_str());

								size_t i = 0;
								for (vector<string>::iterator itbl = blockedMap[commands[1]].begin();
										itbl != blockedMap[commands[1]].end(); ++itbl) {
									vector<ClientList>::iterator itcl = find(clientList.begin(), clientList.end(), (*itbl).c_str());

									if (itcl != clientList.end())
										cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", (i + 1), itcl->hostname.c_str(), itcl->ip_addr.c_str(), itcl->portnum);

									i++;
								}

								if (blockedMap[commands[1]].size() > 0)
									cse4589_print_and_log("[%s:END]\n", commands[0].c_str());
							}
							else // Error
								printError(str_to_char(commands[0]));
						}

						free(cmd);
					}
					// Check if new client is requesting connection
					else if (sock_index == server_socket) {
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr*) &client_addr, (socklen_t*) &caddr_len);
						if (fdaccept < 0)
							cerr << "Accept failed" << endl;

						cout << "Remote Host connected" << endl;

						// Add new client to the list
						addClientToList(fdaccept, client_addr, caddr_len);

						// Add to watched socket list
						FD_SET(fdaccept, &master_list);
						if (fdaccept > head_socket)
							head_socket = fdaccept;
					}
					// Read from existing clients
					else {
						cout << "Attempting to read messages from clients" << endl;

						/* Initialize buffer to receive response */
						char *buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE * 32);
						memset(buffer, '\0', BUFFER_SIZE * 32);

						if (recv(sock_index, buffer, BUFFER_SIZE * 32, 0) <= 0){
							 close(sock_index);
							 cout << "Remote Host terminated connection" << endl;

							 // Remove from watched list
							 FD_CLR(sock_index, &master_list);
						}
						else {
							cout << "Client sent me: " << buffer <<
									" | Size: " << strlen(buffer) <<  endl << flush;

							// Process specific commands from the client
							processClientMessages(sock_index, buffer);
						}

						free(buffer);
					}
				}
			}
		}
	}

	cout << "Exit server()" << endl;
	return 1;
}
