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

// Edit threadsX for Custom GPU Config, see README
int threadsX = 4; // for Nvidia 8800/9800/GTS250 with 512 MB RAM

// suggestions for other systems:
// int threadsX = 16; // for Nvidia 8800/9800/GTS250 with 512 MB RAM
// int threadsX = 24; // for AMD 6970 with 2 GB RAM:
// int threadsX = 1; // default for low end devices


// do not edit these values
cl_uint maxDims = 3;

int threadsY = 4;

size_t globalThreads[3] = {1,threadsX,threadsY};
size_t localThreads[3]  = {1,1,threadsY};

#endif

