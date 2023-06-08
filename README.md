# IPK Project

UDP/TCP server and client for calculator on linux.

## Usage

### Client

```
/ipkcpc -h <host> -p <port> -m <mode>
```

`make runtcp` runs client as
`/ipkcpc -h 127.0.0.1 -p 2020 -m tcp`

`make runudp` runs client as
`/ipkcpc -h 127.0.0.1 -p 3030 -m udp`

### Server

```
/ipkpd -h <host> -p <port> -m <mode>
```

`make runtcp/runudp` work same as with client

## Calculator protocol

Original: https://git.fit.vutbr.cz/NESFIT/IPK-Projekty/src/branch/master/Project%201/Protocol.md

```
operator = "+" / "-" / "*" / "/"
expr = "(" operator 2*(SP expr) ")" / 1*DIGIT
query = "(" operator 2*(SP expr) ")"
```

### UDP

Communication is ready from the get-go. The client CAN sen queries and should recieve computed result (not to be confused with TCP *result*). Any number of queries can be sent, including none. When the client wishes to terminate the connection, it just terminates it. The server doesn't reply.


In case of a malformed, illegal, or otherwise unexpected message, the server will close the connection.

### TCP

Communication begins in the Init state with a *hello* greeting from the client, to which the server responds with *hello* if accepting requests, transitioning to the Established state. Afterwards, the client CAN send queries with the *solve* command and should receive the computed *result*. Any number of *solve* queries can be sent in this state, including none. When the client wishes to terminate the connection, it MUST send a *bye* command. The server will reply with *bye* before terminating its part of the connection.

In case of a malformed, illegal, or otherwise unexpected message, the server will respond with bye and close the connection.


#### TCP Message formats
```
hello = "HELLO" LF
solve = "SOLVE" SP query LF
result = "RESULT" SP 1*DIGIT LF
bye = "BYE" LF
```