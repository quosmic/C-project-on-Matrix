//Group Number 35

// 2020A8PS1824H 			Samyak Paharia 	
// 	2019B4A30927H			Raghav Gupta	
// 	2019B2A31530H			Yash Saravgi	
// 	2019B4AA0918H			Mohit Agrawal	
// 	2019B2A31550H			Kashish 	
// 	2019B4A30852H			Rahul Sahu	
// 	2019B5AA1498H			Adrian Jagnania	
// 	2021A3PS2972H			Shantanu Wadhwa 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>
#include "CSF372.h"

#define MAX_NUM 100

int n, a, b, p;
int arr[MAX_NUM][MAX_NUM];
int row_sums[MAX_NUM];
int fd[MAX_NUM][2];

pid_t pid;

void errorHandler()
{
    PRINT_INFO("Error reported, killing worker process\n");
    kill(pid, SIGTERM);
}

void sigchldHandler(int sig)
{
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFSIGNALED(status))
        {
            PRINT_INFO("Child process %d terminated by signal %d\n", pid, WTERMSIG(status));
        }
    }

    if (pid == -1 && errno != ECHILD)
    {
        perror("waitpid");
    }
}

// Function to check if a number is prime
int isPrime(int num)
{
    if (num <= 1)
    {
        return 0;
    }
    if (num == 2 || num == 3)
    {
        return 1;
    }
    if (num % 2 == 0)
    {
        return 0;
    }
    for (int i = 3; i <= sqrt(num); i += 2)
    {
        if (num % i == 0)
        {
            return 0;
        }
    }
    return 1;
}

// Function to calculate the average of p prime numbers before and after x, including x
int calculateThapx(int x)
{
    int sum = 0, acount = 0, pcount = 0;
    for (int i = x - 1; i >= 2 && pcount < p; i--)
    {
        if (isPrime(i))
        {
            sum += i;
            pcount++;
            PRINT_INFO("A prime number is discovered before %d: %d\n", x, i);
        }
    }
    for (int i = x + 1; acount < p; i++)
    {
        if (isPrime(i))
        {
            sum += i;
            acount++;
            PRINT_INFO("A prime number is discovered after %d: %d\n", x, i);
        }
    }
    if (isPrime(x))
    {
        sum += x;
        acount++;
        PRINT_INFO("A prime number is discovered: %d\n", x);
    }

    if ((pcount + acount) == 0)// this condition follows when negative number is taken itno consideration
    {
        PRINT_INFO("No prime numbers found for x = %d.\n", x);
        return 0;
    }
    else
        PRINT_INFO("All prime numbers discovered for x = %d\n", x);
    return sum / (acount + pcount);// thapx is returned
}

// Worker thread function
void *workerThread(void *arg)// here thapx is calculated 
{
    int x = *((int *)arg);// element is recieved and we are dereferencing it
    uint32_t thapx = (uint32_t)calculateThapx(x);// typecasting is taking place from 4bytes(int) to 4bytes(unsigned int)
    PRINT_INFO("\nThe value of thapx is %d for x = %d\n\n", thapx, x);
    uintptr_t ptr_val = (uintptr_t)thapx;//uintptr is a 4 byte ptr which is used to store the address of thaps
    void *ptr = (void *)ptr_val;//typecasting of uintptr to void*
    return ptr;
}
// Worker process function
int workerProcess(int i)
{
    int thapx_sum = 0, count = 0;
    pthread_t threads[n];// for each row n threads are made that will deal with n column elements
    PRINT_INFO("Worker process %d starts execution\n", i);
    PRINT_INFO("Worker process %d is processing row %d\n", i, i);
    for (int j = 0; j < n; j++)
    {
        PRINT_INFO("Worker thread %d is created\n", j);
        pthread_create(&threads[j], NULL, workerThread, &arr[i][j]);// thread created for each element in the row, syntax is given and the value provided is arr[i][j] and worker thread function is called
        PRINT_INFO("Worker thread %d starts execution\n", j);
    }
    for (int j = 0; j < n; j++)
    {
        void *thapx;//4 byte ptr is declared
        pthread_join(threads[j], &thapx);//waits for thread to complete
        uintptr_t thapx_int = (uintptr_t)thapx;//converts 4 byte void ptr to 4 byte int ptr
        int x = (int)thapx_int;//typecasting of int ptr to int 
        thapx_sum += x;
        count++;
        PRINT_INFO("Worker thread %d terminates\n", j);
    }
    int wpapx = thapx_sum / count;// wpapx for a single row
    PRINT_INFO("All thapx values have been calculated for row %d\n", i);
    PRINT_INFO("\nThe value of wpapx is %d for row %d\n\n", wpapx, i);

    // Write the WPAPX value to the pipe
    close(fd[i][0]); // Close the read end of the pipe
    write(fd[i][1], &wpapx, sizeof(wpapx));
    PRINT_INFO("The wpapx value sent to controller for row %d\n", i);
    close(fd[i][1]); // Close the write end of the pipe

    return wpapx;
}

