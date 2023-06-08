/*
 * IPK Project 2
 *
 * Andrej Nešpor
 * xnespo10
 * 
 */


// Include libraries for Linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <vector>
#include <sstream>

#include "ipkpd.hpp"


// Global variables
std::string mode;
int clientSocket;
int serverSocket;

// Function to parse arguments
char* argparse(int argc, char *argv[], long& port) {
    // Parse arguments using getopt
    char* host;
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
                    std::cerr << "ERROR: invalid port: "<< optarg << "\n";
                    std::cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
                    exit(EXIT_FAILURE);
                }
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                std::cerr << "ERROR: invalid option: " << optopt << "\n";
                std::cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    // Check that all required arguments were provided
    if (!host || port == -1 || mode.empty()) {
        std::cerr << "ERROR: insufficient arguments\n";
        std::cout << "Usage: " << argv[0] << " -h <host> -p <port> -m <mode>\n";
        exit(EXIT_FAILURE);
    }
    return host;
}

// Function used to check if string is unsigned digit
bool isDigit(std::string str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            // Return false if any character is not a digit
            return false;
        }
    }
    // Return true if all characters are digits
    return true;
}

// Control function used to print stack content
void print_stack(std::vector<std::string> stack) {
    for (const auto& str : stack) {
        std::cout << str << " ";
    }
    std::cout << std::endl;
}

void reduce (std::vector<std::string>& stack, int position, bool& exit) {
        // Split stack by last LPAR
        std::vector<std::string> stack1(stack.begin(), stack.begin() + position);
        std::vector<std::string> stack2(stack.begin() + position, stack.end());

        if (stack2.size() < 5) {
            exit = true;
            return;
        }
        // Every stack has (, op, 2*EXP, )

        // Remove ( from stack
        stack2.erase(stack2.begin());
        // Get operation and remove it from stack
        std::string op = stack2.front();
        stack2.erase(stack2.begin());
        // Get first number and remove it from stack
        long int tmp = std::stol(stack2.front());
        stack2.erase(stack2.begin());
        // Loop through stack until RPAR
        while (stack2.front() != ")") {
            // Choose aritmetic operation based on operator
            if (op == "+") {
                tmp += std::stol(stack2.front());
            } else if (op == "-") {
                tmp -= std::stol(stack2.front());
            } else if (op == "*") {
                tmp *= std::stol(stack2.front());
            } if (op == "/") {
                if (std::stol(stack2.front()) == 0) {
                    // Div by zero
                    exit = true;
                    return;
                }
                tmp /= std::stol(stack2.front());
            }
            // Remove used number from stack
            stack2.erase(stack2.begin());
        }
        // Remove RPAR from stack
        stack2.erase(stack2.begin());
        // Insert result to bottom of stack
        stack2.insert(stack2.begin(), std::to_string(tmp));
        
        // Put the stacks back into one
        stack = stack1;
        stack.insert(stack.end(), stack2.begin(), stack2.end());
}

