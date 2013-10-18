#pragma once
#define __CL_ENABLE_EXCEPTIONS

#include "gaussian.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;



int main(int argc, char* argv[])
{
    ImageFilter*	img_filter;
	img_filter = new ImageFilter(string(argv[1]));
	unsigned int num_of_frames = 0;
try
{
	/*****************/
	/*     GPU		 */
	/*****************/
	img_filter->init_GPU_OpenCL();
	img_filter->start_GPU_Timer();
	img_filter->run_GPU();
	img_filter->stop_GPU_Timer();
	img_filter->print_GPU_Timer();

	/*****************/
	/*     CPU		 */
	/*****************/
#ifdef ENABLE_CPU
	pfe->init_CPU(num_of_frames);
	pfe->start_CPU_Timer();
	pfe->run_CPU(num_of_frames);
	pfe->stop_CPU_Timer();
	pfe->print_CPU_Timer();
#endif
	/*****************************/
	/* GPU-CPU results evalution */
	/*****************************/
	img_filter-> write_bmp_image( );
	delete(img_filter);
}
#ifdef __CL_ENABLE_EXCEPTIONS
	catch(cl::Error err)
	{
		cout << "Error: " << err.what() << "(" << err.err() << ")" << endl;
		cout << "Please check CL/cl.h for error code" << endl;
		delete(img_filter);
	}
#endif
	catch(string msg)
	{
		cout << "Exception caught: " << msg << endl;
		delete(img_filter);
	}

	return 0;
}

