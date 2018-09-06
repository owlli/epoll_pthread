/**
 * @Author: lizhiheng <lzh>
 * @Date:   2018-09-02T23:28:07+08:00
 * @Email:  phoenix_lzh@sina.com
 * @Filename: client.c
 * @Last modified by:   lzh
 * @Last modified time: 2018-09-06T01:15:07+08:00
 * @License: GPLV3
 */
#include "proto.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int sd;
  struct sockaddr_in raddr;
  struct msg_st msg_st_val;
  size_t msg_st_len = sizeof(msg_st_val);

  srand(time(NULL));

  if (argc < 2) {
    fprintf(stderr, "Usage...\n");
    exit(1);
  }

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    perror("socket()");
    exit(1);
  }

  raddr.sin_family = AF_INET;
  raddr.sin_port = htons(atoi(SERVERPORT));
  inet_pton(AF_INET, argv[1], &raddr.sin_addr);
  if (connect(sd, (void *)&raddr, sizeof(raddr)) < 0) {
    perror("connect");
    exit(1);
  }

  msg_st_val.num = htonl(rand() % 100);
  memset(msg_st_val.str, '\0', msg_st_len);
  strcpy((char *)msg_st_val.str, argv[2]);

  if (send(sd, &msg_st_val, msg_st_len, 0) < 0) {
    perror("send()");
    exit(1);
  }
  // sleep(50);
  // close(sd);
  return 0;
}
