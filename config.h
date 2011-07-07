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

#if !defined(CONFIG_H_INCLUDED)
#define CONFIG_H_INCLUDED

// Thread Work item config
int threadsX = 1;
int threadsY = 4;
int threadsZ = 32;

int totalThreads = threadsX * threadsY *threadsZ;

cl_uint maxDims = 3;
size_t globalThreads[3] = {threadsX,threadsY,threadsZ};
size_t localThreads[3]  = {1,threadsY,threadsZ};

#endif

