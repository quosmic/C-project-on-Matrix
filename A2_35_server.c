// Raghav Gupta 2019B4A30927H
// Samyak Paharia 2020A8PS1824H
// Yash Saravgi 2019B2A31530H
// Anish Guruvelli 2020A2PS2418H
// Kashish 2019B2A31550H
// Zakia Firdous Munshi 2020A1PS2385H
// Adrian Jagnania 2019B5AA1498H
// Rahul Sahu 2019B4A30852H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "CSF372.h"

#define CONNECT_CHANNEL_KEY 1234
#define MAX_CLIENTS 100
#define MAX_NAME_LENGTH 32
#define SHM_SEG_SIZE 128
#define SUCCESS 0
#define ERR_NAME_NOT_UNIQUE -1
#define ERR_INVALID_KEY -2

typedef enum
{
    ACTION_REGISTER,
    ACTION_UNREGISTER,
    ACTION_CLIENT_REQUEST,
    ACTION_SERVER_RESPONSE,
    ACTION_NONE
} ActionType;

typedef enum
{
    REQ_EVEN_OR_ODD,
    REQ_ARTHMETICS,
    REQ_IS_PRIME,
    REQ_IS_NEGATIVE
} ClientRequestType;

typedef struct
{
    ActionType type;
    char name[MAX_NAME_LENGTH];
    int key;
    ClientRequestType request;
    int arg1;
    int arg2;
    char arg3;
} Message;

typedef struct
{
    pthread_mutex_t mutex;
    Message request;
    Message response;
} SharedMemorySegment;

typedef struct
{
    char name[MAX_NAME_LENGTH];
    int key;
    int shmid;
    int request_count;
} ClientInfo;

// clients[] = {1, 2, 4};
ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int generate_key()
{
    static int key = CONNECT_CHANNEL_KEY + 1;
    return key++;
}

