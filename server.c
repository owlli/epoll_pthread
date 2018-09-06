/**
 * @Author: lizhiheng <lzh>
 * @Date:   2018-09-02T22:28:56+08:00
 * @Email:  phoenix_lzh@sina.com
 * @Filename: server.c
 * @Last modified by:   lzh
 * @Last modified time: 2018-09-06T16:19:34+08:00
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
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

//定义这个宏之后,errno在多线程中是安全的,不同线程之间没有影响
#define _LIBC_REENTRANT
//获得线程号
#define gettid() syscall(__NR_gettid)

// sd和newsd通过fcntl设置为非阻塞模式,然后就errno就不会出现eintr了

// i的作用是记录是否将所有线程中的文件描述符从epoll队列中移除
//当epoll_wait返回时,每新建一个线程i的值就加1
//在新建线程中将此线程处理的socket从epoll队列中移除后再将i减1
//只有当i为0时,才进行下一次epoll_wait
int i, epfd, maxevents;
static pthread_mutex_t mut_num1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mut_num2 = PTHREAD_MUTEX_INITIALIZER;

//打印客户端发送的结构体内容
void printf_msg_st(struct msg_st msg_st_val) {
  // printf("num = %d\n", msg_st_val.num);
  printf("str = %s\n", msg_st_val.str);
}

//删除监控的epoll_event
int del_ep_sd(int ep_sd) {
  pthread_mutex_lock(&mut_num2);
  struct epoll_event ep;
  ep.data.fd = ep_sd;
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, ep_sd, &ep)) {
    printf("errno = %d\n", errno);
    printf("ep_sd = %d\n", ep_sd);
    perror("del_ep_sd,epoll_ctl()");
    return -1;
  }
  // pthread_mutex_lock(&mut_num);
  maxevents--;
  // pthread_mutex_unlock(&mut_num);
  printf("success delete %d ep_sd\n", ep_sd);
  pthread_mutex_unlock(&mut_num2);
  return 0;
}

//线程处理函数
void *udf_func(void *p) {
  int p_sd = *(int *)p;
  if (del_ep_sd(p_sd) < 0) {
    exit(1);
  }
  pthread_mutex_lock(&mut_num1);
  i--;
  pthread_mutex_unlock(&mut_num1);
  printf("entry udf_func,pthread id is %ld\n", gettid());
  //将线程设置为分离模式,exit后自动释放资源,不需要进程执行pthread_join
  pthread_detach(pthread_self());
  struct msg_st msg_st_val;

  size_t ret = 0, msg_st_len = sizeof(msg_st_val);
  while (1) {
    ret = recv(p_sd, &msg_st_val, msg_st_len, 0);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      printf("ret < 0,p_sd = %d\n", p_sd);
      close(p_sd);
      break;
    } else if (ret == 0) {
      printf("ret == 0,p_sd = %d\n", p_sd);
      // if (del_ep_sd(p_sd) < 0) {
      //   exit(1);
      // }
      if (close(p_sd)) {
        printf("close fail,pthread id is %ld\n", gettid());
        printf("close %d fail\n", p_sd);
        perror("close()");
        exit(1);
      }
      printf("close %d\n", p_sd);
      break;
    } else if (ret != msg_st_len) {
      continue;
    }
    msg_st_val.num = ntohl(msg_st_val.num);
    printf("read complete,ret = %d\n", (int)ret);
    printf_msg_st(msg_st_val);
    printf("p_sd = %d\n", p_sd);
    printf("maxevents = %d\n", maxevents);
  }
  printf("child pthread complete\n\n");
  // sleep(10);
  pthread_exit(NULL);
}

//将文件描述符sd设置为非阻塞模式
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
  int nfd;
  int n = 0;
  pthread_t tid;
  int err;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    perror("socket()");
    exit(1);
  }
  // sd必须为非阻塞socket,否则如果在accept上发生阻塞
  //程序就不会执行到epoll_wait,从而无法处理其他可读事件
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
  maxevents = 1;
  while (1) {
    // pthread_mutex_lock(&mut_num1);
    while ((nfd = epoll_wait(epfd, eps, maxevents, -1)) < 0) {
      if (errno == EINTR)
        continue;
      perror("epoll_wait()");
      exit(1);
    }
    printf("epoll_wait complete\n");
    for (n = 0; n < nfd; n++) {
      //如果发现可读事件发生在sd上,将accept后新建的newsd加入到监视列表
      if (eps[n].data.fd == sd) {
        //因为设置epoll为et模式,只有当监视的文件描述符从不可读变为可读才触发
        //为防止多个客户端同时connect,所以必须循环accept!!!
        //直到accept返回-1且errno为EGAIN或返回0,处理完TCP就绪队列中的所有连接后再退出循环
        while ((newsd = accept(sd, (void *)&raddr, &raddr_len)) > 0) {
          printf("a new client has connected\n");
          printf("newsd = %d\n", newsd);
          setnonblocking(newsd);
          ep.data.fd = newsd;
          ep.events = EPOLLIN | EPOLLET;
          pthread_mutex_lock(&mut_num2);
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, newsd, &ep) < 0) {
            perror("epoll_ctl()");
            exit(1);
          }
          // pthread_mutex_lock(&mut_num);
          maxevents++;
          pthread_mutex_unlock(&mut_num2);
        }
        if (newsd < 0 && errno != EAGAIN) {
          perror("accept()");
          exit(1);
        }
        // pthread_mutex_unlock(&mut_num1);
      }
      //否则创建新线程处理客户端send来的数据
      else {
        // pthread_mutex_unlock(&mut_num);
        pthread_mutex_lock(&mut_num1);
        i++;
        pthread_mutex_unlock(&mut_num1);
        printf("start newpthread,eps[%d].data.fd = %d\n", n, eps[n].data.fd);
        err = pthread_create(&tid, NULL, udf_func, (void *)&eps[n].data.fd);
        if (err) {
          fprintf(stderr, "pthread_create():%s\n", strerror(err));
          exit(1);
        }
        // pthread_mutex_lock(&mut_num);
      }
    }
    //当所有线程中将此线程处理的socket从epoll队列中移除后,才进行epoll_wait
    //如果没有移除,在客户端断开连接时,又会触发epoll的可读事件
    //程序会再次给此socket分配一个进程,这个进程从socket中会读到0字节数据,接着关闭此socket
    //而之前处理此socket的进程也会关闭一次此socket,造成关闭此socket两次的bug!!!
    //所以,当有线程处理一个socket时,必须将此socket移除epoll监视列表再epoll_wait!!!
    while (i)
      ;
    // pthread_mutex_unlock(&mut_num1);
  }

  close(epfd);
  close(sd);
  pthread_mutex_destroy(&mut_num1);
  pthread_mutex_destroy(&mut_num2);

  return 0;
}
