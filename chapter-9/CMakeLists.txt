include_directories( "C:/Users/rbanger/Documents/AMD APP/samples/opencl/SDKUtil/include" )

set( gaussian.Source ../common/bmp_image.cpp gaussian.cpp ../common/Timer.cpp)
set( gaussian.Headers ../include/bmp_image.h gaussian_kernel.h gaussian.h ../include/Timer.h )
set( gaussian.Files ${gaussian.Source} ${gaussian.Headers} )
add_executable( gaussian  ${gaussian.Files} )
target_link_libraries( gaussian ${OPENCL_LIBRARIES} )

set( sobel.Source ../common/bmp_image.cpp sobel.cpp ../common/Timer.cpp)
set( sobel.Headers ../include/bmp_image.h sobel_kernel.h sobel.h ../include/Timer.h )
set( sobel.Files ${sobel.Source} ${sobel.Headers} )
add_executable( sobel  ${sobel.Files} )
target_link_libraries( sobel ${OPENCL_LIBRARIES} )

set( median.Source ../common/bmp_image.cpp median.cpp ../common/Timer.cpp)
set( median.Headers ../include/bmp_image.h median_kernel.h median.h ../include/Timer.h )
set( median.Files ${median.Source} ${median.Headers} )
add_executable( median  ${median.Files} )
target_link_libraries( median ${OPENCL_LIBRARIES} )