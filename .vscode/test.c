#if defined(__has_include)
#  if __has_include("SDL3/SDL.h")
#    include "SDL3/SDL.h"
#  elif __has_include("SDL.h")
#    include "SDL.h"
#  else
#    error "SDL header not found; please install SDL3 development headers and add their include path to your project."
#  endif
#else
#  include "SDL.h"
#endif
