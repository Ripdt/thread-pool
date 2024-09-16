## How it works?

This code implements a server that uses two sockets or local named pipes to communicate with clients. It runs two types of connections: one to strings and another to numbers. The server uses a thread pool to manage multiple clients in a coherent manner. Let's make a walkthrough:

### Abstract:

1.  **Initialization**: Create two sockets (string and numeric) and a thread pool with two threads.
2.  **Monitoring connections with `select`**: Waits connections in the two sockets.
3.  **Acception**: Accepts a connection with a client in one of the two pipes.
4.  **Data process**:
    -   `string`: to_upper.
    -   `numeric`: adds 10.
5.  **Response**: The server writes the data to the client.
6.  **Concurrency**: Uses separated thread, allowing multiple clients at the same time.

This code uses concurrency, sockets and interprocess communication in a Linux environment, allowing the server to manage multiple clients.

### 1. **Creation of two sockets**

The local sockets are created with the functions socket and bind. The server uses two different paths:

-   `/tmp/pipestr` for strings.
-   `/tmp/pipenum` for numbers.

This sockets use the `AF_UNIX` type to communicate with local proccess in the same operational system.

### 2. **Use of `select` for multiple sockets**

The server uses the `select` function to wait both sockets at the same time. It identifies which pipe is ready to accept a new connection.

#### What happens inside `select`:

-   The server call `select` and waits for one of the sockets to have a waiting connection.
-   If a client tries to connect to the string socket (`/tmp/pipestr`), the server accepts it and proccess as a string connection.
-   If a client tries to connect to the numeric socket  (`/tmp/pipenum`), the server accepts it and proccess as a numeric connection.

### 3. **Acception of new connections**

When the `select` function detects a connection, the `accept` function returns a new socket that represents the specific connection with the client.

### 4. **Struct `client_data_t`**

This struct storages two informations:

-   `newsockfd`: the client socket.
-   `pipe_type`: indicates which type of pipe the client it's using (string or numeric).

### 5. **Client data processing**

After the server accepts the connection, it needs to process what the client sent. This is done by the `process_client`, which is added to the thread pool to be executed using concurrency.

#### Inside `process_client`:

-   **Data reception**: The server reads the data using `read`.
-   **Pipe based processing**:
    -   If the client is connected in the string pipe, the buffer is transformed to upper.
    -   If the client is connected in the numeric pipe, the buffer is added up a constant (10).
-   **Server response**: The server writes the results using `write`.

### 6. **Thread pool**

The server uses a thread pool library called `thpool` that is responsible to manage the threads to execute the functions using concurrency.

-   The server initializes the thread pool with two threads using `thpool_init(2)`.
-   When a new client connects, the `process_client` function is added to the pool, which executes a thread separetedly of the others. This allows the server to accept another connections while process the others.

### 7. **Ending**

-   The connection with the client is closed (`newsockfd`).
-   The allocated memory for the client (`client_data_t`) is freed.

If the server is closed:

-   Close the pipes (`sockfd_str` and `sockfd_num`).
-   Destroy the _thread pool_ using `thpool_destroy`.

## Execution

As the library chosen to run the thread pool isn't pre-compiled, it needs to be compiled with the project. So, execute this command:
```
gcc server.c thpool.c -lpthread -o ./bin/server
```
And run it:
```
./bin/server
```
In a different terminal, compile a client
```
gcc client.c -o ./bin/client
```
Then run it:
```
./bin/client [S|N]
```
You can run as much clients as you want.
