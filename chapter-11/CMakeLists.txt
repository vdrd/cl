add_executable( linear_regression linear_regression.cpp )
target_link_libraries( linear_regression ${OPENCL_LIBRARIES} )
add_executable( parabolic_regression parabolic_regression.cpp )
target_link_libraries( parabolic_regression ${OPENCL_LIBRARIES} )
