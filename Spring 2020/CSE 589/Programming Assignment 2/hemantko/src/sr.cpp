#include "../include/simulator.h"
#include <stdlib.h>

#include <iostream>
#include <queue>

#define PAYLOAD_SIZE 20
#define TIMEOUT 30.0
#define LOGICAL_TIME_UNIT 1
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

/* Structures and Global Variables */
class SR {
public:
	struct pkt packet;
	int ack;
	int timeover;
};

SR *packets_in_A, *packets_in_B;

// Packet index for which we are waiting an acknowledgment to be received
int window_start_A = 0, window_start_B = 0;

// Last transmitted packet index in the window
int last_transmitted_A = 0, last_transmitted_B = 0;

// Sequence numbers
int sequence_number_A = 0, sequence_number_B = 0;

// Window size
int window_size = 0;

// Number of packets in A and B
int packets_in_window_A = 0, packets_in_window_B = 0;

// Buffer messages
queue<msg> buffer;

// Logical timer
bool start_timer = false;

// Current time
float current_time = 0;

/* Utility Functions */
int getPacketChecksum(struct pkt *p);
int getPacketChecksum(struct pkt *p) {
	int checksum = 0;

	if (p == NULL)
		return checksum;

	for (int i = 0; i < PAYLOAD_SIZE; i++)
		checksum += (unsigned char) p->payload[i];

	checksum += p->seqnum;
	checksum += p->acknum;
	cout << "SR getPacketChecksum(): " << checksum << endl;
	return checksum;
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	cout << "SR A_output" << endl;

	buffer.push(message);

	if (packets_in_window_A == window_size)
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
		if (packets_in_window_A != 0)
			last_transmitted_A = (last_transmitted_A + 1) % window_size;

	for (int i = 0; i < PAYLOAD_SIZE; i++)
		packets_in_A[last_transmitted_A].packet.payload[i] = m.data[i];

	packets_in_A[last_transmitted_A].packet.seqnum = sequence_number_A;
	packets_in_A[last_transmitted_A].packet.acknum = DEFAULT_ACK_NUM;
	packets_in_A[last_transmitted_A].packet.checksum = getPacketChecksum(&packets_in_A[last_transmitted_A].packet);

	// Set the acknowledgment to not received
	cout << "Time over in packet A: " << last_transmitted_A << " is: " << current_time + TIMEOUT << endl;
	packets_in_A[last_transmitted_A].timeover = current_time + TIMEOUT;
	packets_in_A[last_transmitted_A].ack = 0;

	// Update the sequence number and the number of packets in window
	sequence_number_A++;
	packets_in_window_A++;

	cout << "Sending packet from A with sequence number: " << packets_in_A[last_transmitted_A].packet.seqnum << " to layer 3" << endl;
	tolayer3(A, packets_in_A[last_transmitted_A].packet);

	// If the logical timer is off, switch it on and call starttimer()
	if (!start_timer) {
		start_timer = !start_timer;
		starttimer(A, LOGICAL_TIME_UNIT);
	}
}

