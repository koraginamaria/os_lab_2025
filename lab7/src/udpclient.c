#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int sockfd, n;
  int bufsize = 1024;
  int port = 20001;
  char *ip = NULL;

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

  // IP из позиционных аргументов
  if (optind < argc) {
    ip = argv[optind++];
  }

  if (ip == NULL) {
    fprintf(stderr, "Usage: %s <IP> [--bufsize N] [--port N]\n", argv[0]);
    exit(1);
  }

  char sendline[bufsize], recvline[bufsize + 1];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      exit(1);
    }

    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      exit(1);
    }

    printf("REPLY FROM SERVER: %s\n", recvline);
  }
  
  close(sockfd);
  return 0;
}
