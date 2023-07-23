#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBitNetOccurrence.h"
#include "SNLInstTermOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLOccurrenceTest: public ::testing::Test {
  protected:
    void SetUp() override {
      // top: i(i)
      //  |-> h0: i(i)
      //       |-> h1: i(i)
      //            |-> h2: i(i)
      //                 |-> prim: i(i)
      // simple test design with i term and net at each level
      // connected to upper and lower level
      // 
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitivesLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
      auto designsLib = SNLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, SNLName("PRIM"));
      auto primi = SNLScalarTerm::create(prim, SNLTerm::Direction::Input, SNLName("i"));
      auto primiNet = SNLScalarNet::create(prim, SNLName("i"));
      primi->setNet(primiNet);
      auto top = SNLDesign::create(designsLib, SNLName("TOP"));
      auto topi = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("i"));
      auto topiNet = SNLScalarNet::create(top, SNLName("i"));
      topi->setNet(topiNet);
      auto h0 = SNLDesign::create(designsLib, SNLName("H0"));
      auto h0i = SNLScalarTerm::create(h0, SNLTerm::Direction::Input, SNLName("i"));
      auto h0iNet = SNLScalarNet::create(h0, SNLName("i"));
      h0i->setNet(h0iNet);
      auto h1 = SNLDesign::create(designsLib, SNLName("H1"));
      auto h1i = SNLScalarTerm::create(h1, SNLTerm::Direction::Input, SNLName("i"));
      auto h1iNet = SNLScalarNet::create(h1, SNLName("i"));
      h1i->setNet(h1iNet);
      auto h2 = SNLDesign::create(designsLib, SNLName("H2"));
      auto h2i = SNLScalarTerm::create(h2, SNLTerm::Direction::Input, SNLName("i"));
      auto h2iNet = SNLScalarNet::create(h2, SNLName("i"));
      h2i->setNet(h2iNet);
      primInstance_ = SNLInstance::create(h2, prim, SNLName("prim"));
      h2Instance_ = SNLInstance::create(h1, h2, SNLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, SNLName("h1"));
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
      primInstance_->getInstTerm(prim->getScalarTerm(SNLName("i")))->setNet(h2->getScalarNet(SNLName("i")));
      h2Instance_->getInstTerm(h2->getScalarTerm(SNLName("i")))->setNet(h1->getScalarNet(SNLName("i")));
      h1Instance_->getInstTerm(h1->getScalarTerm(SNLName("i")))->setNet(h0->getScalarNet(SNLName("i")));
      h0Instance_->getInstTerm(h0->getScalarTerm(SNLName("i")))->setNet(top->getScalarNet(SNLName("i")));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLOccurrenceTest, testhEmptyOccurrences) {
  ASSERT_NE(SNLUniverse::get(), nullptr);
  auto emptyPath = SNLPath();
  auto emptyNetOccurrence = SNLBitNetOccurrence(emptyPath, nullptr);
  auto emptyInstTermOccurrence = SNLInstTermOccurrence(emptyPath, nullptr);
  auto emptyTermOccurrence = SNLBitTermOccurrence(emptyPath, nullptr);
  EXPECT_FALSE(emptyNetOccurrence.isValid());
  EXPECT_EQ(emptyNetOccurrence, emptyInstTermOccurrence);
  EXPECT_EQ(emptyNetOccurrence, SNLInstTermOccurrence());
  EXPECT_FALSE(emptyNetOccurrence < emptyInstTermOccurrence);
  EXPECT_FALSE(emptyInstTermOccurrence < emptyNetOccurrence);
}

TEST_F(SNLOccurrenceTest, testh0Level) {
  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, SNLPath());
  auto iTerm = h0Instance_->getModel()->getScalarTerm(SNLName("i"));
  ASSERT_NE(nullptr, iTerm);
  auto h0iTermOccurrence = SNLBitTermOccurrence(h0Path, iTerm);
  EXPECT_EQ(h0Path, h0iTermOccurrence.getPath());
  EXPECT_EQ(iTerm, h0iTermOccurrence.getTerm());
  EXPECT_EQ(iTerm->getNet(), h0iTermOccurrence.getNet());
  EXPECT_EQ(SNLBitNetOccurrence(h0Path, iTerm->getNet()), h0iTermOccurrence.getNetOccurrence());
  EXPECT_LT(SNLBitTermOccurrence(), h0iTermOccurrence);
  EXPECT_LT(h0iTermOccurrence, SNLBitNetOccurrence(h0Path, iTerm->getNet()));
}

