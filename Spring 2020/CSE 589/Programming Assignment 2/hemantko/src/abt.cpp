#include "../include/simulator.h"
#include <stdlib.h>

#include <iostream>
#include <queue>

#define PAYLOAD_SIZE 20
#define TIMEOUT 20.0
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
struct pkt packet_to_send;

// Sequence numbers
int sequence_number_A = 0, sequence_number_B = 0;

// States
enum States {
	// Waiting for new packet from layer 5
	waiting_packet,

	// Waiting for acknowledgment from layer 3
	waiting_acknowledgment
};
States sender_state = waiting_packet;

// Buffer messages
queue<msg> buffer;

/* Utility Functions */
int getPacketChecksum(struct pkt *packet);
int getPacketChecksum(struct pkt *packet) {
	int checksum = 0;

	if (packet == NULL)
		return checksum;

	for (int i = 0; i < PAYLOAD_SIZE; i++) {
		checksum += (unsigned char) packet->payload[i];
	}

	checksum += packet->seqnum;
	checksum += packet->acknum;
	cout << "ABT getPacketChecksum(): " << checksum << endl;
	return checksum;
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	cout << "ABT A_output" << endl;

	buffer.push(message);

	if (sender_state != waiting_packet)
		return;

	// Set the packet to waiting for acknowledgment
	sender_state = waiting_acknowledgment;

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
		packet_to_send.payload[i] = m.data[i];

	packet_to_send.seqnum = sequence_number_A;
	packet_to_send.acknum = DEFAULT_ACK_NUM;
	packet_to_send.checksum = getPacketChecksum(&packet_to_send);

	cout << "Sending packet from A with sequence number: " << sequence_number_A << " to layer 3" << endl;
	tolayer3(A, packet_to_send);
	starttimer(A, TIMEOUT);
}

/* Called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	cout << "ABT A_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	if (packet.acknum != sequence_number_A) {
		cerr << "Expected acknowledgment: " << sequence_number_A
				<< ", received  acknowledgment: " << packet.acknum << endl;
		return;
	}

	sequence_number_A = (sequence_number_A + 1) % 2;
	sender_state = waiting_packet;
}

/* Called when A's timer goes off */
void A_timerinterrupt() {
	cout << "ABT A_timerinterrupt" << endl;

	if (sender_state == waiting_acknowledgment) {
		tolayer3(A, packet_to_send);
		starttimer(A, TIMEOUT);
	}
}

/* The following routine will be called once (only) before any other */
/* Entity A routines are called. You can use it to do any initialization */
void A_init() {
	cout << "ABT A_init" << endl;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* Called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	cout << "ABT B_input" << endl;

	if (packet.checksum != getPacketChecksum(&packet)) {
		cerr << "Checksum mismatch" << endl;
		return;
	}

	if (packet.seqnum != sequence_number_B) {
		// Duplicate packet
	} else {
		cout << "Expected packet received, sending it to layer 5" << endl;
		sequence_number_B = (sequence_number_B + 1) % 2;
		tolayer5(B, packet.payload);
	}

	cout << "Sending acknowledgment number: " << packet.seqnum << " to layer 3" << endl;
	packet.acknum = packet.seqnum;
	packet.checksum = getPacketChecksum(&packet);
	tolayer3(B, packet);
}

/* The following routine will be called once (only) before any other */
/* Entity B routines are called. You can use it to do any initialization */
void B_init() {
	cout << "ABT B_init" << endl;
}
