/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */


/* Author - Phạm Nguyễn Trường An, 2013/08/01 */

/*
 * This file implement the Drop SWD algorithm.
 */

#ifndef dropswd_h
#define dropswd_h

#include "drop-tail.h"

/*
 * A bounded, drop-tail queue
 */
class DropSwd : public DropTail {
  public:
	DropSwd(): DropTail(){
		Occ_other_udp = Occ_tcp = Occ_voip = 0;

		bind_bool("drop_front_", &drop_front_);
		bind_bool("summarystats_", &summarystats);
		bind_bool("queue_in_bytes_", &qib_);  // boolean: q in bytes?
		bind("mean_pktsize_", &mean_pktsize_);
	}

  protected:

	void enque(Packet*);
	Packet* deque();

  private:
	int Occ_tcp, Occ_voip, Occ_other_udp; /*the 3 counter varibale*/

	bool is_tcp(Packet* p);
	bool is_other_udp(Packet* p);
	bool is_voip(Packet* p);

	bool is_invalid(Packet *p); /* invalide packet is voip papket that have timestamp more than 300ms ago*/
	void accept_packet(Packet* p); /*enque packet and update counter*/
	void remove_packet(Packet *p); /*update counter as if packet is dropped but don't actually drop packet*/
	void remove_packet(Packet *p, bool _drop); /*drop packet and update counter */
};

#endif
