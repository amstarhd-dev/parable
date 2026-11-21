#include "dfu_control.h"
#include <cassert>
#include <string>
using resource_control::Event;
using resource_control::Protocol;
static Event send(Protocol& p, const std::string& s) {
    Event event=Event::none;
    for(char c:s) {auto next=p.feed(c);if(next!=Event::none)event=next;}
    return event;
}
int main(){
 const std::string artifact(64,'a'), nonce(32,'b');
 const std::string enter="ENTER_DFU "+artifact+" "+nonce+"\n";
 for(unsigned fragment=1;fragment<128;++fragment){
  Protocol p(artifact.c_str());
  assert(send(p,"HELLO "+nonce+"\n")==Event::rejected);
  assert(send(p,enter)==Event::rejected);
  p.reportCompleted();
  assert(send(p,enter)==Event::rejected);
  assert(send(p,"HELLO "+nonce+"\n")==Event::hello);
  assert(send(p,"ENTER_DFU "+std::string(64,'c')+" "+nonce+"\n")==Event::rejected);
  assert(send(p,"ENTER_DFU "+artifact+" "+std::string(32,'c')+"\n")==Event::rejected);
  assert(send(p,"ENTER_DFU\n")==Event::rejected);
  assert(send(p,std::string(500,'x')+"\n")==Event::rejected);
  for(size_t i=0;i<enter.size();i+=fragment){
   auto e=send(p,enter.substr(i,fragment));
   assert(e==(i+fragment>=enter.size()?Event::enter:Event::none));
  }
  assert(send(p,enter)==Event::rejected); // one-shot
 }
 Protocol p(artifact.c_str());p.reportCompleted();
 assert(send(p,"HELLO "+nonce+"\n")==Event::hello);
 p=Protocol(artifact.c_str()); // USB disconnect cancels report/authorization
 assert(send(p,enter)==Event::rejected);
 p.reportCompleted();
 assert(send(p,enter)==Event::rejected);
 assert(send(p,"HELLO "+std::string(32,'g')+"\n")==Event::rejected);
 assert(send(p,std::string("HELLO \0",7)+std::string(31,'a')+"\n")==Event::rejected);
 unsigned random=1;for(unsigned i=0;i<100000;++i){random=random*1664525+1013904223;assert(p.feed(char(random>>24))!=Event::enter);}
}