/* Called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	cout << "SR A_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	cout << "Waiting for acknowledgment number: " << packets_in_A[window_start_A].packet.seqnum << endl;
	if (packet.acknum == packets_in_A[window_start_A].packet.seqnum) {
		cout << "Received packet with acknowledgment number: " << packets_in_A[window_start_A].packet.seqnum
			<< " and window start: " << window_start_A << endl;
		packets_in_A[window_start_A].ack = 1;
		packets_in_window_A--;

		if (packets_in_window_A == 0) {
			window_start_A = (window_start_A + 1) % window_size;
			last_transmitted_A = (last_transmitted_A + 1) % window_size;

			if (!buffer.empty()) {
				struct msg m = buffer.front();
				cout << "Message to be processed: " << m.data << endl;
				buffer.pop();

				for (int i = 0; i < PAYLOAD_SIZE; i++)
					packets_in_A[last_transmitted_A].packet.payload[i] = m.data[i];

				cout << "Sending packet from A with sequence number: " << sequence_number_A << " to layer 3" << endl;

				packets_in_A[last_transmitted_A].packet.seqnum = sequence_number_A;
				packets_in_A[last_transmitted_A].packet.acknum = DEFAULT_ACK_NUM;
				packets_in_A[last_transmitted_A].packet.checksum = getPacketChecksum(&packets_in_A[last_transmitted_A].packet);

				// Set acknowledgment to not received
				cout << "Time over in packet A: " << last_transmitted_A << " is: " << current_time + TIMEOUT << endl;
				packets_in_A[last_transmitted_A].timeover = current_time + TIMEOUT;
				packets_in_A[last_transmitted_A].ack = 0;

				// Update the number of packets in window
				sequence_number_A++;
				packets_in_window_A++;

				tolayer3(A, packets_in_A[last_transmitted_A].packet);
			} else {
				// If the logical timer is on, switch it off and call stoptimer()
				start_timer = false;
				stoptimer(A);
			}
		}
		else {
			int i = window_start_A;
			while (i != last_transmitted_A) {
				if (packets_in_A[(i + 1) % window_size].ack != 1)
					break;

				packets_in_window_A--;
				i = (i + 1) % window_size;
				if (i == last_transmitted_A)
					last_transmitted_A = i;
			}

			window_start_A = (i + 1) % window_size;
			if (packets_in_window_A == 0)
				last_transmitted_A = window_start_A;

			struct msg m;
			if (!buffer.empty()) {
				m = buffer.front();
				cout << "Message to be processed: " << m.data << endl;
				buffer.pop();
			} else {
				cerr << "Empty buffer queue. No messages to process" << endl;
				return;
			}

			for (int i = 0; i < PAYLOAD_SIZE; i++)
				packets_in_A[last_transmitted_A].packet.payload[i] = m.data[i];

			cout << "Sending packet from A with sequence number: " << sequence_number_A << "to layer 3" << endl;

			packets_in_A[last_transmitted_A].packet.seqnum = sequence_number_A;
			packets_in_A[last_transmitted_A].packet.acknum = DEFAULT_ACK_NUM;
			packets_in_A[last_transmitted_A].packet.checksum = getPacketChecksum(&packets_in_A[last_transmitted_A].packet);

			// Set acknowledgment to not received
			cout << "Time over in packet A: " << last_transmitted_A << " is: " << current_time + TIMEOUT << endl;
			packets_in_A[last_transmitted_A].timeover = current_time + TIMEOUT;
			packets_in_A[last_transmitted_A].ack = 0;

			// Update the number of packets in window
			sequence_number_A++;
			packets_in_window_A++;

			tolayer3(A, packets_in_A[last_transmitted_A].packet);
		}
	}

	// Duplicate acknowledgment
	else if (packet.acknum <= packets_in_A[window_start_A].packet.seqnum)
		cout << "Received old acknowledgment: " << packet.acknum << endl;

	// Receiving acknowledgments for future window packets
	else if (packet.acknum > packets_in_A[window_start_A].packet.seqnum) {
		int i = window_start_A;
		while (i != last_transmitted_A) {
			if (packet.acknum == packets_in_A[(i + 1) % window_size].packet.seqnum) {
				cout << "Received future acknowledgment: " << packets_in_A[(i + 1) % window_size].packet.seqnum << endl;
				packets_in_A[(i + 1) % window_size].ack = 1;
				break;
			}
			i = (i + 1) % window_size;
		}
	}
}

/* Called when A's timer goes off */
void A_timerinterrupt() {
	current_time = current_time + LOGICAL_TIME_UNIT;
	// cout << "SR A_timerinterrupt() current time: " << current_time <<  endl;

	if (packets_in_window_A != 0) {
		int i = window_start_A;
		while (i != last_transmitted_A) {
			if (packets_in_A[i].ack == 0 && packets_in_A[i].timeover < current_time) {
				cout << "Current time: " << current_time << " Time over: " << packets_in_A[i].timeover << endl;
				cout << "Sending packet with sequence number: " << packets_in_A[i].packet.seqnum << " to layer 3" << endl;
				packets_in_A[i].timeover = current_time + TIMEOUT;
				tolayer3(A, packets_in_A[i].packet);
			}
			i = (i + 1) % window_size;
		}

		if (packets_in_A[i].ack == 0 && packets_in_A[i].timeover < current_time) {
			cout << "Current time: " << current_time << " Time over: " << packets_in_A[i].timeover << endl;
			cout << "Sending packet with sequence number: " << packets_in_A[i].packet.seqnum << " to layer 3" << endl;
			packets_in_A[i].timeover = current_time + TIMEOUT;
			tolayer3(A, packets_in_A[window_start_A].packet);
		}
	}

	starttimer(A, LOGICAL_TIME_UNIT);
}

