add_executable( linear_regression linear_regression.cpp )
target_link_libraries( linear_regression ${OPENCL_LIBRARIES} )

add_executable( parabolic_regression parabolic_regression.cpp )
target_link_libraries( parabolic_regression ${OPENCL_LIBRARIES} )

add_executable( bitonic_sort bitonic_sort.cpp )
target_link_libraries( bitonic_sort ${OPENCL_LIBRARIES} )

add_executable( knn_classification knn_classification.cpp )
target_link_libraries( knn_classification ${OPENCL_LIBRARIES} )
