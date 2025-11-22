#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  int lfd, cfd;
  int nread;
  int bufsize = 100;
  int port = 10050;
  int backlog = 5;

  // Обработка аргументов командной строки
  static struct option options[] = {
    {"bufsize", required_argument, 0, 0},
    {"port", required_argument, 0, 0},
    {"backlog", required_argument, 0, 0},
    {0, 0, 0, 0}
  };

  int option_index = 0;
  while (1) {
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            bufsize = atoi(optarg);
            if (bufsize <= 0) {
              fprintf(stderr, "bufsize must be positive\n");
              exit(1);
            }
            break;
          case 1:
            port = atoi(optarg);
            if (port <= 0) {
              fprintf(stderr, "port must be positive\n");
              exit(1);
            }
            break;
          case 2:
            backlog = atoi(optarg);
            if (backlog <= 0) {
              fprintf(stderr, "backlog must be positive\n");
              exit(1);
            }
            break;
          default:
            fprintf(stderr, "Invalid option\n");
            exit(1);
        }
        break;
      default:
        fprintf(stderr, "getopt error\n");
        exit(1);
    }
  }

  const size_t kSize = sizeof(struct sockaddr_in);
  char buf[bufsize];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(lfd, backlog) < 0) {
    perror("listen");
    exit(1);
  }

  printf("TCP Server listening on port %d\n", port);

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      exit(1);
    }
    printf("Connection established\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    close(cfd);
    printf("Connection closed\n");
  }
}
