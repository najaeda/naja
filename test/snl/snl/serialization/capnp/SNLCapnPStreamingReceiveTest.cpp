#include "gtest/gtest.h"

#include <ifaddrs.h>

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapnPStreamingReceiveTest: public ::testing::Test {
  //Test libraries
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      //
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
  protected:
    SNLDB*      db_;
};

using path = std::filesystem::path;

namespace {

SNLDB* receive(uint16_t port) {
  return SNLCapnP::receive(port);
}

}

TEST_F(SNLCapnPStreamingReceiveTest, test0) {
  #if 0
  //std::future<SNLDB*> a = std::async(std::launch::async, receive, 44444);
  SNLDB* db = nullptr;
  std::thread receiverThread([&]{ std::cerr << "Starting receiver" << std::endl; db = SNLCapnP::receive(44444); });
  receiverThread.detach();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  //auto t = std::thread();

  //std::thread senderThread([&]{ SNLCapnP::send(db_, "localhost", 44444); });
  SNLCapnP::send(db_, "127.0.0.1", 44444, 2);
  //senderThread.detach();

  ASSERT_NE(nullptr, db);
  
  //path snlSenderPath(path(SNL_CAPNP_TEST_PATH) / path("snlSenderTest"));
  //std::string snlSenderCommand(snlSenderPath.string() + " 127.0.0.1 44444 test.txt");
  //
  //std::cerr << snlSenderCommand << std::endl;

  //int status = std::system(snlSenderPath.c_str());
  //std::cerr << "akecoucou1 " << status << std::endl;
  //ASSERT_NE(0, WIFEXITED(status));

  //ASSERT_NE(nullptr, db);
  //std::cerr << "akecoucou2" << std::endl;

  
  SNLCapnP::dump(db_, outPath);
  SNLUniverse::get()->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(nullptr, db_->getTopDesign());
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  EXPECT_EQ(2, db_->getLibraries().size());
  using Libraries = std::vector<SNLLibrary*>;
  Libraries libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto library = libraries[0];
  ASSERT_TRUE(library);
  EXPECT_EQ(SNLID::LibraryID(0), library->getID());
  EXPECT_EQ(SNLName("ROOT"), library->getName());
  EXPECT_TRUE(library->isRoot());
  EXPECT_EQ(SNLLibrary::Type::Standard, library->getType());
  EXPECT_TRUE(library->isStandard());
  EXPECT_TRUE(library->getDesigns().empty());
  EXPECT_EQ(2, library->getLibraries().size());
  libraries = Libraries(library->getLibraries().begin(), library->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib1 = libraries[0];
  EXPECT_NE(nullptr, lib1);
  EXPECT_EQ(SNLName("lib1"), lib1->getName());
  EXPECT_TRUE(library->isRoot());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(library, lib1->getParentLibrary());
  auto lib2 = libraries[1];
  EXPECT_NE(nullptr, lib2);
  EXPECT_TRUE(lib2->isAnonymous());
  EXPECT_EQ(library, lib2->getParentLibrary());
  EXPECT_EQ(2, lib2->getLibraries().size());
  libraries = Libraries(lib2->getLibraries().begin(), lib2->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib3 = libraries[0];
  EXPECT_NE(nullptr, lib3);
  EXPECT_EQ(SNLName("lib3"), lib3->getName());
  EXPECT_TRUE(lib3->getLibraries().empty());
  auto lib4 = libraries[1];
  EXPECT_NE(nullptr, lib4);
  EXPECT_TRUE(lib4->isAnonymous());
  EXPECT_TRUE(lib4->getLibraries().empty());

  libraries = Libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  auto primitives = libraries[1];
  EXPECT_EQ(SNLName("PRIMITIVES"), primitives->getName());
  EXPECT_TRUE(primitives->isRoot());
  EXPECT_TRUE(primitives->isPrimitives());
  EXPECT_FALSE(primitives->isStandard());
  EXPECT_FALSE(primitives->isInDB0());
  libraries = Libraries(primitives->getLibraries().begin(), primitives->getLibraries().end());
  EXPECT_EQ(2, libraries.size());
  auto prims1 = libraries[0];
  EXPECT_NE(nullptr, prims1);
  EXPECT_TRUE(prims1->isAnonymous());
  EXPECT_TRUE(prims1->isPrimitives());
  EXPECT_TRUE(prims1->getDesigns().empty());
  EXPECT_EQ(1, prims1->getLibraries().size());
  auto prims2 = libraries[1];
  EXPECT_NE(nullptr, prims2);
  EXPECT_FALSE(prims2->isAnonymous());
  EXPECT_TRUE(prims2->isPrimitives());
  EXPECT_EQ(1, prims2->getDesigns().size());
  auto prim = *(prims2->getDesigns().begin());
  EXPECT_NE(nullptr, prim);
  EXPECT_TRUE(prim->isPrimitive());
  EXPECT_FALSE(prim->isAnonymous());

  libraries = Libraries(prims1->getLibraries().begin(), prims1->getLibraries().end());
  EXPECT_EQ(1, libraries.size());
  auto prims3 = libraries[0];
  EXPECT_NE(nullptr, prims3);
  EXPECT_FALSE(prims3->isAnonymous());
  EXPECT_EQ(SNLName("prims3"), prims3->getName());
  #endif
}