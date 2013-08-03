/*
 * File: Header File for a new 'Ping' Agent Class for the ns
 *       network simulator
 * Author: Marc Greis (greis@cs.uni-bonn.de), May 1998
 *
 */


#ifndef a_ping_h
#define a_ping_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"


struct hdr_ping {
  char ret;
  double send_time;

  //Header access
  
  static int offset_; // required by PacketHeaderManager
	inline static int& offset() { return offset_; }
	inline static hdr_ping* access(const Packet* p) {
		return (hdr_ping*) p->access(offset_);
	}
};


class APingAgent : public Agent {
 public:
  APingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);
 protected:
  int off_ping_;
};


#endif
