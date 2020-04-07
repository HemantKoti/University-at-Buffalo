/**
 * @utils
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
 *
 * This file contains utilities required by all other classes.
 */

#include "../include/logger.h"
#include "../include/global.h"

/**
 * Print all the error messages
 *
 * @param Error message
 */
void printError(char *errMsg) {
	cse4589_print_and_log("[%s:ERROR]\n", errMsg);
	cse4589_print_and_log("[%s:END]\n", errMsg);
}

/**
 * Split a given string by a token
 *
 * @param String
 * @param Delimiter
 */
vector<string> splitString(char *str, const char *delims, bool fromCmdMenu) {
	string strCopy(str);

	vector<string> result;
	if (str[strlen(str) - 1] == '\n')
		str[strlen(str) - 1] = 0;

	for (char *token = strtok(str, delims); token != NULL; token = strtok(NULL, delims)) {
		if (fromCmdMenu && strcmp(token, cmd_send) == 0) {
			result.push_back(token); // Send command

			token = strtok(NULL, delims);
			if (token != NULL)
				result.push_back(token); // Client IP

			token = strtok(NULL, delims);
			string temp = strCopy.substr(strCopy.find(token));
			if (temp.size())
				result.push_back(temp); // Message

			break;
		} else if (fromCmdMenu && strcmp(token, cmd_broadcast) == 0) {
			result.push_back(token); // Broadcast command

			token = strtok(NULL, delims);
			string temp = strCopy.substr(strCopy.find(token));
			if (temp.size())
				result.push_back(temp); // Message

			break;
		}

		result.push_back(token);
	}

	return result;
}

/**
 * Converts string to char* representation
 *
 * @param Strint
 */
char *str_to_char(string str) {
	// cout << "Enter str_to_char()" << endl;

	// Convert string to char* pointer
	char *writable = new char[str.size() + 1];
	copy(str.begin(), str.end(), writable);
	writable[str.size()] = '\0';

	// cout << "Exit str_to_char()" << endl;
	return writable;
}

/**
 * Serialize the client list
 *
 * @param Client List
 */
string serializeClientList(vector<ClientList> &clientlist) {
	cout << "Enter serializeClientList()" << endl;

	string buffer = "";
	for (size_t i = 0; i < clientlist.size(); i++)
		buffer += (clientlist[i].toString() + "\n");

	// cout << "Serialized string: " << buffer << endl;
	cout << "Exit serializeClientList()" << endl;
	return buffer;
}

/**
 * Deserialize the client list
 *
 * @param Client List
 */
vector<ClientList> deSerializeClientList(char *message) {
	cout << "Enter deSerializeClientList()" << endl;

	vector<ClientList> clientlist;
	vector<string> row = splitString(message, newLineDelim, false);
	for (size_t i = 0; i < row.size(); i++) {
		ClientList client;

		vector<string> col = splitString(str_to_char(row[i]), clientListDelim, false);
		client.fdsocket = stoi(col[0]);
		client.hostname = col[1];
		client.ip_addr = col[2];
		client.portnum = stoi(col[3]);

		// cout << "Client ToString(): " << client.toString() << endl;
		clientlist.push_back(client);
	}

	cout << "Exit deSerializeClientList()" << endl;
	return clientlist;
}

/**
 * Send all the messages stored in the buffer
 *
 * @param Socket
 * @param Buffer string
 * @param Length of the buffer string
 */
int sendAll(int &sockfd, char *buf, int *len) {
	cout << "Enter sendAll()" << endl;

	int total = 0;        // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;

	while (total < *len) {
		n = send(sockfd, buf + total, bytesleft, 0);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	cout << "Exit sendAll()" << endl << flush;
	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

/**
 * Vaildate an IP Address
 *
 * @param IP
 */
bool validateIP(char *ip) {
    struct sockaddr_in sa;
	return inet_pton(AF_INET, ip, &(sa.sin_addr)) > 0;
}

/**
 * Vaildate a port number
 *
 * @param Port
 */
bool validatePort(char *port) {
	try {
		stoi(port);
	} catch (invalid_argument &ia) {
		cerr << "Invalid argument error: " << ia.what() << endl;
		return false;
	} catch (out_of_range &oor) {
		cerr << "Out of Range error: " << oor.what() << endl;
		return false;
	}

	return true;
}
