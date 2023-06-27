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

void sleep() {
    static int c;
    printf("tick %d\n", c++);
    yieldForS(1);
    if(c == 10) {
        coco_exit(0);
    }
}

void kernal() {
    while (1) {
        sleep();
    }
}

int main() { coco_start(kernal); }