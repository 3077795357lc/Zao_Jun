#ifndef ENV_STATUS_H
#define ENV_STATUS_H
#include "common_type.h"

#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void dht11_init(void);
int dht11_read(char *humi,char *temp);

#endif