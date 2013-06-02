# This is the root cmake file. This one shall be used to create the 
# different project build related files

cmake_minimum_required( VERSION 2.8 )
project( OpenCL_By_Example )
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR} )
set( OPENGL_INCLUDE_DIRS ${AMDAPPSDKSAMPLESROOT}/include/ )
set( OPENGL_LIBRARIES    ${AMDAPPSDKSAMPLESROOT}/lib/x86_64/glut64.lib ${AMDAPPSDKSAMPLESROOT}/lib/x86_64/glew64.lib )
find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} include ${OPENGL_INCLUDE_DIRS})
SUBDIRS( chapter-1/ 
         chapter-2/ 
         chapter-3/ 
         chapter-4/
         chapter-6/
         chapter-7MM/
         images/
         lbm/
         test/
         gaussian/
         cl_gl_interop/
         )
