# libnetworking
This project is part of the work on a status monitor for the EDSAC machine. Network communication between the measurement nodes and the mothership.

** This is not suitable for connection to public networks**

cJSON is used directly in this project (contrib/cJSON.c, include/contrib/cJSON.h). cJSON can be found at https://github.com/DaveGamble/cJSON

GLib2.0 >= 2.32 is required as a dependency so that will need to be installed. On Debian this package is called libglib2.0-dev.

On debian the following packages are required to build libnetworking:
```
gcc autoconf libglib2.0-dev libtool make
```

Compile using
```
autoreconf -i
./configure
make
```

Run unit tests using
```
make check
```

Clean up using
```
make distclean
```

## Using the library
For function prototypes see the header files in include/.

For a code example, see src/test/system.c.

### Setup
A process may decide to only send, only receive or both send and receive messages.

Before sending any messages, one must run
```
bool start_sending(const struct sockaddr *addr, socklen_t addrlen);
```
To set up the connection through which to send a message. This only needs to be done once in the lifetime of a process (unless the connection dies, which it shouldn't do and would trigger an allert on the server). addr and addrlen refer to the IPv4 address of the server. Returns true on success.

Before receiving any messages, one must run
```
bool start_server(const struct sockaddr *addr, socklen_t addrlen);
```
To start a server listening on the IPv4 address specified in addr. addrlen should be the size of the addr structure and must be big enough for an IPv4 sockaddr structure. For more information see the BIND(2) man page. start_server only needs to be called once in the lifetime of a process. Returns true on success.

One may find this function useful to convert a string e.g. "127.0.0.1" and port number into a dynamically allocated sockaddr structure:
```
struct sockaddr *alloc_addr(const char *addr, uint16_t port)
```

### Stopping
To close the sending connection:
```
void stop_sending(void);
```

To stop listening for new connections:
```
void stop_server(void);
```

### Message Structures
Messages are reperesented by the Message structure found in representation.h:
```
typedef struct {
    MessageType type;
    MessageData data;
} Message;
```
MessageData is a union over the data segments for each message data type. For definitions of these, see representation.h.

One type of message is a software error. This can be instanced like so:
```
Message msg;
software_error(&msg, "this is the error message");
```

Another type of message is a hardware error. Hardware error messages can be instanced like so:
```
Message msg;
hardware_error(&msg, 1 /* valve number */, 2 /* test point number */, true /* test point high? */);
```

The source of a message does not need to be set explicitly as it will be communicated by the IP address of the node.

Once we are done with a message its contents should be freed. If the Message structure itself was dynamically allocated then that must be freed separately:
```
Message *pmsg = malloc(sizeof(Message));
assert(NULL != pmsg);
software_error(pmsg, "this is the error message");
// ...
free_message(pmsg);
free(pmsg);
pmsg = NULL;
```

### Sending a Message
A message can be sent over the network as follows:
```
assert(true == send_message(pmsg));
```

This must come after the call to start_sending.

### BufferItem Structures
BufferItem is defined in server.h as follows:
```
typedef struct {
    Message msg; // Error Message
    struct in_addr address; // IPv4 address which sent (or generated) the error
} BufferItem;
```

A BufferItem can be freed using free_bufferitem(BufferItem \*item). Unlike free_message, *this will call free(item)*. So one should not use statically allocated BufferItems. 

### Receiving a Message
Received messages are received asynchronously using signals and buffered in a queue. When convenient use
```
BufferItem *read_message(void);
```
To return the next BufferItem in the queue. If the queue is empty then NULL will be returned immediately (there is no blocking waiting for new messages). 
