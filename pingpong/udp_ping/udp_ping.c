/*
 * udp_ping.c: esempio di implementazione del processo "ping" con
 *             socket di tipo DGRAM.
 *
 * versione 5.1
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova, anno accademico 2016/2017.
 *
 * Copyright (C) 2013-2014 by Giovanni Chiola <chiolag@acm.org>
 * Copyright (C) 2015-2016 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "pingpong.h"

/*
* This function sends and wait for a reply on a socket.
* char message[]: message to send
* int messagesize: message length
*/

double do_ping(size_t msg_size, int msg_no, char message[msg_size], int ping_socket, double timeout)
{
	int lost_count = 0;
	char answer_buffer[msg_size];
	ssize_t recv_bytes, sent_bytes;
	struct timespec send_time, recv_time;
	double roundtrip_time_ms;
	int re_try = 0;

    /*** write msg_no at the beginning of the message buffer ***/
/*** TO BE DONE START ***/

	if(sprintf(message,"%d\n",msg_no) < 0)
		fail("Error writing message");
		
/*** TO BE DONE END ***/

	do {
		debug(" ... sending message %d\n", msg_no);
		
	/*** Store the current time in send_time ***/
/*** TO BE DONE START ***/

		if(clock_gettime(CLOCK_MONOTONIC, &send_time) != 0)
			fail_errno("clock_gettime failed with sendtime");
			
/*** TO BE DONE END ***/

	/*** Send the message through the socket ***/
/*** TO BE DONE START ***/

		if ((sent_bytes = send(ping_socket, message, msg_size, 0)) < msg_size)
			fail_errno("Send error");
			
/*** TO BE DONE END ***/

	/*** Receive answer through the socket (non blocking mode) ***/
/*** TO BE DONE START ***/

		size_t offset;
		if(fcntl(ping_socket,F_SETFL, O_NONBLOCK) == -1)
			fail_errno("set NONBLOCKING socket fail");

		for (offset = 0; (offset + (recv_bytes = recv(ping_socket, answer_buffer + offset, sent_bytes - offset, MSG_WAITALL))) < msg_size; offset += recv_bytes) {
			debug(" ... received %zd bytes back\n", recv_bytes);
			if (recv_bytes < 0)
				fail_errno("Error receiving data");
		}

/*** TO BE DONE END ***/

	/*** Store the current time in recv_time ***/
/*** TO BE DONE START ***/

		if(clock_gettime(CLOCK_MONOTONIC, &recv_time) != 0)
			fail_errno("clock_gettime failed with recvtime");

/*** TO BE DONE END ***/

		roundtrip_time_ms = timespec_delta2milliseconds(&recv_time, &send_time);

		for (;
		     recv_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)
		     && roundtrip_time_ms < timeout;) {
			recv_bytes = recv(ping_socket, answer_buffer, sizeof(answer_buffer), 0);
			clock_gettime(CLOCK_TYPE, &recv_time);
			roundtrip_time_ms = timespec_delta2milliseconds(&recv_time, &send_time);
		}
		if (recv_bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			fail_errno("UDP ping could not recv from UDP socket");
		if (recv_bytes < sent_bytes) {	/*time-out elapsed: packet was lost */
			lost_count++;
			if (recv_bytes < 0)
				recv_bytes = 0;
			printf("\n ... received %zd bytes instead of %zd (lost count = %d); re-trying ...\n", recv_bytes, sent_bytes, lost_count);
			if (++re_try > MAXUDPRESEND) {
				printf(" ... giving-up!\n");
				fail("too many lost datagrams");
			}
			printf(" ... re-trying ...\n");
		}
	} while (sent_bytes != recv_bytes);
	return roundtrip_time_ms;
}

