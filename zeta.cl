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

__kernel void negamax_gpu(  __global unsigned long *board,
                            __global unsigned int *bindex,
                                    unsigned int som,
                                    unsigned long Lastmove,
                            __global unsigned long *bestmove,
                            __global unsigned long *AttackTables
                        )

{

    bestmove[0] = 2;
}
