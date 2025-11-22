#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int success;
};

void* ConnectToServer(void* args) {
  struct ThreadArgs* thread_args = (struct ThreadArgs*)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    thread_args->success = 0;
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(thread_args->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Socket creation failed for %s:%d!\n", 
            thread_args->server.ip, thread_args->server.port);
    thread_args->success = 0;
    return NULL;
  }

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection to %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sockfd);
    thread_args->success = 0;
    return NULL;
  }

  printf("Connected to server %s:%d, sending range %llu-%llu mod %llu\n", 
         thread_args->server.ip, thread_args->server.port, 
         thread_args->begin, thread_args->end, thread_args->mod);

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sockfd, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send to %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sockfd);
    thread_args->success = 0;
    return NULL;
  }

  char response[sizeof(uint64_t)];
  int bytes_received = recv(sockfd, response, sizeof(response), 0);
  if (bytes_received < sizeof(uint64_t)) {
    fprintf(stderr, "Receive from %s:%d failed (got %d bytes, expected %lu)\n", 
            thread_args->server.ip, thread_args->server.port, 
            bytes_received, sizeof(uint64_t));
    close(sockfd);
    thread_args->success = 0;
    return NULL;
  }

  memcpy(&thread_args->result, response, sizeof(uint64_t));
  printf("Received from server %s:%d: %llu\n", 
         thread_args->server.ip, thread_args->server.port, thread_args->result);

  close(sockfd);
  thread_args->success = 1;
  
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value: %s\n", optarg);
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value: %s\n", optarg);
          return 1;
        }
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *file = fopen(servers_file, "r");
  if (!file) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server *servers = NULL;
  unsigned int servers_num = 0;
  char line[255];

  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\n")] = '\0';
    if (strlen(line) == 0) continue;

    char *colon = strchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Invalid server format: %s (expected ip:port)\n", line);
      continue;
    }

    *colon = '\0';
    char *ip = line;
    int port = atoi(colon + 1);

    if (port <= 0) {
      fprintf(stderr, "Invalid port in: %s\n", line);
      continue;
    }

    servers = realloc(servers, (servers_num + 1) * sizeof(struct Server));
    strncpy(servers[servers_num].ip, ip, sizeof(servers[servers_num].ip) - 1);
    servers[servers_num].ip[sizeof(servers[servers_num].ip) - 1] = '\0';
    servers[servers_num].port = port;
    servers_num++;
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file: %s\n", servers_file);
    free(servers);
    return 1;
  }

  printf("Found %u servers, starting parallel computation...\n", servers_num);

  pthread_t threads[servers_num];
  struct ThreadArgs thread_args[servers_num];
  
  for (unsigned int i = 0; i < servers_num; i++) {
    thread_args[i].server = servers[i];
    thread_args[i].begin = (k * i) / servers_num + 1;
    thread_args[i].end = (k * (i + 1)) / servers_num;
    thread_args[i].mod = mod;
    thread_args[i].result = 1;
    thread_args[i].success = 0;
    
    if (pthread_create(&threads[i], NULL, ConnectToServer, &thread_args[i])) {
      fprintf(stderr, "Error creating thread for server %s:%d\n", 
              servers[i].ip, servers[i].port);
      thread_args[i].success = 0;
    }
  }

  uint64_t total_result = 1;
  int successful_connections = 0;
  
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    
    if (thread_args[i].success) {
      total_result = MultModulo(total_result, thread_args[i].result, mod);
      successful_connections++;
    }
  }

  free(servers);

  if (successful_connections == 0) {
    fprintf(stderr, "No servers responded successfully\n");
    return 1;
  }

  printf("\nFinal result: %llu! mod %llu = %llu\n", k, mod, total_result);
  printf("Successfully used %d out of %u servers\n", successful_connections, servers_num);

  return 0;
}
