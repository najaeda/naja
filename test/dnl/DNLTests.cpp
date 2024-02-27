// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
    DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    //Destroy the DNL
    DNL::destroy();
    //Destroy the SNL
    SNLUniverse::get()->destroy();
}

TEST_F(DNLTests, SNLDataAccessWith2levelsOfHierarchy) {
    //Create one module snl with one input and one output
    SNLUniverse* univ = SNLUniverse::create();
    SNLDB* db = SNLDB::create(univ);
    SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, SNLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, SNLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, SNLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, SNLName("subinst"));
    //Create a DNL on top of the SNL
    DNL* dnl = DNL::create();
    assert(dnl != nullptr);
    assert(dnl->getTop().getSNLModel() != nullptr);
    //Validate the access to the SNL data
    DNLID modID = dnl->getTop().getID();
    DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();;
    DNLID submodID = dnl->getTop().getChildInstance(subinst).getID();
    DNLID subinTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subinTerm).getID();
    DNLID suboutTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(suboutTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    EXPECT_EQ(submodID, 1);
    EXPECT_EQ(subinTermID, 2);
    EXPECT_EQ(suboutTermID, 3);
    //Destroy the DNL
    DNL::destroy();
    //Destroy the SNL
    SNLUniverse::get()->destroy();
}

TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchy) {
    //Create one module snl with one input and one output
    SNLUniverse* univ = SNLUniverse::create();
    SNLDB* db = SNLDB::create(univ);
    SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));
    //Create a DNL on top of the SNL
    
   
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, SNLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, SNLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, SNLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, SNLName("subinst"));
    //Create a DNL on top of the SNL
    
    
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, SNLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, SNLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, SNLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, SNLName("subsubinst"));
    //Create a DNL on top of the SNL
    //Validate the access to the SNL data
    DNL* dnl = DNL::create();
     assert(dnl->getTop().getSNLModel() != nullptr);
    DNLID modID = dnl->getTop().getID();
    DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();;
    DNLID submodID = dnl->getTop().getChildInstance(subinst).getID();
    DNLID subinTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subinTerm).getID();
    DNLID suboutTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(suboutTerm).getID();;
    DNLID subsubmodID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getID();
    DNLID subsubinTermID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsubinTerm).getID();
    DNLID subsuboutTermID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsuboutTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    EXPECT_EQ(submodID, 1);
    EXPECT_EQ(subinTermID, 2);
    EXPECT_EQ(suboutTermID, 3);
    EXPECT_EQ(subsubmodID, 2);
    EXPECT_EQ(subsubinTermID, 4);
    EXPECT_EQ(subsuboutTermID, 5);
    //Destroy the DNL
    DNL::destroy();
    //Destroy the SNL
    SNLUniverse::get()->destroy();
}

//Based on last test, create a DNL with 3 levels of hierarchy and validate iso db
// 1. Create a simple SNL netlist and then a DNL on top of it
// 2. Create a sub module snl with one input and one output
// 3. Create a sub module snl with one input and one output
// 4. connect the output of the second sub module to the output of the first sub module and
//    the output of the first sub module to the input of the top module.
// 5. Connect the input of the second sub module to the input of the first submodule module and
//    the input of the first sub module to the input of the top module.
// 6. Validate the iso db
TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDB) {
    //Create one module snl with one input and one output
    SNLUniverse* univ = SNLUniverse::create();
    SNLDB* db = SNLDB::create(univ);
    SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, SNLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, SNLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, SNLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, SNLName("subinst"));
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, SNLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, SNLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, SNLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, SNLName("subsubinst"));
    //Connect the output of the second sub module to the output of the first sub module
    auto subOutNet = SNLScalarNet::create(submod);
    suboutTerm->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
    //Connect the output of the first sub module to the input of the top module
    auto outNet = SNLScalarNet::create(mod);
    outTerm->setNet(outNet);
    subinst->getInstTerm(suboutTerm)->setNet(outNet);
    //Connect the input of the second sub module to the input of the first submodule module
    auto subsInNet = SNLScalarNet::create(submod);
    subinTerm->setNet(subsInNet);
    subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
    //Connect the input of the first sub module to the input of the top module
    auto inNet = SNLScalarNet::create(mod);
    inTerm->setNet(inNet);
    subinst->getInstTerm(subinTerm)->setNet(inNet);
    //Create a DNL on top of the SNL
    DNL* dnl = DNL::create();
    assert(dnl != nullptr);
    assert(dnl->getTop().getSNLModel() != nullptr);
    //Validate the access to the SNL data
    DNLID modID = dnl->getTop().getID();
    DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();;
    DNLID submodID = dnl->getTop().getChildInstance(subinst).getID();
    DNLID subinTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subinTerm).getID();
    DNLID suboutTermID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(suboutTerm).getID();;
    DNLID subsubmodID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getID();
    DNLID subsubinTermID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsubinTerm).getID();
    DNLID subsuboutTermID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsuboutTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    EXPECT_EQ(submodID, 1);
    EXPECT_EQ(subinTermID, 2);
    EXPECT_EQ(suboutTermID, 3);
    EXPECT_EQ(subsubmodID, 2);
    EXPECT_EQ(subsubinTermID, 4);
    EXPECT_EQ(subsuboutTermID, 5);
    //Validate the iso db
    EXPECT_EQ(dnl->getDNLIsoDB().getNumIsos(), 2);
    DNLID inIsoID = dnl->getTop().getTerminalFromBitTerm(inTerm).getIsoID();
    DNLID subinIsoID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subinTerm).getIsoID();
    DNLID subsubInIsoID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsubinTerm).getIsoID();
    DNLID outIsoID = dnl->getTop().getTerminalFromBitTerm(outTerm).getIsoID();
    DNLID suboutIsoID = dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(suboutTerm).getIsoID();
    DNLID subsubOutIsoID = dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getTerminalFromBitTerm(subsuboutTerm).getIsoID();
    EXPECT_EQ(inIsoID, subinIsoID);
    EXPECT_EQ(subinIsoID, subsubInIsoID);
    EXPECT_EQ(outIsoID, suboutIsoID);
    EXPECT_EQ(suboutIsoID, subsubOutIsoID);
    const DNLIso& isoIn = dnl->getDNLIsoDB().getIsoFromIsoIDconst(inIsoID);
    const DNLIso& isoOut = dnl->getDNLIsoDB().getIsoFromIsoIDconst(outIsoID);
    EXPECT_EQ(isoIn.getDrivers().size(), 1);
    EXPECT_EQ(isoOut.getDrivers().size(), 1);
    EXPECT_EQ(isoIn.getReaders().size(), 1);
    EXPECT_EQ(isoOut.getReaders().size(), 1);
    //Destroy the DNL
    DNL::destroy();
    //Destroy the SNL
    SNLUniverse::get()->destroy();
}

    
