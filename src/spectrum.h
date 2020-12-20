#if !defined(SPECTRUM_H)

#include "general.h"

#define SPECTRUM_RGB        1
#define SPECTRUM_WAVELENGTH 1

#if !defined(SPECTRUM_MODE)
#define SPECTRUM_MODE SPECTRUM_RGB
#endif 

#if SPECTRUM_MODE == SPECTRUM_RGB

#include "ray_math.h"
typedef Vec3 Spectrum;

#elif SPECTRUM_MODE == SPECTRUM_WAVELENGTH
#else 
#error SPECTRUM_MODE is illdefined
#endif 

#define SPECTRUM_H 1
#endif
