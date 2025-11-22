#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int fd;
  int nread;
  int bufsize = 100;
  int port = 0;
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

  // IP и порт из позиционных аргументов
  if (optind < argc) {
    ip = argv[optind++];
  }
  if (optind < argc && port == 0) {
    port = atoi(argv[optind++]);
  }

  if (ip == NULL || port == 0) {
    fprintf(stderr, "Usage: %s <IP> <port> [--bufsize N] [--port N]\n", argv[0]);
    exit(1);
  }

  char buf[bufsize];
  struct sockaddr_in servaddr;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  servaddr.sin_port = htons(port);

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }

  write(1, "Input message to send\n", 22);
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
  }

  close(fd);
  exit(0);
}
