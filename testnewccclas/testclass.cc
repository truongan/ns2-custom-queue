// #include "testclass.h"


// //static class APingClass   : public TclClass {
// static class testclassClass : public TclClass {
// public:
// 	testclassClass() : TclClass("OTclTestClass") {}
// 	TclObject * create (int, const char* const*){
// 		return new testclass();
// 	}
// } class_test_class;

// static class APingClass : public TclClass {
// public:
//   APingClass() : TclClass("Agent/BCD") {}
//   // TclObject* create(int, const char*const*) {
//   //   return (new APingAgent());
//   // }
// } class_ping;

/*
 * File: Code for a new 'Ping' Agent Class for the ns
 *       network simulator
 * Author: Marc Greis (greis@cs.uni-bonn.de), May 1998
 *
 */


#include "testclass.h"






static class testclassClass : public TclClass {
public:
	testclassClass() : TclClass("OTclTestClass") {}
	TclObject * create (int, const char* const*){
		return (new test_class());
	}
} class_test_class;