/* The following routine will be called once (only) before any other */
/* Entity A routines are called. You can use it to do any initialization */
void A_init() {
	cout << "SR A_init" << endl;

	window_size = getwinsize();
	packets_in_A = new SR[window_size];

	// Set all packets to unacknowledged
	for (int i = 0; i < window_size; i++)
		packets_in_A[i].ack == 0;

	start_timer = true;
	starttimer(A, LOGICAL_TIME_UNIT);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* Called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	cout << "SR B_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	cout << "Expected sequence number: " << sequence_number_B << endl;
	if (packet.seqnum == sequence_number_B) {
		cout << "Expected packet received, sending it to layer 5" << endl;
		tolayer5(B, packet.payload);

		cout << "Sending acknowledgment number: " << sequence_number_B << " to layer 3" << endl;
		packet.acknum = sequence_number_B;
		packet.checksum = getPacketChecksum(&packet);
		tolayer3(B, packet);

		sequence_number_B++;
		cout << "Time over in packet B: " << sequence_number_B << " is: " << sequence_number_B + window_size - 1 << endl;
		packets_in_B[window_start_B].timeover = sequence_number_B + window_size - 1;
		window_start_B = (window_start_B + 1) % window_size;

		while (packets_in_B[window_start_B].packet.seqnum == sequence_number_B) {
			cout << "Expected packet received, sending it to layer 5" << endl;
			tolayer5(B, packets_in_B[window_start_B].packet.payload);
			sequence_number_B++;
			packets_in_B[window_start_B].timeover = sequence_number_B + window_size - 1;
			window_start_B = (window_start_B + 1) % window_size;
		}
	}
	else {
		if (packet.seqnum > sequence_number_B) {
			cout << "Received sequence number: " << packet.seqnum << ". Acknowledge future window packets" << endl;
			if (packet.seqnum <= sequence_number_B + window_size) {
				for (int i = 0; i < window_size; i++) {
					cout << "Comparing time over in packet B: " << packets_in_B[i].timeover << " with sequence number of received packet: " << packet.seqnum << endl;
					if (packets_in_B[i].timeover == packet.seqnum) {
						cout << "Buffering packet with sequence number: " << packet.seqnum << endl;
						packets_in_B[i].packet = packet;
						packet.acknum = packet.seqnum;
						packet.checksum = getPacketChecksum(&packet);
						tolayer3(B, packet);
						break;
					}
				}
			}
		}
		else {
			cout << "Old packet received, sending old acknowledgment to layer 3" << endl;
			packet.acknum = packet.seqnum;
			packet.checksum = getPacketChecksum(&packet);
			tolayer3(B, packet);
		}
	}
}

/* The following routine will be called once (only) before any other */
/* Entity B routines are called. You can use it to do any initialization */
void B_init() {
	cout << "SR B_init" << endl;

	window_size = getwinsize();
	packets_in_B = new SR[window_size];
	for (int i = 0; i < window_size; i++)
		packets_in_B[i].timeover = i;
}
