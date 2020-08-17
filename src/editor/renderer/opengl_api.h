#if !defined(OPENGL_API_H)

#include "thirdparty/glcorearb.h"

typedef struct {

#define GLProc(name, type) type name;
#include "editor/renderer/gl_proc_list.inc"
#undef GLProc

} OpenGLApi;

#define OPENGL_API_H 1
#endif
