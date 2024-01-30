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

void f(void *) {
    printf("once\n");
    int tid = coco_fork();
    printf("twice 1\n");
    if (tid) {
        printf("parent, childs tid %d\n", tid);
        coco_waitpid(tid, NULL, 0);
    } else {
        printf("child\n");
    }
    printf("twice 2\n");
    coco_exit(0);
}

void kernal() {
    int tid = add_task(f, NULL);
    coco_waitpid(tid, NULL, 0);
    coco_exit(0);
}

int main() { coco_start(kernal, NULL); }