int main(int argc, char *argv[])
{
    int fapx = 0;

    // Set up SIGCHLD handler
    signal(SIGCHLD, sigchldHandler);

    // Parse the input
    if (argc < 5)// CHECKS FOR INPUT FORMAT IF ATLEAST THE INITIAL 4 INPUTS I.E 4+1=5 TOTAL IPS GIVEN
    {
        PRINT_INFO("Usage: %s n a b p x1 x2 ... xn\n", argv[0]);
        exit(1);
    }

    n = atoi(argv[1]); // ARG CONVERTS FROM STRING TO INT
    a = atoi(argv[2]);
    b = atoi(argv[3]);
    p = atoi(argv[4]);

    if (n < 4 || n > 10)// GIVEN CONDITION IN QUESTION
    {
        PRINT_INFO("Invalid value for n = %d\n", n);
        return 1;
    }

    if (n * n != argc - 5)//AGAIN THE CONDITION OF INCORRECT ELEMENTS IS BEING CHECKED
    {
        PRINT_INFO("Incorrect number of input values.\n");
        return 1;
    }

    int count = 0;
    int val;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            val = atoi(argv[5 + count]);// INITIALLY ASSIGNS X1 TO VAL AND CHECKS IF VAL IS BETWEEN A &B
            if (val < a || val > b)
            {
                PRINT_INFO("Value does not lie between the given limits: %d and %d\n", a, b);
                return 1;
            }
            arr[i][j] = val;// VALUES ARE ENTERED IN THE MATRIX, ARR GLOBALLY DEFINED
            count++;
        }
    }

    PRINT_INFO("Input values read successfully.\n");

    pid_t pid[n];// N CHILDREN AND PIDS OF EACH CHILD STORED IN ARRAY

    // Create n pipes
    for (int i = 0; i < n; i++)
    {
        if (pipe(fd[i]) == -1)// with this statement we are making n pipes and storing the file descriptor in a global array fd. if pipe formation fails, -1 is returned causing fail of pipe formation 
        {
            perror("Pipe creation failed");
            return 1;
        }
    }

    // Calculate row sums using child processes
    for (int i = 0; i < n; i++)
    {
        pid[i] = fork();
        if (pid[i] < 0)
        {
            perror("Fork failed");
            return 1;
        }
        else if (pid[i] == 0)
        {
            // In child process
            close(fd[i][0]);// read end closed / one end of the pipe should be closed as it is bidirectional
            int row_sum = 0;
            for (int j = 0; j < n; j++)
            {
                row_sum += arr[i][j];
            }
            write(fd[i][1], &row_sum, sizeof(row_sum));// we are inserting the row sum in the write end of the pipe. the following is the syntax of insertion
            close(fd[i][1]);
            exit(0);
        }
        else
        {
            signal(SIGUSR1, errorHandler);
            signal(SIGCHLD, sigchldHandler);
            //SIGCHLD signal is used to indicate that a child process has terminated.

            // Wait for the worker process to terminate
            pause();

            // In parent process
            close(fd[i][1]);
        }
    }

    // Read row sums from pipes
    int row_sums[n];
    for (int i = 0; i < n; i++)
    {
        read(fd[i][0], &row_sums[i], sizeof(row_sums[i]));// we wrote row sums in the pipe and now we are reading them and storing them in rowsums array
        close(fd[i][0]);
    }

    PRINT_INFO("Parent process starts execution\n");

    // Calculate fapx value using worker processes
    for (int i = 0; i < n; i++)
    {
        int wpapx = workerProcess(i); //for each row wpapx is calculated using worker process
        PRINT_INFO("The value of wpapx is captured by the controller for row %d\n", i);
        fapx += wpapx;
    }

    // Calculate the final fapx value
    fapx = fapx / n;
    PRINT_INFO("\n\nThe value of fapx is %d\n\n", fapx);
    PRINT_INFO("Parent process terminates.\n");

    return 0;
}
