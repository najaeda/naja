// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "PNLScalarTerm.h"
// #include "PNLBusTerm.h"
#include "PNLScalarNet.h"
// #include "PNLBusNet.h"
// #include "PNLBusNetBit.h"

using namespace naja::NL;

class PNLNetTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
      auto library = NLLibrary::create(db, NLName("LIB"));
      design_ = PNLDesign::create(library, NLName("Design"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    PNLDesign*  design_;
};

TEST_F(PNLNetTest, testCreation) {
  //Create Model
  auto db = design_->getDB();
  ASSERT_TRUE(db);
  auto primitives = db->getLibrary(NLName("PRIMITIVES"));
  EXPECT_EQ(0, primitives->getID());
  ASSERT_TRUE(primitives);
  EXPECT_TRUE(primitives->isPrimitives());
  auto primitive = PNLDesign::create(primitives, PNLDesign::Type::Primitive, NLName(""));
  ASSERT_TRUE(primitive);
  EXPECT_TRUE(primitive->isPrimitive());
  EXPECT_TRUE(primitive->isLeaf());
  PNLScalarTerm::create(primitive, PNLTerm::Direction::Input, NLName("i0"));
  PNLScalarTerm::create(primitive, PNLTerm::Direction::Input, NLName("i1"));
  PNLScalarTerm::create(primitive, PNLTerm::Direction::Output, NLName("o"));

  ASSERT_TRUE(design_);
  auto lib = design_->getLibrary();
  ASSERT_TRUE(lib);
  EXPECT_EQ(1, lib->getID());
  EXPECT_EQ(NLName("Design"), design_->getName());
  EXPECT_EQ(0, design_->getID());
  EXPECT_FALSE(design_->isAnonymous());
  EXPECT_EQ(design_, design_->getLibrary()->getPNLDesign(0));
  EXPECT_EQ(design_, design_->getLibrary()->getPNLDesign(NLName("Design")));
  EXPECT_TRUE(design_->getNets().empty());
  EXPECT_TRUE(design_->getScalarNets().empty());
  EXPECT_TRUE(design_->getScalarNets().getParentTypeCollection<PNLBitNet*>().getSubCollection<PNLScalarNet*>().empty());
  //EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_TRUE(design_->getBitNets().empty());
  EXPECT_TRUE(design_->getBitNets().getParentTypeCollection<PNLDesignObject*>().empty());
  //EXPECT_TRUE(design_->getBitNets().getParentTypeCollection<PNLDesignObject*>().getSubCollection<PNLBusNetBit*>().empty());
  EXPECT_EQ(0, design_->getNets().size());
  EXPECT_EQ(0, design_->getScalarNets().size());
  EXPECT_EQ(0, design_->getScalarNets().getParentTypeCollection<PNLBitNet*>().getSubCollection<PNLScalarNet*>().size());
  //EXPECT_EQ(0, design_->getBusNets().size());
  EXPECT_EQ(0, design_->getBitNets().size());
  EXPECT_EQ(0, design_->getBitNets().getParentTypeCollection<PNLDesignObject*>().size());
  //EXPECT_EQ(0, design_->getBitNets().getParentTypeCollection<PNLDesignObject*>().getSubCollection<PNLBusNetBit*>().size());

  auto i0Term = PNLScalarTerm::create(design_, PNLTerm::Direction::Input, NLName("I0"));
  auto i1Term = PNLScalarTerm::create(design_, PNLTerm::Direction::Input, NLName("I1"));
  //auto oTerm = PNLBusTerm::create(design_, PNLTerm::Direction::Input, 31, 0, NLName("O"));

  auto i0Net = PNLScalarNet::create(design_);
  EXPECT_EQ(0, i0Net->getID());
  EXPECT_EQ(NLID(NLID::Type::Net, 1, 1, 0, 0, 0, 0), i0Net->getNLID());
  EXPECT_EQ(NLID::DesignObjectReference(1, 1, 0, 0), i0Net->getReference());
  //EXPECT_EQ(i0Net, NLUniverse::get()->getNet(NLID::DesignObjectReference(1, 1, 0, 0)));
  //EXPECT_EQ(i0Net, NLUniverse::get()->getObject(i0Net->getNLID())); //TODO <- solve
  EXPECT_FALSE(design_->getNets().empty());
  EXPECT_FALSE(design_->getScalarNets().empty());
  EXPECT_FALSE(design_->getBitNets().empty());
  //EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_EQ(1, design_->getNets().size());
  EXPECT_EQ(1, design_->getScalarNets().size());
  EXPECT_EQ(1, design_->getBitNets().size());
  EXPECT_EQ(i0Net, design_->getScalarNet(0));
  // EXPECT_EQ(i0Net, design_->getBitNet(0, 0));
  // EXPECT_EQ(i0Net, design_->getBitNet(0, 8));
  EXPECT_FALSE(i0Net->getBits().empty());
  EXPECT_EQ(1, i0Net->getBits().size());
  EXPECT_TRUE(i0Net->getComponents().empty());
  EXPECT_TRUE(i0Net->getInstTerms().empty());
  EXPECT_TRUE(i0Net->getBitTerms().empty());

  EXPECT_FALSE(i0Term->getNet());
  i0Term->setNet(i0Net);
  EXPECT_EQ(i0Net, i0Term->getNet());

  EXPECT_FALSE(i0Net->getComponents().empty());
  EXPECT_EQ(1, i0Net->getComponents().size());
  EXPECT_TRUE(i0Net->getInstTerms().empty());
  EXPECT_FALSE(i0Net->getBitTerms().empty());
  EXPECT_EQ(1, i0Net->getBitTerms().size());

  auto i1Net = PNLScalarNet::create(design_);
  EXPECT_EQ(1, i1Net->getID());
  EXPECT_EQ(NLID(NLID::Type::Net, 1, 1, 0, 1, 0, 0), i1Net->getNLID());
  EXPECT_EQ(NLID::DesignObjectReference(1, 1, 0, 1), i1Net->getReference());
  //EXPECT_EQ(i1Net, NLUniverse::get()->getNet(NLID::DesignObjectReference(1, 1, 0, 1)));
  //EXPECT_EQ(i1Net, NLUniverse::get()->getObject(i1Net->getNLID()));
  //EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(NLID::Type::Net, 1, 1, 0, 2, 0, 0)));
  EXPECT_FALSE(i1Net->getBits().empty());
  EXPECT_EQ(1, i1Net->getBits().size());
  EXPECT_FALSE(design_->getNets().empty());
  EXPECT_FALSE(design_->getScalarNets().empty());
  EXPECT_FALSE(design_->getBitNets().empty());
  //EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_EQ(2, design_->getNets().size());
  EXPECT_EQ(2, design_->getScalarNets().size());
  EXPECT_EQ(1+1, design_->getBitNets().size());
  EXPECT_EQ(i1Net, design_->getNet(1));
  EXPECT_EQ(i1Net, design_->getScalarNet(1));
  //EXPECT_EQ(nullptr, design_->getBusNet(1)); 
  EXPECT_EQ(nullptr, design_->getNet(2)); 
  EXPECT_EQ(nullptr, design_->getScalarNet(2)); 
  //EXPECT_EQ(nullptr, design_->getBusNet(2)); 

  EXPECT_FALSE(i1Term->getNet());
  i1Term->setNet(i1Net);
  EXPECT_EQ(i1Net, i1Term->getNet());

  EXPECT_EQ(PNLNet::Type::Standard ,i0Net->getType());
  i0Net->setType(PNLBitNet::Type::Assign0);
  EXPECT_EQ(PNLNet::Type::Assign0 ,i0Net->getType());
  EXPECT_TRUE(i0Net->isConstant0());
  i0Net->setType(PNLBitNet::Type::Supply1);
  EXPECT_EQ(PNLNet::Type::Supply1 ,i0Net->getType());
  EXPECT_FALSE(i0Net->isSupply0());
  EXPECT_TRUE(i0Net->isSupply1());

  auto instance0 = PNLInstance::create(design_, primitive, NLName("instance0"));
  auto instance1 = PNLInstance::create(design_, primitive, NLName("instance1"));
  auto instance2 = PNLInstance::create(design_, primitive, NLName("instance2"));

  EXPECT_FALSE(instance0->isBlackBox());
  EXPECT_TRUE(instance0->isPrimitive());
  EXPECT_TRUE(instance0->isLeaf());

  // PNLBusNet* net0 = PNLBusNet::create(design_, 31, 0, NLName("net0"));
  // ASSERT_TRUE(net0);
  // EXPECT_EQ(NLName("net0"), net0->getName());
  // EXPECT_EQ(2, net0->getID());
  // EXPECT_EQ(31, net0->getMSB());
  // EXPECT_EQ(0, net0->getLSB());
  // EXPECT_EQ(32, net0->getWidth());
  // EXPECT_EQ(design_, net0->getDesign());
  // EXPECT_FALSE(net0->isAnonymous());
  // EXPECT_EQ(net0, design_->getNet(2));
  // EXPECT_EQ(net0, design_->getNet(NLName("net0")));
  // EXPECT_EQ(net0, design_->getBusNet(NLName("net0")));
  // EXPECT_EQ(nullptr, design_->getScalarNet(NLName("net0")));
  // EXPECT_EQ(NLID(NLID::Type::Net, 1, 1, 0, 2, 0, 0), net0->getNLID());
  // EXPECT_EQ(NLID::DesignObjectReference(1, 1, 0, 2), net0->getReference());
  // EXPECT_EQ(net0->getBit(1), design_->getBusNetBit(2, 1));
  // EXPECT_EQ(net0->getBit(31), design_->getBusNetBit(2, 31));
  // EXPECT_EQ(net0->getBit(1), design_->getBitNet(2, 1));
  // EXPECT_EQ(net0->getBit(31), design_->getBitNet(2, 31));
  // EXPECT_EQ(nullptr, design_->getBusNetBit(2, 32));
  // EXPECT_EQ(nullptr, design_->getBusNetBit(0, 32)); //is a scalar net
  // EXPECT_EQ(nullptr, design_->getBitNet(2, 32));
  // EXPECT_EQ(i0Net, design_->getBitNet(0, 32)); //is a scalar net 32 not relevant
  // EXPECT_EQ(net0, NLUniverse::get()->getNet(NLID::DesignObjectReference(1, 1, 0, 2)));
  // EXPECT_EQ(net0, design_->getNet(2));
  // EXPECT_EQ(net0, design_->getBusNet(2));
  // EXPECT_FALSE(net0->getBits().empty());
  // EXPECT_EQ(32, net0->getBits().size());
  // EXPECT_EQ(nullptr, design_->getScalarNet(2)); 
  // EXPECT_EQ(nullptr, design_->getNet(3)); 
  // EXPECT_EQ(nullptr, design_->getScalarNet(3)); 
  // EXPECT_EQ(nullptr, design_->getBusNet(3)); 
  // EXPECT_FALSE(design_->getNets().empty());
  // EXPECT_FALSE(design_->getScalarNets().empty());
  // EXPECT_FALSE(design_->getBitNets().empty());
  // EXPECT_FALSE(design_->getBusNets().empty());
  // EXPECT_EQ(3, design_->getNets().size());
  // EXPECT_EQ(2, design_->getScalarNets().size());
  // EXPECT_EQ(1, design_->getBusNets().size());
  // EXPECT_EQ(1+1+32, design_->getBitNets().size());
  // EXPECT_THAT(std::vector(design_->getNets().begin(), design_->getNets().end()),
  //   ElementsAre(i0Net, i1Net, net0));
  // EXPECT_THAT(std::vector(design_->getScalarNets().begin(), design_->getScalarNets().end()),
  //   ElementsAre(i0Net, i1Net));
  // EXPECT_THAT(std::vector(design_->getBusNets().begin(), design_->getBusNets().end()),
  //   ElementsAre(net0));
  // EXPECT_THAT(std::vector(design_->getBitNets().begin(), design_->getBitNets().end()),
  //   ElementsAre(i0Net, i1Net,
  //     net0->getBit(31), net0->getBit(30), net0->getBit(29), net0->getBit(28),
  //     net0->getBit(27), net0->getBit(26), net0->getBit(25), net0->getBit(24),
  //     net0->getBit(23), net0->getBit(22), net0->getBit(21), net0->getBit(20),
  //     net0->getBit(19), net0->getBit(18), net0->getBit(17), net0->getBit(16),
  //     net0->getBit(15), net0->getBit(14), net0->getBit(13), net0->getBit(12),
  //     net0->getBit(11), net0->getBit(10), net0->getBit(9),  net0->getBit(8),
  //     net0->getBit(7),  net0->getBit(6),  net0->getBit(5),  net0->getBit(4),
  //     net0->getBit(3) , net0->getBit(2),  net0->getBit(1),  net0->getBit(0)));

  // EXPECT_THAT(std::vector(net0->getBit(0)->getBits().begin(), net0->getBit(0)->getBits().end()),
  //   ElementsAre(net0->getBit(0)));
  // EXPECT_THAT(std::vector(net0->getBit(21)->getBits().begin(), net0->getBit(21)->getBits().end()),
  //   ElementsAre(net0->getBit(21)));

  // EXPECT_THAT(std::vector(design_->getBitNets().begin(), design_->getBitNets().end()),
  //   ElementsAre(i0Net, i1Net,
  //     net0->getBitAtPosition(0), net0->getBitAtPosition(1), net0->getBitAtPosition(2),
  //     net0->getBitAtPosition(3), net0->getBitAtPosition(4), net0->getBitAtPosition(5),
  //     net0->getBitAtPosition(6), net0->getBitAtPosition(7), net0->getBitAtPosition(8),
  //     net0->getBitAtPosition(9), net0->getBitAtPosition(10), net0->getBitAtPosition(11),
  //     net0->getBitAtPosition(12), net0->getBitAtPosition(13), net0->getBitAtPosition(14),
  //     net0->getBitAtPosition(15), net0->getBitAtPosition(16), net0->getBitAtPosition(17),
  //     net0->getBitAtPosition(18), net0->getBitAtPosition(19), net0->getBitAtPosition(20),
  //     net0->getBitAtPosition(21), net0->getBitAtPosition(22), net0->getBitAtPosition(23),
  //     net0->getBitAtPosition(24), net0->getBitAtPosition(25), net0->getBitAtPosition(26),
  //     net0->getBitAtPosition(27), net0->getBitAtPosition(28), net0->getBitAtPosition(29),
  //     net0->getBitAtPosition(30), net0->getBitAtPosition(31)));

  // EXPECT_EQ(nullptr, net0->getBit(32));
  // EXPECT_EQ(nullptr, net0->getBit(-1));
  // EXPECT_EQ(nullptr, net0->getBitAtPosition(32));
  // EXPECT_EQ(nullptr, net0->getBitAtPosition(33));

  // NLID::Bit bitNumber = 31;
  // for (auto bit: net0->getBusBits()) {
  //   EXPECT_EQ(PNLNet::Type::Standard, bit->getType());
  //   EXPECT_EQ(net0, bit->getBus());
  //   EXPECT_EQ(net0->getID(), bit->getID());
  //   EXPECT_EQ(design_, bit->getDesign());
  //   EXPECT_FALSE(bit->getType().isDriving());
  //   EXPECT_FALSE(bit->isAnonymous());
  //   //EXPECT_EQ(bit->getName(), bit->getBus()->getName());
  //   EXPECT_EQ(NLID(NLID::Type::NetBit, 1, 1, 0, 2, 0, bitNumber--), bit->getNLID());
  //   //EXPECT_EQ(bit, NLUniverse::get()->getBusNetBit(bit->getNLID()));
  //   EXPECT_EQ(bit, NLUniverse::get()->getObject(bit->getNLID()));
  // }
  // EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(NLID::Type::NetBit, 1, 1, 0, 2, 0, 32)));

  // net0->setType(PNLBitNet::Type::Supply1);
  // EXPECT_FALSE(net0->isSupply0());
  // EXPECT_TRUE(net0->isSupply1());
  // for (auto bit: net0->getBits()) {
  //   EXPECT_EQ(PNLNet::Type::Supply1, bit->getType());
  //   EXPECT_TRUE(bit->getType().isSupply());
  //   EXPECT_TRUE(bit->getType().isDriving());
  //   EXPECT_TRUE(bit->isConstant1());
  // }
  // net0->setType(PNLBitNet::Type::Assign0);
  // EXPECT_FALSE(net0->isSupply0());
  // (net0->isSupply1());
  // for (auto bit: net0->getBits()) {
  //   EXPECT_EQ(PNLNet::Type::Assign0, bit->getType());
  //   EXPECT_TRUE(bit->getType().isDriving());
  //   EXPECT_TRUE(bit->isConstant0());
  // }

  // net0->destroy();
  // EXPECT_EQ(nullptr, design_->getNet(2));
  // EXPECT_EQ(nullptr, design_->getNet(NLName("net0")));

  i1Net->destroy();
  EXPECT_EQ(nullptr, design_->getNet(1));
}

// TEST_F(PNLNetTest, testBusNetBitDestruction) {
//   //PNLBusNet* net0 = PNLBusNet::create(design_, 31, 0, NLName("net0"));
//   EXPECT_EQ(32, net0->getWidth());
//   EXPECT_EQ(32, net0->getBits().size());

//   //destroy bit 3
//   auto bit3 = net0->getBit(3);
//   auto bit3Position = net0->getBitPosition(bit3->getBit());
//   EXPECT_EQ(bit3, net0->getBitAtPosition(bit3Position));
//   ASSERT_NE(nullptr, bit3);
//   bit3->destroy();
//   bit3 = net0->getBit(3);
//   EXPECT_EQ(nullptr, bit3);
//   EXPECT_EQ(31, net0->getBits().size());
//   EXPECT_EQ(nullptr, net0->getBitAtPosition(bit3Position));
// }

TEST_F(PNLNetTest, testNetType) {
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Assign0).isAssign());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Assign1).isAssign());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Supply0).isSupply());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Supply1).isSupply());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Assign0).isDriving());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Assign1).isDriving());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Supply0).isDriving());
  EXPECT_TRUE(PNLNet::Type(PNLNet::Type::Supply1).isDriving());
  EXPECT_FALSE(PNLNet::Type(PNLNet::Type::Standard).isDriving());
}

