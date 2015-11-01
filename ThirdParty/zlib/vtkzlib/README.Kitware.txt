This directory contains a subset of the zlib library (1.2.8) and
some custom changes.

We only include enough of the distribution to provide the functionalities
required.

We would like to thank the zlib team for distributing this library.
http://www.zlib.net

Added Files
-----------

vtkzlibConfig.h.in
  -Header file configured by cmake.

Changed Files
-----------

CMakeLists.txt
  -Simplify for building within VTK

zconf.h.cmakein
  -Mangles symbols exported from the zlib library for use by VTK.

zlib.h, gzread.c
  -Changed Z_PREFIX conditionals for VTK name mangling.
