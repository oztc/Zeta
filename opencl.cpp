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

#include <oclUtils.h>
#include <shrQATest.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "opencl.h"
#include "config.h"


cl_int status = 0;
cl_uint numPlatforms;
cl_platform_id platform = NULL;
size_t deviceListSize;
const char *content;
const size_t *contentSize;
cl_context_properties cps[3];

int initializeCLDevice() {

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
            if(!strcmp(pbuff, "NVIDIA Corporation"))
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
    cps = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };

	/////////////////////////////////////////////////////////////////
	// Create an OpenCL context
	/////////////////////////////////////////////////////////////////

    /* try gpu first */
    context = clCreateContextFromType(cps, 
                                      CL_DEVICE_TYPE_GPU, 
                                      NULL, 
                                      NULL, 
                                      &status);
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

    return 0;
}

int initializeCL() {

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
                      sizeof(cl_ulong) * 4 * max_depth * threadsX * threadsY,
                      BOARDS, 
                      &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (BoardBuffer)\n");
		return 1;
	}

    MoveBuffer = clCreateBuffer(
				      context, 
                      CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                      sizeof(cl_ulong) * 1 * max_depth * threadsX * threadsY * threadsY ,
                      MOVES, 
                      &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (MoveBuffer)\n");
		return 1;
	}

    ScoreBuffer = clCreateBuffer(
				      context, 
                      CL_MEM_READ_WRITE ,
                      sizeof(cl_int) * 1 * max_depth * threadsY,
                      NULL, 
                      &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (ScoreBuffer)\n");
		return 1;
	}

    BestmoveBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong),
                       &bestmove, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (BestmoveBuffer)\n");
		return 1;
	}

    CountersBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 2 * threadsY,
                       COUNTERS, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (CountersBuffer)\n");
		return 1;
	}

    SetMaskBBBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 65,
                       &SetMaskBB, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (SetMaskBBBuffer)\n");
		return 1;
	}

    ClearMaskBBBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 65,
                       &ClearMaskBB, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (ClearMaskBBBuffer)\n");
		return 1;
	}

    AttackTablesBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 2 * 7 * 64,
                       &AttackTables, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (AttackTablesBuffer)\n");
		return 1;
	}

    AttackTablesToBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 2 * 7 * 64,
                       &AttackTablesTo, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (AttackTablesToBuffer)\n");
		return 1;
	}

    PawnAttackTablesBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 4 * 64,
                       &PawnAttackTables, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (PawnAttackTables)\n");
		return 1;
	}

    RAttackIndexBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_int) * 64,
                       &RAttackIndex, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (RAttackIndexBuffer)\n");
		return 1;
	}

    RAttacksBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 0x19000,
                       &RAttacks, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (RAttacksBuffer)\n");
		return 1;
	}

    RMaskBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 64,
                       &RMask, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (RMaskBuffer)\n");
		return 1;
	}

    BAttackIndexBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_int) * 64,
                       &BAttackIndex, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (BAttackIndexBuffer)\n");
		return 1;
	}

    BAttacksBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 0x1480,
                       &BAttacks, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (BAttacksBuffer)\n");
		return 1;
	}

    BMaskBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(cl_ulong) * 64,
                       &BMask, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (BMaskBuffer)\n");
		return 1;
	}

    doneBuffer = clCreateBuffer(
					   context, 
                       CL_MEM_READ_WRITE,
                       sizeof(cl_int) * threadsY * max_depth,
                       NULL, 
                       &status);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: clCreateBuffer (doneBuffer)\n");
		return 1;
	}

	/////////////////////////////////////////////////////////////////
	// build CL program object, create CL kernel object
	/////////////////////////////////////////////////////////////////
    content = source;
    contentSize = &sourceSize;
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
int  runCLKernels(unsigned int som, Move lastmove, unsigned int maxdepth) {

    cl_event events[2];
    int i = 0;

    /*** Set appropriate arguments to the kernel ***/
    /* the output array to the kernel */

    /* the input to the kernel */
    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&BoardBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BoardBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&MoveBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (MoveBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&ScoreBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (ScoreBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_uint), 
                    (void *)&som);
    if(status != CL_SUCCESS) 
	{ 
		print_debug( "Error: Setting kernel argument. (som)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_uint), 
                    (void *)&maxdepth);
    if(status != CL_SUCCESS) 
	{ 
		print_debug( "Error: Setting kernel argument. (maxdepth)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_ulong), 
                    (void *)&lastmove);
    if(status != CL_SUCCESS) 
	{ 
		print_debug( "Error: Setting kernel argument. (lastmove)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&BestmoveBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BestmoveBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&CountersBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (CountersBuffer)\n");
		return 1;
	}
    i++;


    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&SetMaskBBBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (SetMaskBBBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&ClearMaskBBBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (ClearMaskBBBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&AttackTablesBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (AttackTablesBuffer)\n");
		return 1;
	}
    i++;    

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&AttackTablesToBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (AttackTablesToBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&PawnAttackTablesBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (PawnAttackTablesBuffer)\n");
		return 1;
	}
    i++;


    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_uint), 
                    (void *)&threadsX);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (threadsX)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_uint), 
                    (void *)&threadsY);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (threadsY)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&RAttackIndexBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (RAttackIndexBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&RAttacksBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (RAttacksBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&RMaskBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (RMaskBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&BAttackIndexBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BAttackIndexBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&BAttacksBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BAttacksBuffer)\n");
		return 1;
	}
    i++;

    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&BMaskBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (BMaskBuffer)\n");
		return 1;
	}
    i++;


    status = clSetKernelArg(
                    kernel, 
                    i, 
                    sizeof(cl_mem), 
                    (void *)&doneBuffer);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Setting kernel argument. (doneBuffer)\n");
		return 1;
	}
    i++;

    /* 
     * Enqueue a kernel run call.
     */
    status = clEnqueueNDRangeKernel(
			     commandQueue,
                 kernel,
                 maxDims,
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

//    status = clFinish(commandQueue);

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
                1 * sizeof(cl_ulong),
                &bestmove,
                0,
                NULL,
                &events[1]);
    
    if(status != CL_SUCCESS) 
	{ 
        print_debug("Error: clEnqueueReadBuffer failed. (BestmoveBuffer)\n");

		return 1;
    }

    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Waiting for read buffer call to finish. (BestmoveBuffer)\n");
		return 1;
	}
    status = clReleaseEvent(events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Release event object.(BestmoveBuffer)\n");
		return 1;
	}


    /* Enqueue readBuffer*/
    status = clEnqueueReadBuffer(
                commandQueue,
                CountersBuffer,
                CL_TRUE,
                0,
                2 * threadsY * sizeof(cl_ulong),
                COUNTERS,
                0,
                NULL,
                &events[1]);
    
    if(status != CL_SUCCESS) 
	{ 
        print_debug("Error: clEnqueueReadBuffer failed. (CountersBuffer)\n");

		return 1;
    }
    /* Wait for the read buffer to finish execution */
    status = clWaitForEvents(1, &events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Waiting for read buffer call to finish. (CountersBuffer)\n");
		return 1;
	}
    status = clReleaseEvent(events[1]);
    if(status != CL_SUCCESS) 
	{ 
		print_debug("Error: Release event object.(CountersBuffer)\n");
		return 1;
	}




    /* release resources */

    status = clReleaseKernel(kernel);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseKernel \n");
		return 1; 
	}
    status = clReleaseCommandQueue(commandQueue);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseCommandQueue\n");
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

    status = clReleaseMemObject(MoveBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (MoveBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(BestmoveBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BestmoveBuffer)\n");
		return 1; 
	}  

	status = clReleaseMemObject(CountersBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (CountersBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(AttackTablesBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (AttackTablesBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(AttackTablesToBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (AttackTablesToBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(PawnAttackTablesBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (PawnAttackTablesBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(RAttackIndexBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (RAttackIndexBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(RAttacksBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (RAttacksBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(RMaskBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (RMaskBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(BAttackIndexBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BAttackIndexBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(BAttacksBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BAttacksBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(BMaskBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (BMaskBuffer)\n");
		return 1; 
	}

	status = clReleaseMemObject(doneBuffer);
    if(status != CL_SUCCESS)
	{
		print_debug("Error: In clReleaseMemObject (doneBuffer)\n");
		return 1; 
	}

	return 0;
}


int  releaseCLDevice() {

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
    Stats = fopen("zeta_nv.debug", "ab+");
    fprintf(Stats, "%s, status:%i", debug, status);
    if (status == CL_DEVICE_NOT_AVAILABLE)
        fprintf(Stats, "CL_DEVICE_NOT_AVAILABLE");
    fclose(Stats);
}