int prepare_udp_socket(char *pong_addr, char *pong_port)
{
	struct addrinfo gai_hints, *pong_addrinfo = NULL;
	int ping_socket;
	int gai_rv;

    /*** Specify the UDP sockets' options ***/
	memset(&gai_hints, 0, sizeof gai_hints);

/*** TO BE DONE START ***/

	//gai_hints.ai_flags = AI_PASSIVE;
	gai_hints.ai_family = AF_INET; // AF_INET, AF_INET6, AF_UNSPEC
	gai_hints.ai_socktype = SOCK_DGRAM; // SOCK_STREAM, SOCK_DGRAM
	gai_hints.ai_protocol = 0; // 0 per "any"

/*** TO BE DONE END ***/

	if ((ping_socket = socket(gai_hints.ai_family, gai_hints.ai_socktype, gai_hints.ai_protocol)) < 0)
		fail_errno("UDP Ping could not get socket");

    /*** change socket behavior to NONBLOCKING ***/
/*** TO BE DONE START ***/

	if(fcntl(ping_socket, F_SETFL, O_NONBLOCK) < 0)
		fail_errno("Setting socket to NONBLOCKING failed");

/*** TO BE DONE END ***/

    /*** call getaddrinfo() in order to get Pong Server address in binary form ***/
/*** TO BE DONE START ***/

    if ((gai_rv = getaddrinfo(pong_addr, pong_port, &gai_hints, &pong_addrinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(gai_rv));
        exit(1);
    }

/*** TO BE DONE END ***/

#ifdef DEBUG
	{
		char ipv4str[INET_ADDRSTRLEN];
		const char * const cp = inet_ntop(AF_INET, &(((struct sockaddr_in *)(pong_addrinfo-> ai_addr))->sin_addr), ipv4str, INET_ADDRSTRLEN);
		if (cp == NULL)
			printf(" ... inet_ntop() error!\n");
		else
			printf(" ... about to connect socket %d to IP address %s, port %hu\n",
			     ping_socket, cp, ntohs(((struct sockaddr_in *)(pong_addrinfo->ai_addr))->sin_port));
	}
#endif

    /*** connect the ping_socket UDP socket with the server ***/
/*** TO BE DONE START ***/

	if(connect(ping_socket, pong_addrinfo->ai_addr, pong_addrinfo->ai_addrlen) < 0)
		fail_errno("Connection error");

/*** TO BE DONE END ***/

	freeaddrinfo(pong_addrinfo);
	return ping_socket;
}

int main(int argc, char *argv[])
{
	struct addrinfo gai_hints, *server_addrinfo;
	int ping_socket, ask_socket;;
	int msg_size, norep;
	int gai_rv;
	char ipstr[INET_ADDRSTRLEN];
	struct sockaddr_in *ipv4;
	char request[40], answer[10];
	ssize_t nr;
	int pong_port;

	if (argc < 4)
		fail("Incorrect parameters provided. Use: udp_ping PONG_ADDR PONG_PORT MESSAGE_SIZE [NO_REPEAT]\n");
	for (nr = 4, norep = REPEATS; nr < argc; nr++)
		if (*argv[nr] >= '1' && *argv[nr] <= '9')
			sscanf(argv[nr], "%d", &norep);
	if (norep < MINREPEATS)
		norep = MINREPEATS;
	else if (norep > MAXREPEATS)
		norep = MAXREPEATS;
	if (sscanf(argv[3], "%d", &msg_size) != 1 || msg_size < MINSIZE || msg_size > MAXUDPSIZE)
		fail("Wrong message size");

    /*** Specify TCP socket options ***/
	memset(&gai_hints, 0, sizeof gai_hints);
/*** TO BE DONE START ***/

	//gai_hints.ai_flags = AI_PASSIVE;
	gai_hints.ai_family = AF_INET; // AF_INET, AF_INET6, AF_UNSPEC
	gai_hints.ai_socktype = SOCK_STREAM; // SOCK_STREAM, SOCK_DGRAM
	//gai_hints.ai_protocol = 0; // 0 per "any"
	
/*** TO BE DONE END ***/

    /*** call getaddrinfo() in order to get Pong Server address in binary form ***/
/*** TO BE DONE START ***/

	if ((gai_rv = getaddrinfo(argv[1], argv[2], &gai_hints, &server_addrinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(gai_rv));
		exit(1);
	}

/*** TO BE DONE END ***/

    /*** Print address of the Pong server before trying to connect ***/
	ipv4 = (struct sockaddr_in *)server_addrinfo->ai_addr;
	printf("UDP Ping trying to connect to server %s (%s) on TCP port %s\n", argv[1], inet_ntop(AF_INET, &ipv4->sin_addr, ipstr, INET_ADDRSTRLEN), argv[2]);

    /*** create a new TCP socket and connect it with the server ***/
/*** TO BE DONE START ***/
	if ((ask_socket = socket(server_addrinfo->ai_family, server_addrinfo->ai_socktype, server_addrinfo->ai_protocol)) < 0)
		fail_errno("Socket error");

	if ((connect(ask_socket, server_addrinfo->ai_addr, server_addrinfo->ai_addrlen)) < 0)
		fail_errno("Connection error");
/*** TO BE DONE END ***/

	freeaddrinfo(server_addrinfo);
	printf(" ... connected to Pong server: asking for %d repetitions of %d _bytes UDP messages\n", norep, msg_size);
	sprintf(request, "UDP %d %d\n", msg_size, norep);

    /*** Write the request on the TCP socket ***/
/** TO BE DONE START ***/

	size_t len = strlen(request);

    //printf("%s", request);

	if((send(ask_socket, request, len, 0)) != len)
		fail_errno("Send error");

/*** TO BE DONE END ***/

	nr = read(ask_socket, answer, sizeof(answer));
	if (nr < 0)
		fail_errno("UDP Ping could not receive answer from Pong server");
	if (nr==sizeof(answer))
		--nr;
	answer[nr] = 0;

    /*** Check if the answer is OK, and fail if it is not ***/
/*** TO BE DONE START ***/

	//printf("%s",answer);
	
	if((strncmp(answer, "OK", 2)) != 0)
		fail ("Answer is not \"OK\"");
		
	else
/*** TO BE DONE END ***/

    /*** else ***/
	sscanf(answer + 3, "%d\n", &pong_port);
	printf(" ... Pong server agreed to ping-pong using port %d :-)\n", pong_port);
	sprintf(answer, "%d", pong_port);
	shutdown(ask_socket, SHUT_RDWR);
	close(ask_socket);

	ping_socket = prepare_udp_socket(argv[1], answer);

	{
		char message[msg_size];
		memset(&message, 0, (size_t)msg_size);
		double ping_times[norep];
		struct timespec zero, resolution;
		int repeat;
		for (repeat = 0; repeat < norep; repeat++) {
			ping_times[repeat] = do_ping((size_t)msg_size, repeat + 1, message, ping_socket, UDP_TIMEOUT);
			printf("Round trip time was %6.3lf milliseconds in repetition %d\n", ping_times[repeat], repeat + 1);
		}
		memset((void *)(&zero), 0, sizeof(struct timespec));
		if (clock_getres(CLOCK_TYPE, &resolution) != 0)
			fail_errno("UDP Ping could not get timer resolution");
		print_statistics(stdout, "UDP Ping: ", norep, ping_times, msg_size, timespec_delta2milliseconds(&resolution, &zero));

	}

	close(ping_socket);
	exit(EXIT_SUCCESS);
}
