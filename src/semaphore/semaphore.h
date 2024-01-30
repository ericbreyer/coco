typedef unsigned int coco_sem;

void coco_sem_init(coco_sem *sem, int value);
void coco_sem_wait(coco_sem *sem);
void coco_sem_post(coco_sem *sem);
