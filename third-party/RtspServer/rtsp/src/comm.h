/*
 * Author: 清水
 */

#ifndef __COMM_H__
#define __COMM_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define dbg(fmt, ...) do {("[DEBUG %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define info(fmt, ...) do {printf("[INFO  %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define warn(fmt, ...) do {printf("[WARN  %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define err(fmt, ...) do {printf("[ERROR %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)

#endif

