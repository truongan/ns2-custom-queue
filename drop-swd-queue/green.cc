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
 */ 
/* Author - Apu Kapadia, University of Illinois at Urbana-Champaign, and LANL, 2/1/2004*/ 
/* Based on original implementation by Sunil Thulasidasan, LANL 10/19/2001 */

#include <math.h>
#include <stdlib.h>
#include "green.h"
#include "delay.h"
#include "random.h"
#include "flags.h"
#include "tcp.h"

static class GreenClass : public TclClass {
 public:
	GreenClass() : TclClass("Queue/Green") {}
	TclObject* create(int, const char*const*) {
		return (new Green);
	}
} class_green;


Green::Green() { 
	q_ = new PacketQueue();
	factor = 1;
	loss_estimate = 0;

	//ctr = 0; //debugging counter 
	qsamp = 0;
	qsum = 0;

	util_estimate = 1;
	Kbytes_sent = 0;
	green_drops = 0;
	queue_drops = 0;

	double j;
	
	active_flows_ = 0;
	next_estimate_ = FIRST_ESTIMATE;
	N_estimate_ = 0;
	for(int i = 0; i < maxFlows; i++) {
		j = Random::uniform();
		flowTable[i].active_ = false;
	}

	
	greenArrivals_ = 0;
	greenAllDrops_ = 0;
	greenEarlyDrops_ = 0;
	greenForcedDrops_ = 0;
	greenDepartures_ = 0;
	
	bind("greenArrivals_", &greenArrivals_);
	bind("greenAllDrops_", &greenAllDrops_);
	bind("greenEarlyDrops_", &greenEarlyDrops_);
	bind("greenForcedDrops_", &greenForcedDrops_);
	bind("greenDepartures_", &greenDepartures_);

	bind("bw_", &bw_);
	bind("mss_", &mss_);
	bind("c_", &c_);
	bind("idmaps_", &idmaps_);
} 

Green::~Green() {
	{
		delete q_;
	}
	
}

int Green::command(int argc, const char*const* argv) {

	Tcl& tcl = Tcl::instance();
	if (argc == 4) {
		if (!(strcmp(argv[1], "set-rtt"))) {
			int fid = atoi(argv[2]);
			double rtt = atof(argv[3]);
			//printf("Setting rtt for flow[%d] = %lf\n", fid, rtt);
			if(idmaps_ == 0)
				{
					//printf("Setting normal rtt to %lf\n",rtt);
					flowTable[fid].rtt_actual_ = rtt;
				}
			else
				{
					//sets RTT between 0.5 to 2 times the actual value
					flowTable[fid].rtt_actual_ = ((0.5+1.5*Random::uniform())) * rtt;
					printf("Setting IDMAPS rtt to %lf instead of %lf\n",flowTable[fid].rtt_actual_,rtt);
					
				}
			return(TCL_OK);
		}
		
	}
	
	return Queue::command(argc, argv);
}

/*
 * The GREEN  engine
 */

int Green::processPkt(Packet* p) {
	
	float rtt; 
	double  pm_, time;
	hdr_ip *iph = hdr_ip::access(p);
	hdr_tcp *tcph = hdr_tcp::access(p);
	
	//might need this for debugging printfs
	time = Scheduler::instance().clock();

	//might need fid for debugging printfs
	int fid = iph->flowid();

	//we use this for IDMaps only - GREEN does not keep per-flow queue state!
	struct flowState *fs;
	fs = &(flowTable[fid]);

	//lookup rtt for the flow
	//if we're using the IDMaps technique, then use stored value
	//otherwise get rtt from TCP header
	double est_rtt;
	if(idmaps_ == 1)
		est_rtt = fs->rtt_actual_; // "flow state" hack for IDMaps simulations
	else
		est_rtt = iph->prio()/1000.0; // "TCP header" hack for now
	
	//N_estimate might be 0 at the beginning. Avoid divide by zero
	if(N_estimate_ == 0)
		pm_ = 0;
	else
		{
			//TCP senders may not have an estimate yet
			//check to see if RTT is >= 1ms
			if(est_rtt >= 0.001)
				{
					rtt = est_rtt;
					pm_ = N_estimate_/(factor*bw_);
					pm_ = (pm_ * (mss_ * (c_/rtt)));
					pm_ *= pm_;
				}
			else
				pm_ = 0.02;
		}
	
	//model doesn't work for pm_ > 0.02, so don't drop more than 2%
	if(pm_ > 0.02)
		pm_ = 0.02;
	
	// Drop with probability pm_
	double u = Random::uniform();
	if (u <= pm_) {
		return(0);
	} else {
		return(1);
	}
}

void Green::enque(Packet* p)
{
		
	//printf("Green enqueue()\n");fflush(stdout);
	greenArrivals_++;

	hdr_ip *iph = hdr_ip::access(p);

	int fid = iph->flowid();
		
	double time = Scheduler::instance().clock();
	if(next_estimate_ == FIRST_ESTIMATE)
		next_estimate_ += LOG_INTERVAL;
	
	if(time > next_estimate_)
		{

			double interval = LOG_INTERVAL + (time - next_estimate_);
			util_estimate = (Kbytes_sent * 8.0) / (bw_/1000.0 * interval);
			
			int qavg = qsum/qsamp;
			qsum = 0;
			qsamp = 0;

			//update the gamma estimator
			if(queue_drops > 0)
				factor *= 0.95;
			else if(util_estimate < 0.98)
				//factor += 0.02;
				factor *= (0.5 + 0.5*util_estimate)/util_estimate;
			printf("time = %lf, util estimate = %lf, factor now = %lf\n", 
			        time, util_estimate, factor);
			printf("green_drops = %lf, queue_drops = %lf\n", green_drops, queue_drops);
			
			Kbytes_sent = 0;
			green_drops = 0;
			queue_drops = 0;



			//estimate no. of flows here
			
			//deactivate all flows
			for(int i = 0; i < maxFlows; i++)
				flowTable[i].active_ = false;
			
			next_estimate_ += LOG_INTERVAL;
			N_estimate_ = active_flows_;
			printf("Time %f s, N = %d\n", time, N_estimate_); fflush(stdout);
			active_flows_ = 0;
		}
	

	if (flowTable[fid].active_ == false)
		{
			flowTable[fid].active_ = true;
			active_flows_++;
		}



	//check whether should drop this packet or not
	int spare = processPkt(p);

		
	if (spare) { //spare the packet 
		
		q_->enque(p);
		
		
		//check queue limit
		if (q_->length() >= qlim_) { //drop the packet
			q_->remove(p);
			drop(p);
			greenForcedDrops_++;
			queue_drops++;
			greenAllDrops_++;

		} else { //keep packet and update flow state
			qsamp++;
			qsum += q_->length();
		}
	}
	
	else { //drop packet
		green_drops++;
		greenEarlyDrops_++;
		greenAllDrops_++;
		drop(p);
	}
}

Packet* Green::deque()
{
	//printf("Green deque() called\n");fflush(stdout);
	greenDepartures_++;
	Packet* p;
	int fid;

	if ((p=(q_->deque()))==0) 
		{	
			//printf("Time %f , Main Queue Empty!\n",Scheduler::instance().clock());
			return NULL;
		}
	
	if (p != 0) { //update flow state for packet being dropped

		hdr_ip *iph = hdr_ip::access(p);
		fid = iph->flowid();
	}
	hdr_cmn* ch = hdr_cmn::access(p);
	Kbytes_sent += ch->size()/1000.0;
	return p;
}






