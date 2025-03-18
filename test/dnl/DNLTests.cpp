// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLPath.h"
#include "SNLUniquifier.h"

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
        //Destroy the SNL
        NLUniverse::get()->destroy();
    }

};

//Validation of SNL data access from DNL(simple netlist)
//Create a simple SNL netlist and then a DNL on top of it
//and validate the access to the data
TEST_F(DNLTests, SNLDataAccess) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
    assert(dnl != nullptr);
    assert(dnl->getTop().getSNLModel() != nullptr);
    //Validate the access to the SNL data
    DNLID modID = dnl->getTop().getID();
    DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
    DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();;
    EXPECT_EQ(modID, 0);
    EXPECT_EQ(inTermID, 0);
    EXPECT_EQ(outTermID, 1);
    dnl->getNonConstDNLInstanceFromID(DNLID_MAX);
    dnl->getDNLInstanceFromID(DNLID_MAX);
    dnl->getNonConstDNLTerminalFromID(DNLID_MAX);
    dnl->getDNLTerminalFromID(DNLID_MAX);
    //Destroy the DNL
    destroy();
}

//Empty top
TEST_F(DNLTests, EmptyTop) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
    //Destroy the DNL
    destroy();
}

//Empty child
TEST_F(DNLTests, EmptyChild) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    univ->setTopDesign(mod);
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
    //Destroy the DNL
    destroy();
}

TEST_F(DNLTests, SNLDataAccessWith2levelsOfHierarchy) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
    ASSERT_NE(dnl, nullptr);
    ASSERT_NE(dnl->getTop().getSNLModel(), nullptr);
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
    destroy();
}

TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchy) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a DNL on top of the SNL
    
   
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    //Create a DNL on top of the SNL
    
    
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
    //Create a DNL on top of the SNL
    //Validate the access to the SNL data
    DNLFull* dnl = get();
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
    destroy();
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
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
    //Connect the output of the second sub module to the output of the first sub module
    auto subOutNet = SNLScalarNet::create(submod, NLName("submodnet"));
    subOutNet->setType(SNLNet::Type::Assign0);
    suboutTerm->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
    //Connect the output of the first sub module to the input of the top module
    auto outNet = SNLScalarNet::create(mod, NLName("modnet"));
    outNet->setType(SNLNet::Type::Assign1);
    outTerm->setNet(outNet);
    subinst->getInstTerm(suboutTerm)->setNet(outNet);
    //Connect the input of the second sub module to the input of the first submodule module
    auto subsInNet = SNLScalarNet::create(submod);
    subsInNet->setType(SNLNet::Type::Assign0);
    subinTerm->setNet(subsInNet);
    subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
    //Connect the input of the first sub module to the input of the top module
    auto inNet = SNLScalarNet::create(mod);
    inNet->setType(SNLNet::Type::Assign1);
    inTerm->setNet(inNet);
    subinst->getInstTerm(subinTerm)->setNet(inNet);
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
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
    EXPECT_EQ(dnl->getDNLIsoDB().getNumNonEmptyIsos(), 2);
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
    EXPECT_EQ(dnl->isInstanceChild(0, 1), true);
    EXPECT_EQ(dnl->isInstanceChild(0, 2), true);
    EXPECT_EQ(dnl->isInstanceChild(1, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 1), false);
    DNLComplexIso complexIso0;
    dnl->getCustomIso(inIsoID, complexIso0);
    EXPECT_EQ(complexIso0.getDrivers().size(), 1);
    EXPECT_EQ(complexIso0.getReaders().size(), 1);
    EXPECT_EQ(complexIso0.getHierTerms().size(), 1);
    DNLComplexIso complexIso1;
    dnl->getCustomIso(outIsoID, complexIso1);
    EXPECT_EQ(complexIso1.getDrivers().size(), 1);
    EXPECT_EQ(complexIso1.getReaders().size(), 1);
    EXPECT_EQ(complexIso1.getHierTerms().size(), 1);
    dnl->display();
    dnl->getTop().display();
    dnl->getTop().getChildInstance(subinst).display();
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subsubinst->getInstTerm(subsubinTerm)).isNull(), true);  
    EXPECT_EQ(dnl->getTop().getChildInstance(subsubinst).isNull(), true); 
    EXPECT_EQ(dnl->getDNLNullInstance().isNull(), true);
    EXPECT_EQ(dnl->getDNLNullTerminal().isNull(), true);
    //Destroy the DNL
    destroy();
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
TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDBNonMT) {
    setenv("NON_MT", "", 1);
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
    //Connect the output of the second sub module to the output of the first sub module
    auto subOutNet = SNLScalarNet::create(submod, NLName("submodnet"));
    suboutTerm->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
    //Connect the output of the first sub module to the input of the top module
    auto outNet = SNLScalarNet::create(mod, NLName("modnet"));
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
    DNLFull* dnl = get();
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
    EXPECT_EQ(dnl->getDNLIsoDB().getNumNonEmptyIsos(), 2);
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
    EXPECT_EQ(dnl->isInstanceChild(0, 1), true);
    EXPECT_EQ(dnl->isInstanceChild(0, 2), true);
    EXPECT_EQ(dnl->isInstanceChild(1, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 1), false);
    DNLComplexIso complexIso0;
    dnl->getCustomIso(inIsoID, complexIso0);
    EXPECT_EQ(complexIso0.getDrivers().size(), 1);
    EXPECT_EQ(complexIso0.getReaders().size(), 1);
    EXPECT_EQ(complexIso0.getHierTerms().size(), 1);
    DNLComplexIso complexIso1;
    dnl->getCustomIso(outIsoID, complexIso1);
    EXPECT_EQ(complexIso1.getDrivers().size(), 1);
    EXPECT_EQ(complexIso1.getReaders().size(), 1);
    EXPECT_EQ(complexIso1.getHierTerms().size(), 1);
    dnl->display();
    dnl->getTop().display();
    dnl->getTop().getChildInstance(subinst).display();
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subsubinst->getInstTerm(subsubinTerm)).isNull(), true);  
    EXPECT_EQ(dnl->getTop().getChildInstance(subsubinst).isNull(), true); 
    EXPECT_EQ(dnl->getDNLNullInstance().isNull(), true);
    EXPECT_EQ(dnl->getDNLNullTerminal().isNull(), true);
    //Destroy the DNL
    destroy();
}

