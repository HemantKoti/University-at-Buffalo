/**
 * @hemantko_assignment1
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
 * This contains the main function.
 */

#include "../include/global.h"
#include "../include/logger.h"

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv) {
	// Initialize Logger
	cse4589_init_log(argv[2]);

	// Clear LOGFILE
	fclose(fopen(LOGFILE, "a"));

	// Start Here

	fstream file;
    file.open("/tmp/debug_pa1_hemantko.txt", ios::out);
	string line;

	// Backup streambuffers of  cout
	streambuf* stream_buffer_cin = cin.rdbuf();
	streambuf* stream_buffer_cout = cout.rdbuf();
	streambuf* stream_buffer_cerr = cerr.rdbuf();

	// Get the streambuffer of the file
	streambuf* stream_buffer_file = file.rdbuf();

	// Redirect cout to file
	// cin.rdbuf(stream_buffer_file);
	// cout.rdbuf(stream_buffer_file);
	// cerr.rdbuf(stream_buffer_file);

	// Redirect cout to stdin
	cin.rdbuf(stream_buffer_file);
	cout.rdbuf(stream_buffer_cout);
	cerr.rdbuf(stream_buffer_cerr);

	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " [c/s] [port]" << endl;
		exit(-1);
	}

	char ip[MAX_PORT_SIZE];
	strcpy(ip, argv[2]);
	if(!validatePort(ip)) {
		printError("Invalid Port Number");
		exit(-1);
	}

	const char server_stdin[] = "s";
	try {
		if (strcmp(argv[1], server_stdin) == 0)
			server(argv[2]);
		else
			client(argv[2]);
	}
	catch (exception &e) {
		cerr << "Unhandled exception: " << e.what() << endl;
	}

	file.close();
	return EXIT_SUCCESS;
}
