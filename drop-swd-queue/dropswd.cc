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

bool DropSwd::is_tcp(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return hip->dport() == 20 || hip->dport() == 21;
}

bool DropSwd::is_other_udp(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return hip->dport() == 53;
}

bool DropSwd::is_voip(Packet* p){
	hdr_ip* hip = hdr_ip::access(p);

	return  	(hip->dport() >=200 && hip->dport() <= 204)
			||	(!is_other_udp(p) && !is_tcp(p))			;
}


void DropSwd::accept_packet(Packet* p){
	int size = hdr_cmn::access(p)->size();

	if (is_tcp(p)) Occ_tcp+= size;
	else if (is_other_udp(p)) Occ_other_udp+= size;
	else if (is_voip(p)) Occ_voip+= size;

	q_->enque(p);

	time_packet* tp = new time_packet();
	tp->received_time = Scheduler::instance().clock();
	tp->according_packet = p;
	time_queue->enque(tp);
}

void DropSwd::remove_packet(Packet *p){
	int size = hdr_cmn::access(p)->size();

	if (is_tcp(p)) Occ_tcp -= size;
	else if (is_other_udp(p)) Occ_other_udp -= size;
	else if (is_voip(p)) Occ_voip -= size;

	q_->remove(p);
	drop(p);

	time_packet * q = (time_packet*)time_queue->head();
	for (
			; q!= 0 && q->according_packet != p
			; q = (time_packet*)q->next_
		);
	if (q != 0){
		time_queue->remove(q);
		drop(q);
	}

}

bool DropSwd::is_invalid(Packet* p){
	hdr_cmn * hcommon = hdr_cmn::access(p);
	return (Scheduler::instance().clock()-hcommon->timestamp()) * 1000 > 300;
}

/*
 * drop-tail
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
			if (is_voip(p)){
				/* select the invalid VOIP packet closest to the front
				 * or the VoIP packet closest to the front if none of them
				 * is invalid
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
			//Select the closeset to the front
			//tcp or other_udp packet accordingly and drop it
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


