#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int sockfd, n;
  int bufsize = 1024;
  int port = 20001;

  // Обработка аргументов командной строки
  static struct option options[] = {
    {"bufsize", required_argument, 0, 0},
    {"port", required_argument, 0, 0},
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

  char mesg[bufsize], ipadr[16];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    exit(1);
  }

  printf("UDP SERVER starts on port %d...\n", port);

  while (1) {
    unsigned int len = SLEN;

    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    mesg[n] = 0;

    printf("REQUEST: %s FROM %s:%d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      exit(1);
    }
  }
}
