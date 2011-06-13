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
int threadsX = 10;
// suggestions for other systems:
// int threadsX = 10; // for GPU with 512 MB RAM, 128 MB usable:
// int threadsX = 20; // for GPU with   1 GB RAM, 256 MB usable:
// int threadsX = 40; // for GPU with   2 GB RAM, 512 MB usable
// int threadsX = 1; // default for low end devices


// do not edit these values
int threadsY = 128;
int threadsZ = 1;
size_t globalThreads[3] = {threadsX,threadsY,threadsZ};
size_t localThreads[3]  = {1,threadsY,threadsZ};

#endif

