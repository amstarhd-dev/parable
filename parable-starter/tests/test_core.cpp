#include "parable_core.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace parable;
static bool near(float a,float b,float e=1e-5f) { return std::fabs(a-b)<e; }
int main() {
    constexpr float sr=48000.f;
    Bank bank(sr);
    // All four real slots exist, independently.
    for (size_t i=0;i<4;++i) assert(bank.get(i).count==0);
    Patch p; p.count=2;
    p.stages[0]={Type::Gain, 1.5f, true};
    p.stages[1]={Type::OnePoleLowpass, 1000.f, true};
    p.outputTrim=0.8f;
    assert(bank.set(0,p)==Status::Ok);
    assert(bank.get(1).count==0);
    // Editing the original snapshot cannot mutate the saved patch.
    p.stages[0].parameter=0.f;
    assert(near(bank.get(0).stages[0].parameter,1.5f));
    // Invalid edits reject atomically; no silent half-updated state.
    Patch bad=bank.get(0); bad.stages[0].parameter=1000.f;
    assert(bank.set(0,bad)==Status::InvalidParameter);
    assert(near(bank.get(0).stages[0].parameter,1.5f));
    bad=bank.get(0); bad.schema=999;
    assert(bank.set(0,bad)==Status::SchemaMismatch);
    bad=bank.get(0); bad.stages[0].type=static_cast<Type>(88);
    assert(bank.set(0,bad)==Status::InvalidType);
    assert(bank.select(4)==Status::InvalidSlot);
    assert(bank.select(0)==Status::Ok);
    // Validated stage order affects sound, at least for noncommuting operators.
    Engine e(sr); assert(e.load(bank.get(0))==Status::Ok);
    const float y=e.process(0.5f);
    assert(std::isfinite(y) && y>0.f && y<0.6f);
    Patch reversed=bank.get(0);
    std::swap(reversed.stages[0],reversed.stages[1]);
    Engine f(sr); assert(f.load(reversed)==Status::Ok);
    const float z=f.process(0.5f);
    // Due to linearity gain/LP commute for this initial one-sample test;
    // use bypass vs chain and clear output test, not artificial 'noncommutation'.
    assert(near(y,z));
    Engine dry(sr); assert(dry.load(makeBypassPatch())==Status::Ok);
    assert(near(dry.process(0.5f),0.5f));
    dry.setVolume(.3f); assert(near(dry.process(0.5f),.15f));
    assert(near(dry.process(9.f),1.f)); // bounded even with extreme input
    assert(dry.process(NAN)==0.f);
    // Validate every stage; never treat unknown graph elements as bypass.
    assert(e.load(bad)==Status::InvalidType);
    std::cout<<"PARABLE_CORE_V0_1: host validation passed\n";
    return 0;
}
