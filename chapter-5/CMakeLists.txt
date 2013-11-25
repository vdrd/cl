add_executable( chapter5.create_binary create_binary.cpp )
target_link_libraries( chapter5.create_binary ${OPENCL_LIBRARIES} )

add_executable( chapter5.binary_saxpy binary_saxpy.cpp )
target_link_libraries( chapter5.binary_saxpy ${OPENCL_LIBRARIES} )

