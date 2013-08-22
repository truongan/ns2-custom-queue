/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */


/* Author - Phạm Nguyễn Trường An, 2013/08/01 */

#include "dropswd.h"


static class DropSwdClass : public TclClass {
 public:
	DropSwdClass() : TclClass("Queue/DropSwd") {}
	TclObject* create(int, const char*const*) {
		return (new DropSwd);
	}
} class_drop_tail;

/*
 * Pacet on port 20 and 21 (FTP packet) is consider TCP
 */
bool DropSwd::is_tcp(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return hip->dport() == 20 || hip->dport() == 21;
}


/*
 * Any thing that is neither TCP or VOIP is consider UDP
 */
bool DropSwd::is_other_udp(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return hip->dport() == 53;
}

bool DropSwd::is_voip(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return  	(hip->dport() >=200 && hip->dport() <= 204)
			||	(!is_other_udp(p) && !is_tcp(p))			;
}

/*
 * eqneue the pacpet p and also update Occ counter
 */
void DropSwd::accept_packet(Packet* p){
	int size = hdr_cmn::access(p)->size();

	if (is_tcp(p)) Occ_tcp+= size;
	else if (is_other_udp(p)) Occ_other_udp+= size;
	else if (is_voip(p)) Occ_voip+= size;

	q_->enque(p);

}

/*
 * Update counter and drop the packet for good
 */
void DropSwd::remove_packet(Packet *p ){
	remove_packet(p, true);
}

/*
 * update the Occ counter when as if packet go off the queue.
 * if the _drop argument is true then we also drop the packet for good
 */
void DropSwd::remove_packet(Packet *p, bool _drop ){
	int size = hdr_cmn::access(p)->size();

	if (is_tcp(p)) Occ_tcp -= size;
	else if (is_other_udp(p)) Occ_other_udp -= size;
	else if (is_voip(p)) Occ_voip -= size;


	if (_drop) {
		q_->remove(p);
		drop(p);
	}

}


/*
 * Invalid packet is packet that have the timestamp field in cmmon header
 * at more than 300ms ago
 */
bool DropSwd::is_invalid(Packet* p){
	hdr_cmn * hcommon = hdr_cmn::access(p);
	return (Scheduler::instance().clock()-hcommon->timestamp()) * 1000 > 300;
}

/*
 * Dequeue and update counter
 */
Packet* DropSwd::deque(){
    if (summarystats && &Scheduler::instance() != 0) {
            Queue::updateStats(qib_?q_->byteLength():q_->length());
    }
    Packet* p = q_->deque();

    //the followling line will upadte Occ counter as if the packet go off queue
    if (p != 0) remove_packet(p, false);

    return p;
}

/*
 * drop swd algorithm goes all in this function
 */
void DropSwd::enque(Packet* p)
{

	if (summarystats) {
		Queue::updateStats(qib_?q_->byteLength():q_->length());
	}

	int qlimBytes = qlim_ * mean_pktsize_;
	if ((!qib_ && (q_->length() + 1) >= qlim_) ||
			(qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes)
		){
		accept_packet(p);
		if (Occ_voip > Occ_tcp && Occ_voip > Occ_other_udp){
			//The mechanism switches to Drop-Tail
			//Or should it be call drop-front??
			if (is_voip(p)){
				/* select the invalid VOIP packet closest to the front or
				 * the VoIP packet closest to the front if none of them is invalid
				 */
				Packet* first_voip = 0;
				Packet* pp = q_->head();
				for (; pp != 0; pp = pp->next_ ){
					if (is_voip(pp)){
						if (first_voip == 0) first_voip = pp;
						if (is_invalid(pp)) break;
					}
				}

				if (pp == 0) pp = first_voip;

				remove_packet(pp);
			}
			else
			{
				remove_packet(p);
			}
		}
		else
		{
			// Select the tcp or other_udp packet
			// closest to the front accordingly and drop it
			for (Packet*pp = q_->head(); pp != 0; pp = pp->next_ ){
				if (
						(Occ_tcp > Occ_other_udp && is_tcp(pp))
					||  (Occ_other_udp <= Occ_tcp && is_other_udp(pp))
				){
					remove_packet(pp);
					break;
				}
			}
		}
	}
	else
	{
		q_->enque(p);
	}
}


