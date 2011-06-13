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

// do not edit these values
int threadsY = 128;
int threadsZ = 8;
size_t globalThreads[3] = {threadsX,threadsY,threadsZ};
size_t localThreads[3]  = {1,1,threadsZ};

#endif

