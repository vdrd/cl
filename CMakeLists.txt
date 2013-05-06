# This is the root cmake file. This one shall be used to create the 
# different project build related files

cmake_minimum_required( VERSION 2.8 )
project( OpenCL_By_Example )
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR} )

find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} )
SUBDIRS( chapter-1/ 
         chapter-2/ 
         chapter-3/ 
         chapter-4/
         chapter-6/
         chapter-7MM/
         lbm/
         test/ )
