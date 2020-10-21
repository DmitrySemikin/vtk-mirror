# CGNS 

## About

The CFD General Notation System (CGNS) provides a standard for recording and recovering computer data associated with the numerical solution of fluid dynamics equations.

## Bugs/Feature and issue tracking

https://cgnsorg.atlassian.net

## Installation

### Installation Instructions using `cmake`

1. Install HDF5 on your system.
  
   - HDF5 can use the standard GNU autotools, so `./configure`, `make`, `sudo make install` should install HDF5 without problems on most systems.
2. Unpack the tar ball containing the source code into some directory.
3. Create a new directory in which to build the library.
4. Use `cmake` to initialize the build tree.
   ```shell
   user@hostname:build_path$ cmake /path/to/cgns/sources/
   ```
5. Use `ccmake` to edit the control variables as needed.
   ```shell
   user@hostname:build_path$ ccmake .
   ```
   - The path to the HDF5 library should be specified with `CMAKE_PREFIX_PATH=$HDF_DIR` for linking with a specific HDF5 version.
     - If HDF5 is built with parallel-IO support via MPI, the `HDF5_NEEDS_MPI` flag must be set to `true`.
     - If HDF5 is built with `zlib` and `szip` support, these need to be flagged with `HDF5_NEEDS_ZLIB` and `HDF5_NEEDS_SZIP` as well as the paths for those libraries.
   - Fortran can be enabled by toggling the `CGNS_ENABLE_FORTRAN` variable.
     - A view of the attempt to autodetect the correct interface between Fortran and C is show, setting the value of `FORTRAN_NAMING`.
     - For `gfortran` and `pgf90` the value of `FORTRAN_NAMING` shoud be `LOWERCASE_`.
   - The build system must be reconfigured after variable changes by pressing `c`. Variables who's value has changed are maked with a `*` in the interface.
   - After configuration, the `Makefile`s must be generated by pressing `g`.
6. Use `make` to build the library.
   ```shell
   user@hostname:build_path$ make
   ```
   - A colorized review of the build process should follow.
7. Installation of the library is accomplished with the `install` target of the makefile.
   ```shell
   user@hostname:build_path$ make install
   ```
   - You must have permissions to alter the directory where CGNS will be installed.

### Installation Instructions using `make`

1. Install HDF5 on your system.
   - HDF5 can use the standard GNU autotools, so `./configure`, `make`, `sudo make install` should install HDF5 without problems on most systems.
2. Typically the standard `./configure`, `make`, `make install` will suffice.  
3. Sample scripts for building parrallel CGNS can be found in `src/SampleScripts`.

## Usage

## License

The distribution and use of the CGNS software is covered by the
following license:

-----------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3.  This notice may not be removed or altered from any source distribution.

----------------------------------------------------------------------

This license is borrowed from the zlib/libpng License:

    http://www.opensource.org/licenses/zlib-license.php

and supercedes the GNU Lesser General Public License (LGPL) which
previously governed the use and distribution of the software.

For details on the policy governing the distribution of the CGNS
standard and software see:

    http://www.grc.nasa.gov/www/cgns/charter/principles.html

## Development
CGNS uses the branching/release model as summarized at:

http://nvie.com/posts/a-successful-git-branching-model/
  

![image](https://github.com/CGNS/cgns.github.io/blob/master/git-model.png)