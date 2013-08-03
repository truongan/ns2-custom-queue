
// #ifdef testclass_h
// #define testclass_h

// #include "object.h"
// #include "agent.h"
// #include "tclcl.h"
// #include "packet.h"
// #include "address.h"
// #include "ip.h"

// class test_class : public TclObject{
// public:
// 	test_class() {};
// 	virtual ~test_class() {};

// };


// #endif



#ifndef a_test_h
#define a_test_h

#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"


/*struct hdr_ping {
  char ret;
  double send_time;
};


class APingAgent : public Agent {
 public:
  APingAgent();
  int command(int argc, const char*const* argv);
  void recv(Packet*, Handler*);
 protected:
  int off_ping_;
};
*/
class test_class : public TclObject{
public:
	test_class() {};
	virtual ~test_class() {};

};
#endif