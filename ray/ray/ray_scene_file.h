#if !defined(RAY_SCENE_FILE_H)

#include "lib/common.h"

#include "ray/ray_tracer.h"

// Parses text and generates scene from it.
Scene make_scene_from_file_description(char *data, u64 data_size);
// Saves scene as file
void  save_scene_to_file_description(Scene *scene, char *filename);

#define RAY_SCENE_FILE_H 1
#endif
