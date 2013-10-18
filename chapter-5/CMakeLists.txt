add_executable( chapter5.create_binary create_binary.cpp )
target_link_libraries( chapter5.create_binary ${OPENCL_LIBRARIES} )

add_executable( chapter5.built_in_kernels built_in_kernels.cpp )
target_link_libraries( chapter5.built_in_kernels ${OPENCL_LIBRARIES} )

