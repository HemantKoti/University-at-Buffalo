#include "../include/simulator.h"
#include <stdlib.h>

#include <iostream>
#include <queue>

#define PAYLOAD_SIZE 20
#define TIMEOUT 30.0
#define DEFAULT_ACK_NUM 999
#define A 0
#define B 1

using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

 This code should be used for PA2, unidirectional data transfer
 protocols (from A to B). Network properties:
 - one way network delay averages five time units (longer if there
 are other messages in the channel for GBN), but can be larger
 - packets can be corrupted (either the header or the data portion)
 or lost, according to user-defined probabilities
 - packets will be delivered in the order in which they were sent
 (although some can be lost).
 **********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Structures and Global Variables */
struct pkt *packets_to_send;

// Last transmitted packet index in the window
// Packet index for which we are waiting an acknowledgment to be received
int last_transmitted_A = 0, window_start_A = 0;

// Sequence numbers
int sequence_number_A = 0, sequence_number_B = 0;

// Window size and number of packets in A's window
int window_size = 0, packets_in_window = 0;

// Buffer messages
queue<msg> buffer;

/* Utility Functions */
int getPacketChecksum(struct pkt *packet);
int getPacketChecksum(struct pkt *packet) {
	int checksum = 0;

	if (packet == NULL)
		return checksum;

	for (int i = 0; i < PAYLOAD_SIZE; i++)
		checksum += (unsigned char) packet->payload[i];

	checksum += packet->seqnum;
	checksum += packet->acknum;
	cout << "GBN getPacketChecksum(): " << checksum << endl;
	return checksum;
}

