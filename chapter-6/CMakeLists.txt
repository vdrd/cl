add_executable( profiling_saxpy profiling_saxpy.cpp )
target_link_libraries( profiling_saxpy ${OPENCL_LIBRARIES} )
