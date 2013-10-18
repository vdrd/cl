include_directories( "C:/Users/rbanger/Documents/AMD APP/samples/opencl/SDKUtil/include" )
set( JPEGDecoder.Source Framewave.cpp APP0.cpp debug.cpp image.cpp JPEG_Decoder.cpp scan.cpp SOF0.cpp)
set( JPEGDecoder.Headers APP0.h debug.h Framewave.h image.h init.h markers.h SOF0.h APP0.h JPEG_Decoder.hpp JPEG_Decoder_Kernels.h )
set( JPEGDecoder.Files ${JPEGDecoder.Source} ${JPEGDecoder.Headers} )

add_executable( JPEGDecoder ${JPEGDecoder.Files} )
target_link_libraries( JPEGDecoder ${OPENCL_LIBRARIES} )