// Function used to calculate expressions
// FSM described in ipkpd.hpp
bool parse_exp(std::string& input) {
    // Main stack used for storing input
    std::vector<std::string> stack;
    int len = input.length();
    size_t cLPAR = 0;
    size_t cRPAR = 0;
    // Stack of LPAR locations in main stack
    std::vector<int> LPAR_loc;
    bool exitLoop = false;
    ParserState curr_state = START;
    // std::cout << "Input: " << input << std::endl;

    // Iterate through expression by chars
    for (int i = 0; i < len && !exitLoop; i++) {
        std::string c = std::string(1, input[i]);
        // std::cout << "Char: " << c << std::endl;
        // Switch for FSM states
        switch (curr_state) {
            // START state only accepts "(" for UDP
            // or "SOLVE (" for TCP
            case START:
                if (i != 0) {
                    exitLoop = true;
                    break;
                }
                if (mode == "tcp") {
                    if (len < 6) {
                        // Check if string has sufficient length
                        exitLoop = true;
                        break;
                    }
                    std::string tmp = c + input[i+1] + input[i+2]; // "SOL "
                    tmp = tmp + input[i+3] + input[i+4] + input[i+5]; // "SOLVE "
                    if (tmp != "SOLVE ") {
                        // Check for the SOLVE keyword in TCP
                        exitLoop = true;
                        break;
                    } else {
                        // Skip in iterating since "SOLVE " is already validated
                        i += 6;
                        c = std::string(1, input[i]);
                    }
                }
                if (c == "(") {
                    // If "(" is found switch state to LPAR and push to stack
                    curr_state = LPAR;
                    cLPAR++;
                    LPAR_loc.push_back(stack.size());
                    stack.push_back(c);
                    break;
                }
                exitLoop = true;
                break;
            case LPAR:
                // Check for operator right after LPAR
                if (c == "+" or c == "-" or c == "*" or c == "/") {
                    // Check if there's space after operator
                    if (input[i+1] == ' ') {
                        // If operator is correct switch state to EXP and push
                        i++;
                        curr_state = EXP;
                        stack.push_back(c);
                        break;
                    }
                }
                exitLoop = true;
                break;
            case EXP:
                if (c == "(") {
                    // If "(" is found switch state to LPAR and push to stack
                    // In EXP states in means query inside query
                    curr_state = LPAR;
                    cLPAR++;
                    LPAR_loc.push_back(stack.size());
                    stack.push_back(c);
                    break;
                } else if (std::isdigit(input[i])) {
                    // If current char is number check if full expression is number
                    // Limit string only after current char
                    std::string tmp = input.substr(i, len);
                    int where = tmp.find(')');
                    if (where == -1) {
                        // if theres no RPAR, it means an error
                        exitLoop = true;
                        break;
                    }
                    // Limit string only until next RPAR
                    std::string num = tmp.substr(0, where);
                    int wheree = num.find(' ');
                    if (wheree != -1) {
                        // If space is found limit the string more
                        where = wheree;
                    }
                    num = tmp.substr(0, where);
                    // Check if the limited string is number
                    if (!isDigit(num)) {
                        exitLoop = true;
                        break;
                    }
                    i = i + num.length() - 1;
                    stack.push_back(num);
                    break;
                } else if (c == ")" && input[i-1] != ' ') {
                    // RPAR in EXP state signal query end
                    // Push RPAR to stack
                    cRPAR++;
                    stack.push_back(c);
                    // End of query was found so reduce
                    // last query to expression
                    reduce(stack, LPAR_loc.back(), exitLoop);
                    // Remove one LPAR location
                    LPAR_loc.pop_back();

                    // If there's same number of LPARs and RPARs
                    // it signals end of main query
                    if (cLPAR == cRPAR) {
                        curr_state = END;
                        break;
                    }
                    curr_state = RPAR;
                    break;
                } else if (c == " ") {
                    // Space is just separator between expressions and queries
                    break;
                }
                exitLoop = true;
                break;
            case RPAR:
                if (c == ")") {
                    // RPAR in RPAR state signal query end
                    // of query that was inside another query
                    // Do the same as RPAR in EXP state
                    cRPAR++;
                    stack.push_back(c);
                    reduce (stack, LPAR_loc.back(), exitLoop);
                    LPAR_loc.pop_back();
                    if (cLPAR == cRPAR) {
                        curr_state = END;
                        break;
                    }
                    curr_state = RPAR;
                    break;
                }
                if (cRPAR == cLPAR) {
                    // Reading char after EXP ended
                    exitLoop = true;
                    break;
                }
                if (c == " ") {
                    // Separator between expressions and queries
                    curr_state = EXP;
                    break;
                }
                exitLoop = true;
                break;
            case END:
                // Reading char after EXP ended
                exitLoop = true;
                break;
            default:
                break;
        }
    }
    if (!stack.empty()) {
        input = stack.back();
    }
    if (mode == "udp") {
        // Change input to error for UDP
        if (exitLoop) {
            input = "Could not parse the message";
        } else if (curr_state != END) {
            input = "Could not parse the message";
            exitLoop = true;
        }
    } if (mode == "tcp") {
        // No need to change input to error for TCP
        // only send BYE and terminate connection
        if (exitLoop) {
            std::cout << "ERR:Could not parse the message" << std::endl;
        } else if (curr_state != END) {
            std::cout << "ERR:Could not parse the message" << std::endl;
            exitLoop = true;
        }
        if (input.at(0) == '-') {
            // Check for negative result in TCP
            std::cout << "ERR:Negative result" << std::endl;
            exitLoop = true;
        }
    }
    return exitLoop;
}

