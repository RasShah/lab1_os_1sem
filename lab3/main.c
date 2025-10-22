#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

void process_exit_handler(void)
{
    printf("Exit handler executed for process with PID: %d\n", (int)getpid());
}

void sigint_custom_handler(int sig_num)
{
    printf("Process %d: Received SIGINT signal (number %d). Terminating process.\n", (int)getpid(), sig_num);
    exit(0);
}

void sigterm_custom_handler(int sig_num)
{
    printf("Process %d: Received SIGTERM signal (number %d). Terminating process.\n", (int)getpid(), sig_num);
    exit(0);
}

int main(void)
{
    pid_t fork_result;
    int wait_status;

    if (atexit(process_exit_handler) != 0) {
        perror("atexit registration failed");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sigint_custom_handler) == SIG_ERR) {
        perror("signal for SIGINT failed");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_struct;
    memset(&sa_struct, 0, sizeof(sa_struct));
    sa_struct.sa_handler = sigterm_custom_handler;
    sigemptyset(&sa_struct.sa_mask);
    sa_struct.sa_flags = 0;

    if (sigaction(SIGTERM, &sa_struct, NULL) == -1) {
        perror("sigaction for SIGTERM failed");
        exit(EXIT_FAILURE);
    }

    printf("Parent process started. PID: %d, PPID: %d\n", (int)getpid(), (int)getppid());

    fork_result = fork();

    if (fork_result == -1) {
        perror("fork system call failed");
        exit(EXIT_FAILURE);
    }
    else if (fork_result == 0) {
        printf("Child process running. PID: %d, PPID: %d\n", (int)getpid(), (int)getppid());
        sleep(2);
        printf("Child process completed\n");
        exit(42);
    }
    else {
        printf("Parent process: Created child process with PID: %d\n", (int)fork_result);

        if (wait(&wait_status) == -1) {
            perror("wait system call failed");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(wait_status)) {
            printf("Parent process: Child exited with status: %d\n", WEXITSTATUS(wait_status));
        } else if (WIFSIGNALED(wait_status)) {
            printf("Parent process: Child terminated by signal: %d\n", WTERMSIG(wait_status));
        } else {
            printf("Parent process: Child process status unknown\n");
        }

        printf("Parent process completed\n");
    }

    return 0;
}