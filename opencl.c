/*
    Zeta CL, OpenCL Chess Engine
    Author: Srdja Matovic <srdja@matovic.de>
    Created at: 20-Jan-2011
    Updated at:
    Description: A Chess Engine written in OpenCL, a language suited for GPUs.

    Copyright (C) 2011 Srdja Matovic
    This program is distributed under the GNU General Public License.
    See file COPYING or http://www.gnu.org/licenses/
*/

#include "opencl.h"
#include <stdio.h>
#include <string.h>

/*
 *        brief OpenCL related initialization 
 *        Create Context, Device list, Command Queue
 *        Create OpenCL memory buffer objects
 *        compile, link CL source 
 *		  Build program and kernel objects
 */
int initializeCL(Piece *board) {

    cl_int status = 0;
    size_t deviceListSize;

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */

    cl_uint numPlatforms;
    cl_platform_id platform = NULL;
    status = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(status != CL_SUCCESS)
    {
        print_debug("Error: Getting Platforms. (clGetPlatformsIDs)\n");
        return 1;
    }
    
    if(numPlatforms > 0)
    {
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, NULL);
        if(status != CL_SUCCESS)
        {
            print_debug("Error: Getting Platform Ids. (clGetPlatformsIDs)\n");
            return 1;
        }
        for(unsigned int i=0; i < numPlatforms; ++i)
        {
            char pbuff[100];
            status = clGetPlatformInfo(
                        platforms[i],
                        CL_PLATFORM_VENDOR,
                        sizeof(pbuff),
                        pbuff,
                        NULL);
            if(status != CL_SUCCESS)
            {
                print_debug("Error: Getting Platform Info.(clGetPlatformInfo)\n");
                return 1;
            }
            platform = platforms[i];
            if(!strcmp(pbuff, "Advanced Micro Devices, Inc."))
            {
                break;
            }
        }
        delete platforms;
    }

    if(NULL == platform)
    {
        print_debug("NULL platform found so Exiting Application.");
        return 1;
    }

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };

	/////////////////////////////////////////////////////////////////
	// Create an OpenCL context
	/////////////////////////////////////////////////////////////////

    /* try gpu first */
    context = clCreateContextFromType(cps, 
                                      CL_DEVICE_TYPE_GPU, 
                                      NULL, 
                                      NULL, 
                                      &status);
    /* then cpu */
    if(status != CL_SUCCESS) 
	{  
        context = clCreateContextFromType(cps, 
                                          CL_DEVICE_TYPE_CPU, 
                                          NULL, 
                                          NULL, 
                                          &status);
	}

    if(status != CL_SUCCESS) 
	{  
		print_debug("Error: Creating Context. (clCreateContextFromType)\n");
		return 1; 
	}

    /* First, get the size of device list data */
    status = clGetContextInfo(context, 
                              CL_CONTEXT_DEVICES, 
                              0, 
                              NULL, 
                              &deviceListSize);
    if(status != CL_SUCCESS) 
	{  
		print_debug("Error: Getting Context Info (device list size, clGetContextInfo)\n");
		return 1;
	}

	/////////////////////////////////////////////////////////////////
	// Detect OpenCL devices
	/////////////////////////////////////////////////////////////////
    devices = (cl_device_id *)malloc(deviceListSize);
	if(devices == 0)
	{
		print_debug("Error: No devices found.\n");
		return 1;
	}

    /* Now, get the device list data */
    status = clGetContextInfo(
			     context, 
                 CL_CONTEXT_DEVICES, 
                 deviceListSize, 
                 devices, 
                 NULL);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Getting Context Info (device list, clGetContextInfo)\n");
		return 1;
	}

	/////////////////////////////////////////////////////////////////
	// Create an OpenCL command queue
	/////////////////////////////////////////////////////////////////
    commandQueue = clCreateCommandQueue(
					   context, 
                       devices[0], 
                       0, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Creating Command Queue. (clCreateCommandQueue)\n");
		return 1;
	}

	/////////////////////////////////////////////////////////////////
	// Create OpenCL memory buffers
	/////////////////////////////////////////////////////////////////
    BoardBuffer = clCreateBuffer(
				      context, 
                      CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                      sizeof(cl_char) * 129,
                      board, 
                      &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: BoardBuffer (inputBuffer)\n");
		return 1;
	}

    BestmoveBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_int),
                       &bestmove, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: BestmoveBuffer (outputBuffer)\n");
		return 1;
	}


	/////////////////////////////////////////////////////////////////
	// build CL program object, create CL kernel object
	/////////////////////////////////////////////////////////////////
    const char *content = source;
    const size_t *contentSize = &sourceSize;
    program = clCreateProgramWithSource(
			      context, 
                  1, 
                  &content,
				  contentSize,
                  &status);
	if(status != CL_SUCCESS) 
	{ 
	  print_debug("Error: Loading Binary into cl_program (clCreateProgramWithBinary)\n");
	  return 1;
	}

    /* create a cl program executable for all the devices specified */
    status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Building Program (clBuildProgram)\n");
		return 1; 
	}

    /* get a kernel object handle for a kernel with the given name */
    kernel = clCreateKernel(program, "negamax_gpu", &status);
    if(status != CL_SUCCESS) 
	{  
		print_debug("Error: Creating Kernel from program. (clCreateKernel)\n");
		return 1;
	}

	return 0;
}