//Based on last test, only with multi driver
TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDBWithMultiDriverMT) {
    //Create one module snl with one input and one output
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto inTerm2 = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in2"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
    auto subsuboutTerm2 = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout2"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
    //Connect the output of the second sub module to the output of the first sub module
    auto subOutNet = SNLScalarNet::create(submod, NLName("submodnet"));
    suboutTerm->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm2)->setNet(subOutNet);
    //Connect the output of the first sub module to the input of the top module
    auto outNet = SNLScalarNet::create(mod, NLName("modnet"));
    outTerm->setNet(outNet);
    subinst->getInstTerm(suboutTerm)->setNet(outNet);
    //Connect the input of the second sub module to the input of the first submodule module
    auto subsInNet = SNLScalarNet::create(submod);
    subinTerm->setNet(subsInNet);
    subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
    //Connect the input of the first sub module to the input of the top module
    auto inNet = SNLScalarNet::create(mod);
    inTerm->setNet(inNet);
    inTerm2->setNet(inNet);
    subinst->getInstTerm(subinTerm)->setNet(inNet);
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
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
    //Validate the iso db
    EXPECT_EQ(dnl->getDNLIsoDB().getNumNonEmptyIsos(), 2);
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
    dnl->getDNLIsoDB().display();
    EXPECT_EQ(isoIn.getDrivers().size(), 2);
    EXPECT_EQ(isoOut.getDrivers().size(), 2);
    EXPECT_EQ(isoIn.getReaders().size(), 1);
    EXPECT_EQ(isoOut.getReaders().size(), 1);
    EXPECT_EQ(dnl->isInstanceChild(0, 1), true);
    EXPECT_EQ(dnl->isInstanceChild(0, 2), true);
    EXPECT_EQ(dnl->isInstanceChild(1, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 1), false);
    DNLComplexIso complexIso0;
    dnl->getCustomIso(inIsoID, complexIso0);
    EXPECT_EQ(complexIso0.getDrivers().size(), 2);
    EXPECT_EQ(complexIso0.getReaders().size(), 1);
    EXPECT_EQ(complexIso0.getHierTerms().size(), 1);
    DNLComplexIso complexIso1;
    dnl->getCustomIso(outIsoID, complexIso1);
    EXPECT_EQ(complexIso1.getDrivers().size(), 2);
    EXPECT_EQ(complexIso1.getReaders().size(), 1);
    EXPECT_EQ(complexIso1.getHierTerms().size(), 1);
    dnl->display();
    dnl->getTop().display();
    dnl->getTop().getChildInstance(subinst).display();
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subsubinst->getInstTerm(subsubinTerm)).isNull(), true);  
    EXPECT_EQ(dnl->getTop().getChildInstance(subsubinst).isNull(), true); 
    EXPECT_EQ(dnl->getDNLNullInstance().isNull(), true);
    EXPECT_EQ(dnl->getDNLNullTerminal().isNull(), true);
    //Destroy the DNL
    destroy();
}

