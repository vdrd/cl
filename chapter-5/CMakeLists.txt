add_executable( create_binary create_binary.cpp )
target_link_libraries( create_binary ${OPENCL_LIBRARIES} )
add_executable( built_in_kernels built_in_kernels.cpp )
target_link_libraries( built_in_kernels ${OPENCL_LIBRARIES} )