/* Called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	cout << "GBN A_output" << endl;

	buffer.push(message);

	if (packets_in_window == window_size)
		return;

	struct msg m;
	if (!buffer.empty()) {
		m = buffer.front();
		cout << "Message to be processed: " << m.data << endl;
		buffer.pop();
	} else {
		cerr << "Empty buffer queue. No messages to process" << endl;
		return;
	}

	if (((last_transmitted_A + 1) % window_size) == window_start_A)
		return;
	else
		// Increment last_transmitted_A, if there are packets in the window
		if (packets_in_window != 0)
			last_transmitted_A = (last_transmitted_A + 1) % window_size;

	for (int i = 0; i < PAYLOAD_SIZE; i++)
		packets_to_send[last_transmitted_A].payload[i] = m.data[i];

	packets_to_send[last_transmitted_A].seqnum = sequence_number_A;
	packets_to_send[last_transmitted_A].acknum = DEFAULT_ACK_NUM;
	packets_to_send[last_transmitted_A].checksum = getPacketChecksum(&packets_to_send[last_transmitted_A]);

	// Update the sequence number and the number of packets in window
	sequence_number_A++;
	packets_in_window++;

	cout << "Sending packet from A with sequence number: " << packets_to_send[last_transmitted_A].seqnum << " to layer 3" << endl;
	tolayer3(A, packets_to_send[last_transmitted_A]);

	// Start the timer when the window is full
	if (window_start_A == last_transmitted_A)
		starttimer(A, TIMEOUT);
}

/* Called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	cout << "GBN A_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	if (packet.acknum != packets_to_send[window_start_A].seqnum) {
		cerr << "Expected acknowledgment: " << packets_to_send[window_start_A].seqnum
				<< ", received  acknowledgment: " << packet.acknum << endl;
		return;
	}

	// Set the sequence number of the packet to be acknowledged to -1
	packets_to_send[window_start_A].seqnum = -1;
	stoptimer(A);
	packets_in_window--;

	if (packets_in_window == 0) {
		if (!buffer.empty()) {
			struct msg m = buffer.front();
			cout << "Message to be processed: " << m.data << endl;
			buffer.pop();

			for (int i = 0; i < PAYLOAD_SIZE; i++)
				packets_to_send[last_transmitted_A].payload[i] = m.data[i];

			cout << "Sending packet from A with sequence number: " << sequence_number_A << " to layer 3" << endl;

			packets_to_send[last_transmitted_A].seqnum = sequence_number_A;
			packets_to_send[last_transmitted_A].acknum = DEFAULT_ACK_NUM;
			packets_to_send[last_transmitted_A].checksum = getPacketChecksum(&packets_to_send[last_transmitted_A]);

			// Update the sequence number and the number of packets in window
			sequence_number_A++;
			packets_in_window++;

			tolayer3(A, packets_to_send[last_transmitted_A]);
			starttimer(A, TIMEOUT);
		}
		else cerr << "Empty buffer queue. No messages to process" << endl;
	}
	else {
		// Increment the window_start_A, if there are packets in the window
		window_start_A = (window_start_A + 1) % window_size;

		if (!buffer.empty()) {
			struct msg m = buffer.front();
			cout << "Message to be processed: " << m.data << endl;
			buffer.pop();

			last_transmitted_A = (last_transmitted_A + 1) % window_size;

			for (int i = 0; i < PAYLOAD_SIZE; i++)
				packets_to_send[last_transmitted_A].payload[i] = m.data[i];

			cout << "Sending packet from A with sequence number: " << sequence_number_A << " to layer 3" << endl;

			packets_to_send[last_transmitted_A].seqnum = sequence_number_A;
			packets_to_send[last_transmitted_A].acknum = DEFAULT_ACK_NUM;
			packets_to_send[last_transmitted_A].checksum = getPacketChecksum(&packets_to_send[last_transmitted_A]);

			// Update  the sequence number and the number of packets in window
			sequence_number_A++;
			packets_in_window++;

			tolayer3(A, packets_to_send[last_transmitted_A]);
		}
		else cerr << "Empty buffer queue. No messages to process" << endl;
	}

	// If there are packets remaining in the window, restart the timer
	if (window_start_A != last_transmitted_A || packets_in_window == 1)
		starttimer(A, TIMEOUT);
}

/* Called when A's timer goes off */
void A_timerinterrupt() {
	cout << "GBN A_timerinterrupt" << endl;

	int temp_index = window_start_A;
	cout << "Expecting acknowledgment: " << packets_to_send[window_start_A].seqnum << endl;
	while (temp_index != last_transmitted_A) {
		cout << "Sending sequence number: " << packets_to_send[temp_index].seqnum << " to layer 3" << endl;
		tolayer3(A, packets_to_send[temp_index]);
		temp_index = (temp_index + 1) % window_size;
	}

	cout << "Sending sequence number: " << packets_to_send[temp_index].seqnum << " to layer 3" << endl;
	tolayer3(A, packets_to_send[temp_index]);

	// If there are packets remaining in the window, restart the timer
	if (window_start_A != last_transmitted_A || packets_in_window == 1)
		starttimer(A, TIMEOUT);
}

/* The following routine will be called once (only) before any other */
/* Entity A routines are called. You can use it to do any initialization */
void A_init() {
	cout << "GBN A_init" << endl;
	window_size = getwinsize();
	packets_to_send = (struct pkt *) malloc(window_size * sizeof(struct pkt));
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* Called from layer 3, when a packet arrives for layer 4 at B */
void B_input(struct pkt packet) {
	cout << "GBN B_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	cout << "Expected sequence number: " << sequence_number_B << endl;
	if (packet.seqnum == sequence_number_B) {
		cout << "Expected packet received, sending it to layer 5" << endl;
		sequence_number_B++;
		tolayer5(B, packet.payload);
	}
	else {
		cout << "Received out of order packet with sequence number: " << packet.seqnum << endl;
		// Send cumulative acknowledgment
		if (packet.seqnum < sequence_number_B) {
			cout << "Sending acknowledgment number: " << packet.seqnum << " to layer 3" << endl;
			packet.acknum = packet.seqnum;
			packet.checksum = getPacketChecksum(&packet);
			tolayer3(B, packet);
		}
	}
}

/* The following routine will be called once (only) before any other */
/* Entity B routines are called. You can use it to do any initialization */
void B_init() {
	cout << "GBN B_init" << endl;
}