//Based on last test, only with multi driver
TEST_F(DNLTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDBWithMultiDriverNonMT) {
    //Create one module snl with one input and one output
    setenv("NON_MT", "", 1);
    NLUniverse* univ = NLUniverse::create();
    NLDB* db = NLDB::create(univ);
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    univ->setTopDesign(mod);
    auto inTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
    auto inTerm2 = SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in2"));
    auto outTerm = SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    //Create a sub module snl with one input and one output
    SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
    auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin"));
    auto subinTerm2 = SNLScalarTerm::create(submod, SNLTerm::Direction::Input, NLName("subin2"));
    auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output, NLName("subout"));
    SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
    SNLInstance* subinst2 = SNLInstance::create(mod, submod, NLName("subinst2"));
    //Create a sub module snl with one input and one output
    SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
    auto subsubinTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
    auto subsuboutTerm = SNLScalarTerm::create(subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
    SNLInstance* subsubinst = SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
    SNLInstance* subsubinst2 = SNLInstance::create(submod, subsubmod, NLName("subsubinst2"));
    //Connect the output of the second sub module to the output of the first sub module
    auto subOutNet = SNLScalarNet::create(submod, NLName("submodnet"));
    subOutNet->setType(SNLNet::Type::Assign1);
    suboutTerm->setNet(subOutNet);
    subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
    //Connect the output of the first sub module to the input of the top module
    auto outNet = SNLScalarNet::create(mod, NLName("modnet"));
    outNet->setType(SNLNet::Type::Assign1);
    outTerm->setNet(outNet);
    subinst->getInstTerm(suboutTerm)->setNet(outNet);
    //Connect the input of the second sub module to the input of the first submodule module
    //auto subsInNet = SNLScalarNet::create(submod);
    subinTerm->setNet(subOutNet);
    subinTerm2->setNet(subOutNet);
    subsubinst->getInstTerm(subsubinTerm)->setNet(subOutNet);
    subsubinst2->getInstTerm(subsubinTerm)->setNet(subOutNet);
    //Connect the input of the first sub module to the input of the top module
    auto inNet = SNLScalarNet::create(mod);
    inNet->setType(SNLNet::Type::Assign1);
    inTerm->setNet(inNet);
    inTerm2->setNet(inNet);
    subinst->getInstTerm(subinTerm)->setNet(inNet);
    subinst2->getInstTerm(subinTerm)->setNet(inNet);
    //Create a DNL on top of the SNL
    DNLFull* dnl = get();
    dnl->getDNLIsoDB().display();
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
    printf("Full path: %s\n", dnl->getTop().getChildInstance(subinst).getChildInstance(subsubinst).getFullPath().c_str());
    //Validate the iso db
    EXPECT_EQ(dnl->getDNLIsoDB().getNumNonEmptyIsos(), 1);
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
    EXPECT_EQ(isoIn.getDrivers().size(), 4);
    EXPECT_EQ(isoOut.getDrivers().size(), 4);
    EXPECT_EQ(isoIn.getReaders().size(), 5);
    EXPECT_EQ(isoOut.getReaders().size(), 5);
    EXPECT_EQ(dnl->isInstanceChild(0, 1), true);
    EXPECT_EQ(dnl->isInstanceChild(0, 2), true);
    EXPECT_EQ(dnl->isInstanceChild(1, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 0), false);
    EXPECT_EQ(dnl->isInstanceChild(2, 1), false);
    DNLComplexIso complexIso0;
    dnl->getCustomIso(inIsoID, complexIso0);
    EXPECT_EQ(complexIso0.getDrivers().size(), 4);
    EXPECT_EQ(complexIso0.getReaders().size(), 5);
    EXPECT_EQ(complexIso0.getHierTerms().size(), 6);
    DNLComplexIso complexIso1;
    dnl->getCustomIso(outIsoID, complexIso1);
    EXPECT_EQ(complexIso1.getDrivers().size(), 4);
    EXPECT_EQ(complexIso1.getReaders().size(), 5);
    EXPECT_EQ(complexIso1.getHierTerms().size(), 6);
    dnl->display();
    dnl->getTop().display();
    dnl->getTop().getChildInstance(subinst).display();
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subsubinst->getInstTerm(subsubinTerm)).isNull(), true);  
    EXPECT_EQ(dnl->getTop().getChildInstance(subsubinst).isNull(), true); 
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subsubinTerm).isNull(), true); 
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subsubinst->getInstTerm(subsubinTerm)).isNull(), true); 
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminalFromBitTerm(subinTerm).isNull(), false); 
    EXPECT_EQ(dnl->getTop().getChildInstance(subinst).getTerminal(subinst->getInstTerm(subinTerm)).isNull(), false); 
    EXPECT_EQ(dnl->getDNLNullInstance().isNull(), true);
    EXPECT_EQ(dnl->getDNLNullTerminal().isNull(), true);
    //Destroy the DNL
    EXPECT_EQ(isCreated(), true);
    std::vector<NLID::DesignObjectID> path;
    path.push_back(0);
    path.push_back(0);
    std::string id("");
    SNLUniquifier uniquifier(path, id);
    uniquifier.process();   
    uniquifier.getFullPath();
    destroy();
}
