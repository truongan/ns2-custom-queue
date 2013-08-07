/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */


/* Author - Phạm Nguyễn Trường An, 2013/08/01 */


#ifndef dropswd_h
#define dropswd_h

#include "drop-tail.h"

class time_packet : public Packet{
  public:
	double received_time;
	Packet *according_packet;
};


/*
 * A bounded, drop-tail queue
 */
class DropSwd : public DropTail {
  public:
	DropSwd(): DropTail(){
		Occ_other_udp = Occ_tcp = Occ_voip = 0;
		time_queue = new PacketQueue;
		bind_bool("drop_front_", &drop_front_);
		bind_bool("summarystats_", &summarystats);
		bind_bool("queue_in_bytes_", &qib_);  // boolean: q in bytes?
		bind("mean_pktsize_", &mean_pktsize_);
	}

  protected:

	void enque(Packet*);

	PacketQueue *time_queue;	/* FIFO queue of time packet */
  private:
	int Occ_tcp, Occ_voip, Occ_other_udp;
	bool is_tcp(Packet* p);
	bool is_other_udp(Packet* p);
	bool is_voip(Packet* p);

	bool is_invalid(Packet *p);
	void accept_packet(Packet* p); /*enque packet and update counter*/
	void remove_packet(Packet* p); /*drop packet and update counter (just drop, don't remove from queue */
};

#endif
