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

#if !defined(OPENCL_H_INCLUDED)
#define OPENCL_H_INCLUDED

#include "types.h"

int load_file_to_string(const char *filename, char **result);
void print_debug(char *debug);

extern char *source;
extern size_t sourceSize;
extern Move bestmove;
extern Move *MOVES;
extern long NODECOUNT;
extern long MOVECOUNT;


cl_mem   BoardBuffer;
cl_mem	 BestmoveBuffer;
cl_mem	 MoveBuffer;
cl_mem	 MovecountBuffer;
cl_mem	 NodecountBuffer;


cl_context          context;
cl_device_id        *devices;
cl_command_queue    commandQueue;

cl_program program;

cl_kernel  kernel;


#endif




