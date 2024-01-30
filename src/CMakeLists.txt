target_include_directories(coco PUBLIC channel waitgroup semaphore)
set(coco_subdirs waitgroup channel semaphore)
foreach(entry IN LISTS coco_subdirs)
    add_subdirectory(${entry})
endforeach()

target_sources(coco PRIVATE coco_config.h coco.h coco.c)