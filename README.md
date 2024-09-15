
## Execution

As the library chosen to run the thread pool isn't pre-compiled, it needs to be compiled with the project. So, execute this command:
```
gcc server.c thpool.c -lpthread -o ./bin/server
```
And run it:
```
./bin/client
```
In a different terminal, compile a client
```
gcc client.c -o ./bin/client
```
Then run it:
```
./bin/client
```
You can run as much clients as you want.