TEST_F(SNLOccurrenceTest, testh1Level) {
  ASSERT_NE(h1Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_);
  auto h1Path = SNLPath(h0Path, h1Instance_);
  auto iTerm = h1Instance_->getModel()->getScalarTerm(SNLName("i"));
  ASSERT_NE(nullptr, iTerm);
  auto h1iTermOccurrence = SNLBitTermOccurrence(h1Path, iTerm);
  EXPECT_EQ(h1Path, h1iTermOccurrence.getPath());
  EXPECT_EQ(iTerm, h1iTermOccurrence.getTerm());
  EXPECT_EQ(iTerm->getNet(), h1iTermOccurrence.getNet());
  EXPECT_EQ(SNLBitNetOccurrence(h1Path, iTerm->getNet()), h1iTermOccurrence.getNetOccurrence());
  EXPECT_LT(SNLBitTermOccurrence(), h1iTermOccurrence);
  EXPECT_LT(h1iTermOccurrence, SNLBitNetOccurrence(h1Path, iTerm->getNet()));
}

TEST_F(SNLOccurrenceTest, testEquipotential0) {
  ASSERT_NE(h0Instance_, nullptr);
  auto top = h0Instance_->getDesign();
  auto topi = top->getScalarTerm(SNLName("i"));
  ASSERT_NE(topi, nullptr);
  SNLEquipotential equipotential(topi);
  EXPECT_THAT(equipotential.getTerms(), ElementsAre(topi));
  SNLPath::PathStringDescriptor h2StringPath = {"h0", "h1", "h2"};
  SNLPath h2Path(top, h2StringPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  auto h2 = h2Path.getModel();
  auto primi = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, primi);
  auto primiOccurrence = SNLInstTermOccurrence(h2Path, primi);
  ASSERT_TRUE(primiOccurrence.isValid());
  EXPECT_THAT(equipotential.getInstTermOccurrences(), ElementsAre(primiOccurrence));
}

TEST_F(SNLOccurrenceTest, testEquipotential1) {
  ASSERT_NE(h0Instance_, nullptr);
  auto top = h0Instance_->getDesign();
  auto topi = top->getScalarTerm(SNLName("i"));
  ASSERT_NE(topi, nullptr);
  SNLPath::PathStringDescriptor h1StringPath = {"h0", "h1"};
  SNLPath h1Path(top, h1StringPath);
  EXPECT_EQ(2, h1Path.size());
  auto h2i = h2Instance_->getInstTerm(h2Instance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, h2i);
  auto h2iOccurrence = SNLInstTermOccurrence(h1Path, h2i);
  SNLEquipotential equipotential(h2iOccurrence);
  EXPECT_THAT(equipotential.getTerms(), ElementsAre(topi));
  SNLPath::PathStringDescriptor h2StringPath = {"h0", "h1", "h2"};
  SNLPath h2Path(top, h2StringPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  auto h2 = h2Path.getModel();
  auto primi = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, primi);
  auto primiOccurrence = SNLInstTermOccurrence(h2Path, primi);
  ASSERT_TRUE(primiOccurrence.isValid());
  EXPECT_THAT(equipotential.getInstTermOccurrences(), ElementsAre(primiOccurrence));
}

#if 0
TEST_F(SNLPathTest, testTopDown1) {
  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_);
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(h0Instance_, h0Path.getTailInstance());
  EXPECT_EQ(h0Instance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(), h0Path.getHeadPath());
  EXPECT_EQ(SNLPath(), h0Path.getTailPath());
  EXPECT_EQ(1, h0Path.size());
  EXPECT_LT(SNLPath(), h0Path);

  ASSERT_NE(h1Instance_, nullptr);
  EXPECT_EQ(h0Instance_->getModel(), h1Instance_->getDesign());
  auto h1Path = SNLPath(h0Path, h1Instance_);
  EXPECT_FALSE(h1Path.empty());
  EXPECT_EQ(2, h1Path.size());
  EXPECT_LT(SNLPath(), h1Path);
  EXPECT_LT(h0Path, h1Path);
  EXPECT_EQ(h0Instance_, h1Path.getHeadInstance());
  EXPECT_EQ(h1Instance_, h1Path.getTailInstance());
  EXPECT_EQ(h1Instance_->getModel(), h1Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(h0Path, h1Path.getHeadPath());
  EXPECT_EQ(SNLPath(h1Instance_), h1Path.getTailPath());

  ASSERT_NE(h2Instance_, nullptr);
  EXPECT_EQ(h1Instance_->getModel(), h2Instance_->getDesign());
  auto h2Path = SNLPath(h1Path, h2Instance_);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  EXPECT_LT(SNLPath(), h2Path);
  EXPECT_LT(h0Path, h2Path);
  EXPECT_LT(h1Path, h2Path);
  EXPECT_EQ(h0Instance_, h2Path.getHeadInstance());
  EXPECT_EQ(h2Instance_, h2Path.getTailInstance());
  EXPECT_EQ(h2Instance_->getModel(), h2Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h2Path.getDesign());
  EXPECT_EQ(h1Path, h2Path.getHeadPath());
  EXPECT_EQ(SNLPath(SNLPath(h1Instance_), h2Instance_), h2Path.getTailPath());

  ASSERT_NE(primInstance_, nullptr);
  EXPECT_EQ(h2Instance_->getModel(), primInstance_->getDesign());
  auto primPath = SNLPath(h2Path, primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(4, primPath.size());
  EXPECT_LT(SNLPath(), primPath);
  EXPECT_LT(h0Path, primPath);
  EXPECT_LT(h1Path, primPath);
  EXPECT_LT(h2Path, primPath);
  EXPECT_EQ(h0Instance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(h2Path, primPath.getHeadPath());
  EXPECT_EQ(SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), primInstance_), primPath.getTailPath());
}

