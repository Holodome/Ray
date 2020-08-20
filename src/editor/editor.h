#if !defined(EDITOR_H)

#include "lib/common.h"
#include "lib/sys.h"

#include "ray/ray_tracer.h"
#include "ray/scene_file.h"

typedef struct {
    char *current_scene_filename;    
    
    Scene ray_tracer_scene;
} Editor;

#define EDITOR_H 1
#endif