void setup(int port, sockaddr_in& addr) {
    // Create a socket based on mode
    if (mode == "tcp") {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Enable reusing the address and port immediately after termination
    int optval = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    // Initialize the server adress structure
    bzero((char*) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // If using TCP, listn for incoming connections
    if (mode == "tcp") {
        if (listen(serverSocket, 2) < 0) {
            std::cerr << "Error listening on socket" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return;
}

void send_bye() {
    // End connection with tcp client
    send(clientSocket, "BYE\n", 4, 0);
    close(clientSocket);
    return;
}

// Handle ctrl+c termination
void end_connection(int signal) {
    if (mode == "tcp") {
        // Gracefull exit for tcp
        send_bye();
    }
    close(serverSocket);

    exit(EXIT_SUCCESS);
}

void communicate_udp() {
    char buffer[1024];
    int bytesrx, bytestx;
    socklen_t client_len;
    struct sockaddr_in client_addr;
    bool status;

    signal(SIGINT, end_connection);

    // Receive incoming data from clients
    while (1) {
        memset(buffer, '\0', 1024);
        client_len = sizeof(client_addr);
        bytesrx = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &client_len);
        if (bytesrx < 0) {
            perror("ERROR in recvfrom");
            return;
        }

        // Process received data
        std::cout << "Received data from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ": " << buffer+2 << std::endl;

        strcpy(buffer, buffer+2);
        std::string sResponse(buffer);
        status = parse_exp(sResponse);
        std::cerr << "Response: " << sResponse << std::endl;

        // Send a response back to the client
        int len = sResponse.length();
        if (len > 255) {
            std::cerr << "Error: response too large" << std::endl;
        }
        sResponse.insert(0, 1, static_cast<char>(len));
        if (status) {
            sResponse.insert(0, 1, '\1'); // Error
        } else {
            sResponse.insert(0, 1, '\0'); // Ok
        }
        sResponse.insert(0, 1, '\1');
        const char* Response = sResponse.c_str();
        bytestx = sendto(serverSocket, Response, len+3, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
        if (bytestx < 0) {
            perror("ERROR in send");
            return;
        }
    }

    // Close the server socket
    close(serverSocket);
}

void communicate_tcp() {
    char buffer[1024];
    int bytesrx, bytestx;
    struct sockaddr_in client_addr;
    bool status = false;
    TCPState curr_state;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGINT, end_connection);

    // Receive incoming data from clients
    while (1) {
        curr_state = INIT;
        status = false;
        clientSocket = accept(serverSocket, (struct sockaddr *)&client_addr, &client_len);
        if (clientSocket < 0) {
            perror("ERROR in accept");
            return;
        }

        while (!status) {
            memset(buffer, '\0', 1024);
            bytesrx = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesrx < 0) {
                perror("ERROR in recv");
                close(clientSocket);
                return;
            } else if (bytesrx == 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }

            // Process received data
            std::cout << "Received data from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ": " << buffer << std::endl;

            std::stringstream ss(buffer);
            std::string sResponse;
            while (std::getline(ss, sResponse)) {
                if (curr_state == INIT) {
                    if (sResponse != "HELLO") {
                        // Wrong connection establishment
                        send_bye();
                        status = true;
                        break;
                    }
                    sResponse += "\n";
                    bytestx = send(clientSocket, sResponse.data(), sResponse.size(), 0);
                    if (bytestx < 0) {
                        perror("ERROR in send");
                        close(clientSocket);
                        return;
                    }
                    curr_state = ESTABLISHED;
                } else {
                    if (sResponse == "BYE") {
                        // Send bye back and end connection
                        send_bye();
                        status = true;
                        break;
                    }
                    // Calculate expression
                    status = parse_exp(sResponse);
                    if (status) {
                        // Error calculating expression
                        send_bye();
                        break;
                    }
                    // Print for server side
                    std::cerr << "Response: " << sResponse << std::endl;
                    // Send response in wanted format
                    sResponse = "RESULT " + sResponse + "\n";

                    // Send a response back to the client
                    bytestx = send(clientSocket, sResponse.data(), sResponse.size(), 0);
                    if (bytestx < 0) {
                        perror("ERROR in send");
                        close(clientSocket);
                        return;
                    }
                }
            }
        }
        // Close the client socket
        close(clientSocket);
    }
    // Close the server socket
    close(serverSocket);
}

int main (int argc, char *argv[]) {
    const char* host = NULL;
    struct sockaddr_in server_addr;
    long port = -1;

    host = argparse(argc, argv, port);
    setup(port, server_addr);

    if (mode == "udp") {
        communicate_udp();
    } else {
        communicate_tcp();
    }

    exit(EXIT_SUCCESS);
}