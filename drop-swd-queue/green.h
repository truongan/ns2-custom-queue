/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* Author - Apu Kapadia, University of Illinois at Urbana-Champaign, and LANL, 2/1/2004*/ 
/* Based on original implementation by Sunil Thulasidasan, LANL 10/19/2001 */

#ifndef ns_green_h
#define ns_green_h

#include <string.h>
#include "queue.h"
#include "config.h"

#define maxFlows 2000

#define FIRST_ESTIMATE 1
#define LOG_INTERVAL 1


// We use this datastructure to keep track of a flow's RTT when using
// IDMaps. This does not mean that GREEN keeps per-flow state information
// This is merely a convenience for simulations.
// We also use this as an easy way to estimate the number of active flows. 
// An alternative method would count the number of unique flows seen in the
// log interval to estimate N.
struct flowState
{
	double rtt_actual_;
	int flowid_ ;
	bool active_ ;
};

class Green : public Queue {
  public:
	Green(); 
	~Green();
  protected:
	//int ctr; // debugging counter
	int qsamp;
	int qsum;

	int command(int argc, const char*const* argv); 
	void enque(Packet*);
	Packet* deque();
	PacketQueue *q_,*cq_;	/* underlying FIFO queue */
	
	int greenArrivals_;
	int greenAllDrops_;
	int greenEarlyDrops_;
	int greenForcedDrops_;
	int greenDepartures_;

	
	double factor; //gamma parameter
	double util_estimate; // fraction of bw used in given interval
	double loss_estimate; // fraction of packets lost in given interval
	
	double Kbytes_sent;
	double queue_drops; // packets dropped by queue overflowing, in interval
	double green_drops;  // packets dropped by green, in interval

	double next_estimate_;
	int N_estimate_;
	int active_flows_;
	int idmaps_;
	double bw_; /*bandwidth of outgoing link to which this queue is attached in bps*/
	double delay_; /*The propogation delay of outgoing link*/
	int mss_; /*max segment size of packet*/
	double c_; /*constant used in probablity equation*/

	struct flowState flowTable[maxFlows];
	int processPkt(Packet* p); 
};

#endif






