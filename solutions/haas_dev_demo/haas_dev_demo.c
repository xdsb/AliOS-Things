/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "aos/init.h"
#include "board.h"
#include <aos/errno.h>
#include <aos/kernel.h>
#include <k_api.h>
#include <stdio.h>
#include <stdlib.h>

int application_start(int argc, char *argv[])
{
    int count = 0;

    printf("haas dev demp entry here!\r\n");

    while (1) {
        aos_msleep(10000);
    };
}
