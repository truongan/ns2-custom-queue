/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */


/* Author - Phạm Nguyễn Trường An, 2013/08/01 */


#ifndef dropswd_h
#define dropswd_h

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

class DropSwd : public Queue {
  public:
	DropSwd();
	~DropSwd();
  protected:
	//int ctr; // debugging counter
	int qsamp;
	int qsum;

	int command(int argc, const char*const* argv);
	void enque(Packet*);
	Packet* deque();
	PacketQueue *q_;	/* underlying FIFO queue */

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






