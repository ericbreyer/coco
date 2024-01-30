#include "coco.h"
#include "semaphore.h"

void coco_sem_init(coco_sem *sem, int value) {
    *sem = value;
}
void coco_sem_wait(coco_sem *sem) {
    while (*sem <= 0) {
        coco_yield();
    }
    --*sem;
}
void coco_sem_post(coco_sem *sem) {
    ++*sem;
}
