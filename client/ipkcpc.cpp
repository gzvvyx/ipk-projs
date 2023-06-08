/*
 * IPK Project 1
 *
 * Andrej Nešpor
 * xnespo10
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>

// Libraries for Linux
// Netdb.h and apra/inte.h are replaced with winsock2.h on windows

using namespace std;

#define UDP_PAYLOAD 255
#define REPLYSIZE 1024

string mode;
int client_socket;
char reply [REPLYSIZE];

// Handle ctrl+C termination
void end_connection(int signal) {
    if (mode == "tcp") {
        signal = send(client_socket, "BYE\n", 4, 0);
        if (signal < 0) {
            perror("ERROR in sendto");
            exit(EXIT_FAILURE);
        }
        signal = recv(client_socket, reply, REPLYSIZE, 0);
        reply[strlen(reply)-1] = '\0';
        cout << reply;
        if (signal < 0) {
            perror("ERROR in recvfrom");
            exit(EXIT_FAILURE);
        }
    }

    close(client_socket);
    exit(EXIT_SUCCESS);
}

int main (int argc, char *argv[]) {
    int bytestx, bytesrx;
    const char *host = NULL;
    socklen_t server_len;
    struct hostent *server;
    struct sockaddr_in server_addr;
    long port = -1;

    // Parse arguments using getopt
    int opt;
    while ((opt = getopt(argc, argv, "h:p:m:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                char *endptr;
                port = strtol(optarg, &endptr, 10);
                // Validate port number
                if (*optarg == '\0' || *endptr != '\0' || port < 0 || port > 65535) {
                    cerr << "ERROR: invalid port: "<< optarg << "\n";
                    cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                cerr << "ERROR: invalid option: " << optopt << "\n";
                cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    // Check that all required arguments were provided
    if (!host || port == -1 || mode.empty()) {
        cerr << "ERROR: insufficient arguments\n";
        cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
        exit(EXIT_FAILURE);
    }

    // Validate host argument
    if (inet_pton(AF_INET, host, &(server_addr.sin_addr)) != 1) {
        cerr << "ERROR: invalid host: " << host << "\n";
        cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
        exit(EXIT_FAILURE);
    }

    // Validate mode argument
    if (mode != "tcp" && mode != "udp") {
        cerr << "ERROR: invalid mode: " << mode << "\n";
        cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
        exit(EXIT_FAILURE);
    }

    // Get server address using DNS
    if (!(server = gethostbyname(host))) {
        cerr << "ERROR: no such host as " << host << "\n";
        exit(EXIT_FAILURE);
    }

    // Find server IP address
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Create socket
    int SOCK;
    if (mode == "tcp") {
        SOCK = SOCK_STREAM;
    } else SOCK = SOCK_DGRAM;
    if ((client_socket = socket(AF_INET, SOCK, 0)) <= 0) {
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}

    // Timout for UDP
    if (mode == "udp") {
        struct timeval timeout = {3,0};
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    }

    // Connect to server
    if (mode == "tcp" && connect(client_socket, (const struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
		perror("ERROR: connect");
		exit(EXIT_FAILURE);
    }

    signal(SIGINT, end_connection);

    string msg;
    int size;
    // Loop to infinitely communicate
    while (1) {
        // Get message from user
        bzero(reply, REPLYSIZE);
        bzero(msg.data(), msg.size());
        getline(cin, msg);

        // Message send
        if (mode == "tcp") {
            msg.push_back('\n');
            bytestx = send(client_socket, msg.data(), msg.size(), 0);
        } else {
            if (cin.eof()) {
                cerr << "ERROR: EOF found\n";
                end_connection(0);
                exit(EXIT_FAILURE);
            }
            if (msg.empty()) {
                cerr << "ERROR: Input is empty\n";
                continue;
            }
            size = msg.size();
            if (size > UDP_PAYLOAD) {
                cerr << "ERROR: UDP payload too large: " << size << "\n";
                end_connection(0);
                exit(EXIT_FAILURE);
            }
            memmove(&msg[2], msg.data(), size);
            msg.at(0) = '\0';
            msg.at(1) = (char)size;
            server_len = sizeof(server_addr);
            bytestx = sendto(client_socket, msg.data(), size+2, 0, (struct sockaddr *) &server_addr, server_len);
        }
        if (bytestx < 0) {
            perror("ERROR in sendto");
            exit(EXIT_FAILURE);
        }

        // Message recieve
        if (mode == "tcp") {
            bytesrx = recv(client_socket, reply, REPLYSIZE, 0);
            cout << reply;
        } else {
            bytesrx = recvfrom(client_socket, reply, REPLYSIZE, 0, (struct sockaddr *) &server_addr, &server_len);
            size = strlen(reply+3);
            if (size > UDP_PAYLOAD) {
                cerr << "ERROR: UDP payload too large: " << size << "\n";
                end_connection(0);
                exit(EXIT_FAILURE);
            }
            if (reply[1] == '\0') {
                cout << "OK:" << reply+3 << "\n";
            } else cout << "ERR:" << reply+3 << "\n";
        }
        if (bytesrx < 0) {
            perror("ERROR in recvfrom");
            exit(EXIT_FAILURE);
        }
        if (strcmp(reply, "BYE\n") == 0) {
            break;
        }
    }

    close(client_socket);

    exit(EXIT_SUCCESS);
}