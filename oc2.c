#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

volatile sig_atomic_t was_sighup = 0;

void sighup_handler(int sig) 
{
    was_sighup = 1;
}

int create_server_socket(uint16_t port) 
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) 
    {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        perror("bind");
        close(fd);
        exit(1);
    }

    if (listen(fd, 16) < 0) 
    {
        perror("listen");
        close(fd);
        exit(1);
    }

    printf("Listening on port %u\n", port);
    fflush(stdout);
    return fd;
}

int main()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;              
    if (sigaction(SIGHUP, &sa, NULL) < 0) 
    {
        perror("sigaction");
        exit(1);
    }

    sigset_t blocked_mask;
    sigset_t orig_mask;

    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);

    if (sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask) < 0) 
    {
        perror("sigprocmask");
        exit(1);
    }

    int server = create_server_socket(2560);
    int client = -1;

    for (;;) 
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(server, &rfds);

        int maxfd = server;
        if (client != -1)
        {
            FD_SET(client, &rfds);
            if (client > maxfd) maxfd = client;
        }

        int ready = pselect(maxfd + 1, &rfds, NULL, NULL, NULL, &orig_mask);
        if (ready < 0)
        {
            if (errno == EINTR) 
            {
                if (was_sighup) 
                {
                    printf("Received SIGHUP\n");
                    was_sighup = 0;
                }
                continue;
            }
            perror("pselect");
            break;
        }

        if (was_sighup) 
        {
            printf("Received SIGHUP\n");
            was_sighup = 0;
        }

        if (FD_ISSET(server, &rfds)) 
        {
            int newfd = accept(server, NULL, NULL);
            if (newfd < 0)
            {
                perror("accept");
            } else {
                if (client == -1) 
                {
                    client = newfd;
                    printf("New client accepted (fd=%d)\n", client);
                } else 
                {
                    printf("Second client rejected (only one allowed)\n");
                    close(newfd);
                }
            }
        }

        if (client != -1 && FD_ISSET(client, &rfds)) 
        {
            char buf[4096];
            ssize_t n = read(client, buf, sizeof(buf));

            if (n <= 0)
            {
                printf("Client disconnected\n");
                close(client);
                client = -1;
            } else 
            {
                printf("Received %zd bytes\n", n);
            }
        }
    }

    if (client != -1) close(client);
    close(server);
    return 0;
}
