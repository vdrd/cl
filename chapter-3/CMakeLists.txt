add_executable( histogram histogram.cpp ) 
target_link_libraries( histogram ${OPENCL_LIBRARIES} )  

add_executable( histogram_image histogram_image.cpp ) 
target_link_libraries( histogram_image ${OPENCL_LIBRARIES} )  

add_executable( bufferRect bufferRect.cpp ) 
target_link_libraries( bufferRect ${OPENCL_LIBRARIES} )  

add_executable( copyRectangular copyRectangular.cpp ) 
target_link_libraries( copyRectangular ${OPENCL_LIBRARIES} )  

