target_include_directories(coco PUBLIC signals channel waitgroup vmac)
set(coco_subdirs waitgroup signals channel)
foreach(entry IN LISTS coco_subdirs)
    add_subdirectory(${entry})
endforeach()

target_sources(coco PRIVATE coco_config.h coco.h coco.c)