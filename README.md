
# Multithreaded Server

Multithreaded Server is basically a server which uses concept of multithreading where each request which come to server is given to a separate thread which handles it and return the response.
## Features

- Multithreaded
- Handle multiple requests at same time
- Maintains threadpool so no need to create thread again and again
- Uses Sockets to establish communication between client and server
- Client send "Hello" and server send "world" as response for each request.
- loadgen.sh script to test how much load our server can handle.

## Tech Stack

**C**: The core language used to create the multithreaded server.

**Socket**: Used to establish communication between client and server.

**Pthread**: Library used to implement multithreading.

## Run Locally

Clone the project

```bash
git clone https://github.com/menishantsharma/Multithreaded-Server
```

Go to the project directory

```bash
cd Multithreaded-Server
```

Compiler server

```bash
gcc server.c -o server
```

Run server

```bash
./server <port> <thread_pool_size>
```

Change permissions of loadgen script
```bash
chmod +x loadgen.sh
```

Run loadgen script 
```bash
./loadgen.sh <number_of_clients> <server_ip> <server_port>
```

## Authors

- **Nishant Sharma** - MTech CSE Student at IIT Bombay
- **Github** - [@Nishant Sharma](https://github.com/menishantsharma)


## License

[MIT](https://choosealicense.com/licenses/mit/)