// Find client in the clients array using the name
int find_client_by_name(const char *name)
{
    for (int i = 0; i < client_count; i++)
    {
        if (strcmp(clients[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Prints the summary
void print_summary(int key)
{
    int total_requests = 0;
    PRINT_INFO("***SUMMARY***\n\n");
    for (int i = 0; i < client_count; i++)
    {
        total_requests += clients[i].request_count;
        PRINT_INFO("Name: %s | Request Count: %d \n", clients[i].name, clients[i].request_count);
    }
    PRINT_INFO("Total Request Count: %d \n", total_requests);
}

// Find client in the clients array using the key
int find_client_by_key(int key)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].key == key)
        {
            return i;
        }
    }
    return -1;
}

int register_client(const char *name)
{
    // Mutex Lock for clients array
    pthread_mutex_lock(&client_list_mutex);

    //check if name is unique
    if (find_client_by_name(name) >= 0)
    {
        pthread_mutex_unlock(&client_list_mutex);
        return ERR_NAME_NOT_UNIQUE;
    }

    // generate new key using the below fn
    int key = generate_key();

    // creates new shared memory for that user
    int shmid = shmget(key, SHM_SEG_SIZE, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // pointer points to the SM address
    SharedMemorySegment *segment = (SharedMemorySegment *)shmat(shmid, NULL, 0);
    if (segment == (SharedMemorySegment *)(-1))
    {
        perror("shmat");
        exit(1);
    }

    // creates mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&segment->mutex, &attr);

    // add the name to clients[] array, adds key, shmid, request_count
    strncpy(clients[client_count].name, name, MAX_NAME_LENGTH);
    clients[client_count].key = key;
    clients[client_count].shmid = shmid;
    clients[client_count].request_count = 0;
    client_count++;

    // Unlock Mutex
    pthread_mutex_unlock(&client_list_mutex);

    return key;
}

void unregister_client(int key)
{   
    // Mutex Lock
    pthread_mutex_lock(&client_list_mutex);

    // Find the client using the key
    int index = find_client_by_key(key);

    //If Found
    if (index >= 0)
    {
        // Gets the shared memory id
        int shmid = clients[index].shmid;

        // Gets the address using the pointer
        SharedMemorySegment *segment = (SharedMemorySegment *)shmat(shmid, NULL, 0);

        // Destroy the mutex, shared memory
        pthread_mutex_destroy(&segment->mutex);
        shmdt(segment);
        shmctl(shmid, IPC_RMID, NULL);

        //Shifts each client 1 position backward from the index it was deleted
        for (int i = index; i < client_count - 1; i++)
        {
            clients[i] = clients[i + 1];
        }
        client_count--;
        PRINT_INFO("Server successfully unregistered client with key: %d\n", key);
    }

    //Unlock Mutex
    pthread_mutex_unlock(&client_list_mutex);
}

// Used to process different client request
void process_request(int key, Message *request, Message *response)
{
    // Find the index
    int client_index = find_client_by_key(key);
    if (client_index < 0)
    {
        response->type = ACTION_SERVER_RESPONSE;
        response->key = ERR_INVALID_KEY;
        return;
    }

    // Increase request_count
    clients[client_index].request_count++;

    // Set the response type to ACTION_SERVER_RESPONSE and key to SUCCESS
    response->type = ACTION_SERVER_RESPONSE;
    response->key = SUCCESS;

    // For each request we do the computation
    switch (request->request)
    {
    case REQ_ARTHMETICS:
        response->arg3 = 'V';
        if (request->arg3 == '+')
        {
            response->arg1 = request->arg1 + request->arg2;
        }
        else if (request->arg3 == '-')
        {
            response->arg1 = request->arg1 - request->arg2;
        }
        else if (request->arg3 == '/')
        {
            response->arg1 = request->arg1 / request->arg2;
        }
        else if (request->arg3 == '*')
        {
            response->arg1 = request->arg1 * request->arg2;
        } else {
            response->arg3 = 'I';
        }
        break;
    case REQ_IS_PRIME:
    {
        int n = request->arg1;
        int is_prime = 1;
        if (n < 2)
        {
            is_prime = 0;
        }
        else
        {
            for (int i = 2; i * i <= n; i++)
            {
                if (n % i == 0)
                {
                    is_prime = 0;
                    break;
                }
            }
        }
        response->arg1 = is_prime;
        break;
    }

    case REQ_EVEN_OR_ODD:
        response->arg1 = request->arg1 % 2;
        break;
    case REQ_IS_NEGATIVE:
        response->arg1 = 1;
        break;
    default:
        response->key = -1;
        break;
    }
}

void *client_thread(void *arg)
{
    // key from the arg
    int key = *((int *)arg);
    free(arg);

    // Infinite Loop
    while (1)
    {   
        // Find the client index
        int client_index = find_client_by_key(key);
        if (client_index < 0)
        {
            break;
        }

        // Gets the shared memory segment address for the client
        SharedMemorySegment *segment = (SharedMemorySegment *)shmat(clients[client_index].shmid, NULL, 0);

        // Mutex Lock
        pthread_mutex_lock(&segment->mutex);

        // Checks if the request type is ACTION_CLIENT_REQUEST
        if (segment->request.type == ACTION_CLIENT_REQUEST)
        {
            PRINT_INFO("Server received a service request on the comm channel from client: %d.\n", key);
            // We send each request to the process_request with key, request and response

            process_request(key, &segment->request, &segment->response);
            PRINT_INFO("Server responded to the client on the comm channel.\n");

            // Reset request type so that it can process other requests
            segment->request.type = ACTION_NONE; // Reset request type
        }

        // Unlock Mutex
        pthread_mutex_unlock(&segment->mutex);

        // Destroy shared memory
        shmdt(segment);
        usleep(10000); // Sleep for 10ms
    }

    return NULL;
}

// Creates a new thread and assign it to the client thread
void handle_client(const char *name, int *key_ptr)
{
    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, (void *)key_ptr);
}

int main()
{
    // Creates Shared Memory
    int shmid = shmget(CONNECT_CHANNEL_KEY, SHM_SEG_SIZE, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    // connect_channel pointer points to the shared memory defined by shmid
    SharedMemorySegment *connect_channel = (SharedMemorySegment *)shmat(shmid, NULL, 0);
    if (connect_channel == (SharedMemorySegment *)(-1))
    {
        perror("shmat");
        exit(1);
    }
    else
    {
        PRINT_INFO("Server initiated and created the connect channel. \n");
    }

    //Initializes the mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&connect_channel->mutex, &attr);

    // Infinite Loop
    while (1)
    {
        // Mutex Lock
        pthread_mutex_lock(&connect_channel->mutex);

        // Checks if the request type = reg
        if (connect_channel->request.type == ACTION_REGISTER)
        {
            PRINT_INFO("Server received a register request. \n");

            //calls the register_client function and it returns a key
            int key = register_client(connect_channel->request.name);

            //set the response type, key and resets the type
            connect_channel->response.type = ACTION_SERVER_RESPONSE;
            connect_channel->response.key = key;
            connect_channel->request.type = ACTION_NONE; // Reset request type

            // if key is positive we pass the key and name to the handle_client function
            if (key >= 0)
            {
                PRINT_INFO("Successful creation of comm channel for the client with key: %d\n", key);
                int *key_ptr = malloc(sizeof(int));
                *key_ptr = key;
                handle_client(connect_channel->request.name, key_ptr);
                PRINT_INFO("Successful response made to the client's register request.\n");
            }
        }
        // Checks if the request type = unreg
        else if (connect_channel->request.type == ACTION_UNREGISTER)
        {
            PRINT_INFO("Server received a unregister request. \n");

            // prints the summary
            print_summary(connect_channel->request.key);

            // unregister the user by passing the key to the unregister_client and reset request type
            unregister_client(connect_channel->request.key);
            connect_channel->request.type = ACTION_NONE; // Reset request type
        }

        // Unlock mutex
        pthread_mutex_unlock(&connect_channel->mutex);
        usleep(10000); // Sleep for 10ms
    }

    //If we are outside we destroy the mutext and clean up connect_channel
    pthread_mutex_destroy(&connect_channel->mutex);
    shmdt(connect_channel);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}