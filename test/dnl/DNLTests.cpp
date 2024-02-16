// Unit tests for DNL in the style of ../snl/snl/ tests
//DNL can only be created on top of existing SNL(given the top)
//No instances can be deleted or created after DNL creation
//For creating SNL for the tests please look at the tests in ../snl/snl/kernel/

#include "../../src/dnl/DNL.h"
#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"
#include "../../src/snl/snl/kernel/SNLUniverse.h"
#include "../../src/snl/snl/kernel/SNLPath.h"
#include "../../src/snl/snl/kernel/SNLScalarTerm.h"
#include "../../src/snl/snl/kernel/SNLInstTerm.h"
#include "../../src/snl/snl/kernel/SNLScalarNet.h"
#include "../../src/snl/snl/kernel/SNLBitNetOccurrence.h"
#include "../../src/snl/snl/kernel/SNLBitTermOccurrence.h"
#include "../../src/snl/snl/kernel/SNLEquipotential.h"
#include "../../src/snl/snl/kernel/SNLException.h"

using namespace naja::DNL;
using namespace naja::SNL;

class DNLTests : public ::testing::Test {
 protected:
  DNLTests() {
    // You can do set-up work for each test here
    }
    ~DNLTests() override {
        // You can do clean-up work that doesn't throw exceptions here
    }
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }
    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

};

//Validation of SNL data access from DNL(simple netlist)
//Create a simple SNL netlist and then a DNL on top of it
//and validate the access to the data
TEST_F(DNLTests, SNLDataAccess) {
    //Create one module snl with one input and one output
    SNLUniverse* univ = SNLUniverse::create();
    SNLDB* db = SNLDB::create(univ);
    SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));
    //Create a DNL on top of the SNL
    DNL* dnl = DNL::create();
    assert(dnl != nullptr);
    assert(dnl->getTop().getSNLModel() != nullptr);
    //Validate the access to the SNL data
    DNLID modID = dnl->getTop().getID();
    DNLID inTermID = dnl->getTop().getTerminal(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminal(outTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    //Destroy the DNL
    DNL::destroy();
    //Destroy the SNL
    SNLUniverse::get()->destroy();
}

    
    







