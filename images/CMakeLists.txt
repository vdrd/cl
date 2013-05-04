add_executable( getSupportedImageFormats getSupportedImageFormats.cpp ) 
target_link_libraries( getSupportedImageFormats ${OPENCL_LIBRARIES} )  

add_executable( histogram_image histogram_image.cpp ) 
target_link_libraries( histogram_image ${OPENCL_LIBRARIES} )  
