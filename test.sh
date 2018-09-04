#!/bin/bash
# @Author: lizhiheng <lzh>
# @Date:   2018-09-04T16:42:58+08:00
# @Email:  phoenix_lzh@sina.com
# @Filename: test.sh
# @Last modified by:   lzh
# @Last modified time: 2018-09-04T17:00:04+08:00
# @License: GPLV3

#需要先在一个终端上运行server

read -p "please input server's ip: " ip
for i in $(seq -f "NO.%03%g"1 100)
do
  ./client $ip $i
  sleep 1
done
