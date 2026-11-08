#include "parable_core.h"
#include "test_check.h"
#include <cmath>
#include <iostream>
using namespace parable;
static bool near(float a,float b,float e=1e-5f) { return std::fabs(a-b)<e; }
int main() {
    constexpr float sr=48000.f;
    Bank bank(sr);
    // All four real slots exist, independently.
    for (size_t i=0;i<4;++i) CHECK(bank.get(i).count==0);
    Patch p; p.count=2;
    p.stages[0]={Type::Gain, 1.5f, true};
    p.stages[1]={Type::OnePoleLowpass, 1000.f, true};
    p.outputTrim=0.8f;
    CHECK(bank.set(0,p)==Status::Ok);
    CHECK(bank.get(1).count==0);
    // Editing the original snapshot cannot mutate the saved patch.
    p.stages[0].parameter=0.f;
    CHECK(near(bank.get(0).stages[0].parameter,1.5f));
    // Invalid edits reject atomically; no silent half-updated state.
    Patch bad=bank.get(0); bad.stages[0].parameter=1000.f;
    CHECK(bank.set(0,bad)==Status::InvalidParameter);
    CHECK(near(bank.get(0).stages[0].parameter,1.5f));
    bad=bank.get(0); bad.schema=999;
    CHECK(bank.set(0,bad)==Status::SchemaMismatch);
    bad=bank.get(0); bad.stages[0].type=static_cast<Type>(88);
    CHECK(bank.set(0,bad)==Status::InvalidType);
    CHECK(bank.select(4)==Status::InvalidSlot);
    CHECK(bank.select(0)==Status::Ok);
    // Validated stage order affects sound, at least for noncommuting operators.
    Engine e(sr); CHECK(e.load(bank.get(0))==Status::Ok);
    const float y=e.process(0.5f);
    CHECK(std::isfinite(y) && y>0.f && y<0.6f);
    Patch reversed=bank.get(0);
    std::swap(reversed.stages[0],reversed.stages[1]);
    Engine f(sr); CHECK(f.load(reversed)==Status::Ok);
    const float z=f.process(0.5f);
    // Due to linearity gain/LP commute for this initial one-sample test;
    // use bypass vs chain and clear output test, not artificial 'noncommutation'.
    CHECK(near(y,z));
    Engine dry(sr); CHECK(dry.load(makeBypassPatch())==Status::Ok);
    CHECK(near(dry.process(0.5f),0.5f));
    dry.setVolume(.3f); CHECK(near(dry.process(0.5f),.15f));
    CHECK(near(dry.process(9.f),1.f)); // bounded even with extreme input
    CHECK(dry.process(NAN)==0.f);
    // Validate every stage; never treat unknown graph elements as bypass.
    CHECK(e.load(bad)==Status::InvalidType);
    std::cout<<"PARABLE_CORE_V0_1: host validation passed: "<<testChecks<<" checks\n";
    return 0;
}