/*
 *        brief Run OpenCL program 
 *		  
 *        Bind host variables to kernel arguments 
 *		  Run the CL kernel
 */
int  runCLKernels(int som, Move lastmove) {
    cl_int   status;
	cl_uint maxDims;
    cl_event events[2];
    size_t globalThreads[1];
    size_t localThreads[1];

    
    globalThreads[0] = 1;
    localThreads[0]  = 1;


    /*** Set appropriate arguments to the kernel ***/
    /* the output array to the kernel */

    /* the input to the kernel */
    status = clSetKernelArg(
                    kernel, 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&BoardBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BoardBuffer)\n");
		return 1;
	}

    status = clSetKernelArg(
                    kernel, 
                    1, 
                    sizeof(cl_uint), 
                    (void *)&som);
    if(status != CL_SUCCESS) 
	{ 
		print_debug( "Error: Setting kernel argument. (som)\n");
		return 1;
	}

    status = clSetKernelArg(
                    kernel, 
                    2, 
                    sizeof(cl_uint), 
                    (void *)&lastmove);
    if(status != CL_SUCCESS) 
	{ 
		print_debug( "Error: Setting kernel argument. (lastmove)\n");
		return 1;
	}

    status = clSetKernelArg(
                    kernel, 
                    3, 
                    sizeof(cl_mem), 
                    (void *)&BestmoveBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BestmoveBuffer)\n");
		return 1;
	}

    /* 
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(
			     commandQueue,
                 kernel,
                 1,
                 NULL,
                 globalThreads,
                 localThreads,
                 0,
                 NULL,
                 &events[0]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Enqueueing kernel onto command queue. (clEnqueueNDRangeKernel)\n");
		return 1;
	}


    /* wait for the kernel call to finish execution */
    status = clWaitForEvents(1, &events[0]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Waiting for kernel run to finish. (clWaitForEvents)\n");
		return 1;
	}

    status = clReleaseEvent(events[0]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Release event object. (clReleaseEvent)\n");
		return 1;
	}

    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(
                commandQueue,
                BestmoveBuffer,
                CL_TRUE,
                0,
                1 * sizeof(cl_int),
                &bestmove,
                0,
                NULL,
                &events[1]);
    
    if(status != CL_SUCCESS) 
	{ 
        print_debug("Error: clEnqueueReadBuffer failed. (clEnqueueReadBuffer)\n");

		return 1;
    }
    
    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Waiting for read buffer call to finish. (clWaitForEvents)\n");
		return 1;
	}
    
    status = clReleaseEvent(events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Release event object.(clReleaseEvent)\n");
		return 1;
	}




    /* release resources */

    status = clReleaseKernel(kernel);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseKernel \n");
		return 1; 
	}
    status = clReleaseProgram(program);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseProgram\n");
		return 1; 
	}
    status = clReleaseMemObject(BoardBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BoardBuffer)\n");
		return 1; 
	}
	status = clReleaseMemObject(BestmoveBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BestmoveBuffer)\n");
		return 1; 
	}   
    status = clReleaseCommandQueue(commandQueue);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseCommandQueue\n");
		return 1;
	}
    status = clReleaseContext(context);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseContext\n");
		return 1;
	}

	return 0;
}





int load_file_to_string(const char *filename, char **result) 
{ 
	int size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL) 
	{ 
		*result = NULL;
		return -1;
	} 
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size+1);
	if (size != fread(*result, sizeof(char), size, f)) 
	{ 
		free(*result);
		return -2;
	} 
	fclose(f);
	(*result)[size] = 0;
	return size;
}

void print_debug(char *debug) {
    FILE 	*Stats;
    Stats = fopen("zeta_amd.debug", "ab+");
    fprintf(Stats, "%s", debug);
    fclose(Stats);
}