TEST_F(PNLNetTest, testErrors) {
  EXPECT_THROW(PNLScalarNet::create(nullptr), NLException);
  //EXPECT_THROW(PNLBusNet::create(nullptr, 31, 0), NLException);

  NLDB* db = design_->getDB();
  ASSERT_NE(db, nullptr);
  NLLibrary* library = db->getLibrary(NLName("LIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  PNLScalarNet* net0 = PNLScalarNet::create(design, NLName("net0"));
  ASSERT_NE(nullptr, net0);
  EXPECT_EQ(NLID::DesignObjectID(0), net0->getID());
  PNLScalarNet* net1 = PNLScalarNet::create(design, NLName("net1"));
  //PNLBusNet* net1 = PNLBusNet::create(design, 31, 0, NLName("net1"));
  //ASSERT_NE(nullptr, net1);
  //EXPECT_EQ(NLID::DesignObjectID(1), net1->getID());
  //EXPECT_THROW(PNLBusNet::create(design, 31, 0, NLName("net0")), NLException);
  //EXPECT_THROW(PNLScalarNet::create(design, NLName("net1")), NLException);
  //EXPECT_THROW(PNLBusNet::create(design, NLID::DesignObjectID(0), 31, 0), NLException);
  EXPECT_THROW(PNLScalarNet::create(design, NLID::DesignObjectID(1)), NLException);
  EXPECT_THROW(PNLScalarNet::create(design, NLID::DesignObjectID(1), NLName("conflict")), NLException);
  net1->destroy();
  //create a design
  auto design1 = PNLDesign::create(library, NLName("design1"));
  //create scalar term
  auto scalarTerm1 = PNLScalarTerm::create(design1, PNLTerm::Direction::Input, NLName("term1"));
  //incompatible nets
  EXPECT_THROW(scalarTerm1->setNet(net0), NLException);
}

TEST_F(PNLNetTest, testRename) {
  auto net0 = PNLScalarNet::create(design_, NLName("net0"));
  //auto net1 = PNLBusNet::create(design_, 31, 0, NLName("net1"));
  auto net2 = PNLScalarNet::create(design_);
  EXPECT_EQ(net0, design_->getNet(NLName("net0")));
  //EXPECT_EQ(net1, design_->getNet(NLName("net1")));
  EXPECT_FALSE(net0->isAnonymous());
  net0->setName(NLName());
  EXPECT_TRUE(net0->isAnonymous());
  EXPECT_EQ(nullptr, design_->getNet(NLName("net0")));
  net0->setName(NLName("net0"));
  EXPECT_FALSE(net0->isAnonymous());
  EXPECT_EQ(net0, design_->getNet(NLName("net0")));
  //EXPECT_FALSE(net1->isAnonymous());
  //net1->setName(NLName("net1")); //nothing should happen...
  // EXPECT_EQ(net1, design_->getNet(NLName("net1")));
  // net1->setName(NLName("n1"));
  // EXPECT_FALSE(net1->isAnonymous());
  // EXPECT_EQ(nullptr, design_->getNet(NLName("net1")));
  // EXPECT_EQ(net1, design_->getNet(NLName("n1")));
  EXPECT_TRUE(net2->isAnonymous());
  net2->setName(NLName("net2"));
  EXPECT_FALSE(net2->isAnonymous());
  EXPECT_EQ(net2, design_->getNet(NLName("net2")));
  //Collision error
  //EXPECT_THROW(net1->setName(NLName("net0")), NLException);

  //BusNetBit rename error
  //EXPECT_THROW(net1->getBit(0)->setName(NLName("net0")), NLException);
}
