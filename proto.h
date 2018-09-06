/**
 * @Author: lizhiheng <lzh>
 * @Date:   2018-09-02T22:42:34+08:00
 * @Email:  phoenix_lzh@sina.com
 * @Filename: proto.h
 * @Last modified by:   lzh
 * @Last modified time: 2018-09-06T16:19:36+08:00
 * @License: GPLV3
 */
#ifndef PROTO_H__
#define PROTO_H__

#include <stdint.h>

//服务器端端口
#define SERVERPORT  "6761"

#define SIZE        1024

//描述客户端发送给服务器端数据的结构体
struct msg_st
{
  uint32_t  num;
  uint8_t   str[SIZE];
}__attribute__((packed));

#endif
