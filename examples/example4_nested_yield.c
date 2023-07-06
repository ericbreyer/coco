/**
 * @file example1.c
 * @author Eric Breyer (ericbreyer.com)
 * @brief Demo of use of coco constructs
 * @version 0.2
 * @date 2023-05-27
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coco.h"

static int c = 10;
void sleep() {
    printf("tick %d\n", c);
    yieldForS(1);
    if(c-- <= 0) {
        return;
    }
    sleep();
}

void kernal() {
    sleep();
    coco_exit(0);
}

int main() { coco_start(kernal); }