TEST_F(SNLPathTest, testBottomUp0) {
  ASSERT_NE(primInstance_, nullptr);
  auto primPath = SNLPath(SNLPath(), primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(primInstance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(primInstance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(SNLPath(), primPath.getHeadPath());
  EXPECT_EQ(SNLPath(), primPath.getTailPath());

  EXPECT_EQ(SNLPath(primInstance_), primPath);
  EXPECT_EQ(SNLPath(primInstance_, SNLPath()), primPath);
}

TEST_F(SNLPathTest, testBottomUp1) {
  ASSERT_NE(primInstance_, nullptr);
  auto primPath = SNLPath(primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(1, primPath.size());
  EXPECT_LT(SNLPath(), primPath);
  EXPECT_EQ(primInstance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(primInstance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(SNLPath(), primPath.getHeadPath());
  EXPECT_EQ(SNLPath(), primPath.getTailPath());

  ASSERT_NE(h2Instance_, nullptr);
  auto h2Path = SNLPath(h2Instance_, primPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(2, h2Path.size());
  EXPECT_LT(SNLPath(), h2Path);
  EXPECT_LT(primPath, h2Path);
  EXPECT_EQ(h2Instance_, h2Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h2Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h2Path.getModel());
  EXPECT_EQ(h2Instance_->getDesign(), h2Path.getDesign());
  EXPECT_EQ(SNLPath(h2Instance_), h2Path.getHeadPath());
  EXPECT_EQ(primPath, h2Path.getTailPath());

  ASSERT_NE(h1Instance_, nullptr);
  auto h1Path = SNLPath(h1Instance_, h2Path);
  EXPECT_FALSE(h1Path.empty());
  EXPECT_EQ(3, h1Path.size());
  EXPECT_LT(SNLPath(), h1Path);
  EXPECT_LT(primPath, h1Path);
  EXPECT_LT(h2Path, h1Path);
  EXPECT_EQ(h1Instance_, h1Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h1Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h1Path.getModel());
  EXPECT_EQ(h1Instance_->getDesign(), h1Path.getDesign());
  EXPECT_EQ(SNLPath(h1Instance_, SNLPath(h2Instance_)), h1Path.getHeadPath());
  EXPECT_EQ(h2Path, h1Path.getTailPath());

  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, h1Path);
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(4, h0Path.size());
  EXPECT_LT(SNLPath(), h0Path);
  EXPECT_LT(primPath, h0Path);
  EXPECT_LT(h2Path, h0Path);
  EXPECT_LT(h1Path, h0Path);
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h0Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_))), h0Path.getHeadPath());
  EXPECT_EQ(h1Path, h0Path.getTailPath());
  EXPECT_NE(h1Path, h0Path);
  SNLPath copyH1Path(h1Path);
  EXPECT_EQ(h1Path, copyH1Path);
  EXPECT_NE(h0Path, copyH1Path);
}

TEST_F(SNLPathTest, comparePaths) {
  EXPECT_EQ(SNLPath(SNLPath(), primInstance_), SNLPath(primInstance_, SNLPath()));
  EXPECT_EQ(
    SNLPath(SNLPath(h2Instance_), primInstance_),
    SNLPath(h2Instance_, SNLPath(primInstance_)));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), primInstance_),
    SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(primInstance_))));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), primInstance_),
    SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(primInstance_)))));
}

TEST_F(SNLPathTest, testErrors) {
  SNLInstance* instance = nullptr;
  SNLPath emptyPath;
  EXPECT_THROW(SNLPath(instance, emptyPath), SNLException);
  EXPECT_THROW(SNLPath(emptyPath, instance), SNLException);
  //incompatible paths
  ASSERT_NE(h0Instance_, nullptr);
  ASSERT_NE(primInstance_, nullptr);
  EXPECT_THROW(SNLPath(SNLPath(h0Instance_), primInstance_), SNLException);
  EXPECT_THROW(SNLPath(h0Instance_, SNLPath(primInstance_)), SNLException);
}

TEST_F(SNLPathTest, testInstanceDestroy) {
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), primInstance_);
    EXPECT_FALSE(path.empty());
  }
  EXPECT_EQ(1, h1Instance_->getModel()->getInstances().size());
  h2Instance_->destroy();
  h2Instance_ = nullptr;
  EXPECT_TRUE(h1Instance_->getModel()->getInstances().empty());
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_)));
    EXPECT_FALSE(path.empty());
  }
}
#endif