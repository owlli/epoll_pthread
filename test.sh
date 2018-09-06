#!/bin/bash
# @Author: lizhiheng <lzh>
# @Date:   2018-09-04T16:42:58+08:00
# @Email:  phoenix_lzh@sina.com
# @Filename: test.sh
# @Last modified by:   lzh
# @Last modified time: 2018-09-05T22:43:53+08:00
# @License: GPLV3

#需要先在一个终端上运行server

#read -p "please input server's ip: " ip
for i in $(seq -f "NO.%03g"1 100)
do
#  ./client $ip $i
  nohup ./client 127.0.0.1 $i &
  sleep 0.2
done
