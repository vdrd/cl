add_executable( chapter4.getSupportedImageFormats getSupportedImageFormats.cpp )
target_link_libraries( chapter4.getSupportedImageFormats ${OPENCL_LIBRARIES} )

add_executable( chapter4.histogram_image histogram_image.cpp bmp_image.cpp )
target_link_libraries( chapter4.histogram_image ${OPENCL_LIBRARIES} )
