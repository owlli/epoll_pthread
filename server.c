/**
 * @Author: lizhiheng <lzh>
 * @Date:   2018-09-02T22:28:56+08:00
 * @Email:  phoenix_lzh@sina.com
 * @Filename: server.c
 * @Last modified by:   lzh
 * @Last modified time: 2018-09-04T01:44:11+08:00
 * @License: GPLV3
 */
#include "proto.h"
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define _LIBC_REENTRANT

// sd和newsd通过fcntl设置为非阻塞模式,然后就errno就不会出现eintr了

int epfd, nfd;
static pthread_mutex_t mut_num = PTHREAD_MUTEX_INITIALIZER;

//打印客户端发送的结构体内容
void printf_msg_st(struct msg_st msg_st_val) {
  printf("num = %d\n", msg_st_val.num);
  printf("str = %s\n", msg_st_val.str);
}

//删除监控的epoll_event
int del_ep_sd(int ep_sd) {
  struct epoll_event ep;
  ep.data.fd = ep_sd;
  ep.events = EPOLLIN;
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, ep_sd, &ep)) {
    perror("epoll_ctl()");
    return -1;
  }
  pthread_mutex_lock(&mut_num);
  nfd--;
  pthread_mutex_unlock(&mut_num);
  return 0;
}

//用户自定义函数
void *udf_func(void *p) {
  pthread_detach(pthread_self());
  struct msg_st msg_st_val;
  int p_sd = *(int *)p;

  size_t ret = 0, msg_st_len = sizeof(msg_st_val);
  while (1) {
    ret = recv(p_sd, &msg_st_val, msg_st_len, 0);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      if (del_ep_sd(p_sd) < 0) {
        exit(1);
      }
      close(p_sd);
      break;
    } else if (ret == 0) {
      if (del_ep_sd(p_sd) < 0) {
        exit(1);
      }
      close(p_sd);
      break;
    } else if (ret != msg_st_len) {
      continue;
    }
    msg_st_val.num = ntohl(msg_st_val.num);
    printf_msg_st(msg_st_val);
  }

  sleep(5);
  pthread_exit(NULL);
}

void setnonblocking(int sd) {
  int sd_old, sd_new;
  sd_old = fcntl(sd, F_GETFL);
  if (sd_old < 0) {
    perror("fcntl(F_GETFL)");
    exit(1);
  }
  sd_new = fcntl(sd, F_SETFL, sd_old | O_NONBLOCK);
  if (sd_new < 0) {
    perror("fcntl(F_SETFL)");
    exit(1);
  }
}

int main() {
  int sd, newsd;
  struct sockaddr_in laddr, raddr;
  socklen_t raddr_len;
  struct epoll_event ep, eps[SIZE];
  // int epfd,nfd;
  int n = 0;
  pthread_t tid;
  int err;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    perror("socket()");
    exit(1);
  }
  setnonblocking(sd);

  int val = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
    perror("setsockopt()");
    exit(1);
  }

  laddr.sin_family = AF_INET;
  laddr.sin_port = htons(atoi(SERVERPORT));
  inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
  if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0) {
    perror("bind()");
    exit(1);
  }

  if (listen(sd, 100) < 0) {
    perror("listen()");
    exit(1);
  }
  epfd = epoll_create(100);
  if (epfd < 0) {
    perror("epoll_create");
    exit(1);
  }
  ep.data.fd = sd;
  ep.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ep) < 0) {
    perror("epoll_ctl()");
    exit(1);
  }
  nfd = 1;
  while (1) {
    while (epoll_wait(epfd, eps, nfd, -1) < 0) {
      if (errno == EINTR)
        continue;
      perror("epoll_wait()");
      exit(1);
    }
    for (n = 0; n < nfd; n++) {
      //如果发现可读事件发生在sd上,将accept后新建的newsd加入到监视列表
      if (eps[n].data.fd == sd && eps[n].events & EPOLLIN) {
        while ((newsd = accept(sd, (void *)&raddr, &raddr_len)) > 0) {
          printf("a new client has connected\n");
          setnonblocking(newsd);
          ep.data.fd = newsd;
          ep.events = EPOLLIN | EPOLLET;
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, newsd, &ep) < 0) {
            perror("epoll_ctl()");
            exit(1);
          }
          pthread_mutex_lock(&mut_num);
          nfd++;
          pthread_mutex_unlock(&mut_num);
        }
        if (newsd < 0) {
          if (errno == EAGAIN)
            continue;
          perror("accept()");
          break;
        }
      } else if (eps[n].events & EPOLLIN) {
        err = pthread_create(&tid, NULL, udf_func, (void *)&eps[n].data.fd);
        if (err) {
          fprintf(stderr, "pthread_create():%s\n", strerror(err));
          exit(1);
        }
      }
    }
  }

  sleep(1);
  return 0;
}
