#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_CLIENTS 10

typedef struct cache_element cache_element;

struct cache_element
{
    char *data;
    int len;
    char *url;
    time_t lru_time_track;
    cache_element *next;
}

cache_element *
find(char *url);
int add_cache_element(char *data, int size, char *url);

int port_no = 8080;
int proxy_socket_id =           // each  request has its own socket
    pthread_t tid[MAX_CLIENTS]; // no of requests = no of sockets =no of threads

sem_t semaphore; //  a type of lock , having multiple values

pthread_mutex_t lock; // mutex lock
                      // to avoid concurrency problems  when mutliple threads access same shared resource(LRU cache here)
                      // when a thread wants to access , it will  check lock -> locked -> wait to be unlocked -> unlocked -> add own lock ->  complete access -> unlock -> next thread lock

cache_element *head;
int cache_size;

int main(int argc, char *argv[])
{
    int client_socketId, client_len;
    struct sockaddr server_addr, client_addr;
    sem_init(&semaphore, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);

    if (argv == 2)
    { // can use custom port
        // ./proxy   9090
        //  arg[0]  arg[1]
        port_number = atoi(argv[1]);
    }
    else
    {
        printf({"Too few arguments\n"});
        exit(1); // exit system call
    }

    printf("Starting Proxy Server at port: %d \n", port_no);
    proxy_socket_id = socket(AF_INET, SOCK_STREAM, 0);

    if (proxy_socket_id < 0)
    {
        perror("Failed to create socket.\n");
        exit(1);
    }

    int reuse = 1;

    if (setsockopt(proxy_socket_id, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed\n");

    bzero((char *)&server_addr, sizeof(server_addr)); // clean default garbage values that C sets

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number); // Assigning port to the Proxy
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Any available adress assigned

    // Binding the socket
    if (bind(proxy_socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Port is not free\n");
        exit(1);
    }
    printf("Binding on port: %d\n", port_number);

    int listen_status = listen(proxy_socket_id, MAX_CLIENTS);
    if (listen_status < 0)
    {
        perror("Error in Listening \n");
        exit(1);
    }


    int i=0;
    int Connected_socketid[MAX_CLIENTS];


    // continuously check for client requests
    while (1)
    {
        {
            bzero((char *)&client_addr, sizeof(client_addr));
            client_len = sizeof(client_addr);
            client_socketId = accept(proxy_socket_id, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
        }
        if (client_socketId < 0)
        {
            fprintf(stderr, "Error in Accepting connection !\n");
            exit(1);
        }
        else
        {
            Connected_socketid[i] = client_socketId; // Storing accepted client into array
            // opened sockets
        }

        // Getting IP address and port number of client
        struct sockaddr_in *client_pt = (struct sockaddr_in *)&client_addr;
        struct in_addr ip_addr = client_pt->sin_addr;
        char str[INET_ADDRSTRLEN];                          // INET_ADDRSTRLEN: Default ip address size
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN); // convert ipaddr to standard readable format
        printf("Client is connected with port number: %d and ip address: %s \n", ntohs(client_addr.sin_port), str);
        //printf("Socket values of index %d in main function is %d\n",i, client_socketId);
		
        
        
        pthread_create(&tid[i],NULL,thread_fn, (void*)&Connected_socketId[i]); // Creating a thread for each client accepted
		// This Thread is responsible for handling request and response for the its client
		i++; 
         
    }
    close(proxy_socketId);									// Close socket
 	return 0;
    
}
