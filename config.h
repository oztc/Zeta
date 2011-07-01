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
int threadsX = 1; // for Nvidia 8800/9800/GTS250 with 512 MB RAM

// suggestions for other systems:
// int threadsX = 16; // for Nvidia 8800/9800/GTS250 with 512 MB RAM
// int threadsX = 32; // for Nvidia 8800/9800/GTS250 with 1 GB RAM
// int threadsX = 68; // for AMD 6970 with 2 GB RAM:
// int threadsX = 1; // default for low end devices


// do not edit these values
cl_uint maxDims = 1;
int threadsY = 1;
size_t globalThreads[1] = {threadsX*threadsY};
size_t localThreads[1]  = {threadsY};

#endif

