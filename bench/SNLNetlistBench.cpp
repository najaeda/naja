// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <filesystem>
#include <string>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "NLName.h"
#include "SNLCapnP.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"

#include "SNLNetlist0.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "."
#endif

static size_t getDumpSize(const std::filesystem::path& dump_path) {
  std::error_code ec;
  size_t total = 0;
  auto interface_path = dump_path / std::string(SNLCapnP::InterfaceName);
  auto implementation_path = dump_path / std::string(SNLCapnP::ImplementationName);
  auto interface_size = std::filesystem::file_size(interface_path, ec);
  if (!ec) {
    total += interface_size;
  }
  ec.clear();
  auto implementation_size = std::filesystem::file_size(implementation_path, ec);
  if (!ec) {
    total += implementation_size;
  }
  return total;
}

static void BM_CreateNetlist0(benchmark::State& state) {
  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    SNLNetlist0::create(db);
    benchmark::DoNotOptimize(SNLNetlist0::getTop());
    NLUniverse::get()->destroy();
  }
}
BENCHMARK(BM_CreateNetlist0);

static void BM_CreateInstances(benchmark::State& state) {
  const int instance_count = static_cast<int>(state.range(0));
  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    auto lib = NLLibrary::create(db, NLName("BENCH"));
    auto model = SNLDesign::create(lib, NLName("MODEL"));
    SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
    SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
    auto top = SNLDesign::create(lib, NLName("TOP"));

    for (int i = 0; i < instance_count; ++i) {
      std::string name = "u" + std::to_string(i);
      SNLInstance::create(top, model, NLName(name));
    }

    benchmark::DoNotOptimize(top);
    NLUniverse::get()->destroy();
  }
  state.SetItemsProcessed(state.iterations() * instance_count);
}
BENCHMARK(BM_CreateInstances)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_TraversalInstances(benchmark::State& state) {
  const int instance_count = static_cast<int>(state.range(0));
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("BENCH_TRAVERSAL"));
  auto model = SNLDesign::create(lib, NLName("MODEL"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Output, NLName("o"));
  auto top = SNLDesign::create(lib, NLName("TOP"));

  for (int i = 0; i < instance_count; ++i) {
    std::string name = "u" + std::to_string(i);
    SNLInstance::create(top, model, NLName(name));
  }

  for (auto _ : state) {
    size_t count = 0;
    for (auto instance: top->getInstances()) {
      benchmark::DoNotOptimize(instance);
      ++count;
    }
    benchmark::DoNotOptimize(count);
  }

  NLUniverse::get()->destroy();
  state.SetItemsProcessed(state.iterations() * instance_count);
}
BENCHMARK(BM_TraversalInstances)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_LoadVerilogFile(benchmark::State& state, const char* filename) {
  const std::filesystem::path benchmark_path =
    std::filesystem::path(SNL_VRL_BENCHMARKS_PATH) / filename;
  const auto file_size = std::filesystem::file_size(benchmark_path);

  for (auto _ : state) {
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    auto lib = NLLibrary::create(db, NLName("VRL"));
    SNLVRLConstructor constructor(lib);
    constructor.parse(benchmark_path);
    constructor.setFirstPass(false);
    constructor.parse(benchmark_path);
    benchmark::DoNotOptimize(lib);
    NLUniverse::get()->destroy();
  }

  state.SetBytesProcessed(state.iterations() * file_size);
}
BENCHMARK_CAPTURE(BM_LoadVerilogFile, Gates0, "test_gates0.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, FullAdder, "fulladder.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, Gates2, "test_gates2.v");
BENCHMARK_CAPTURE(BM_LoadVerilogFile, LargeHierGates, "large_hier_gates.v");

static void BM_HierarchyTraversal(benchmark::State& state) {
  const std::filesystem::path benchmark_path =
    std::filesystem::path(SNL_VRL_BENCHMARKS_PATH) / "large_hier_gates.v";
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto lib = NLLibrary::create(db, NLName("VRL_HIER"));
  SNLVRLConstructor constructor(lib);
  constructor.parse(benchmark_path);
  constructor.setFirstPass(false);
  constructor.parse(benchmark_path);
  auto top = SNLUtils::findTop(lib);

  for (auto _ : state) {
    SNLUtils::SortedDesigns sorted;
    SNLUtils::getDesignsSortedByHierarchicalLevel(top, sorted);
    benchmark::DoNotOptimize(sorted);
  }

  NLUniverse::get()->destroy();
}
BENCHMARK(BM_HierarchyTraversal);

static void BM_CapnPSerialize(benchmark::State& state) {
  const auto base_path = std::filesystem::temp_directory_path() / "snl_bench_dump";
  size_t run_id = 0;
  size_t bytes = 0;

  for (auto _ : state) {
    state.PauseTiming();
    auto universe = NLUniverse::create();
    auto db = NLDB::create(universe);
    SNLNetlist0::create(db);
    auto out_path = base_path / ("run_" + std::to_string(run_id++));
    std::error_code ec;
    std::filesystem::remove_all(out_path, ec);
    std::filesystem::create_directories(out_path, ec);
    state.ResumeTiming();

    SNLCapnP::dump(db, out_path);
    state.PauseTiming();
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
    state.ResumeTiming();

    auto loaded = SNLCapnP::load(out_path);
    benchmark::DoNotOptimize(loaded);

    state.PauseTiming();
    bytes = getDumpSize(out_path);
    std::filesystem::remove_all(out_path, ec);
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
    state.ResumeTiming();
  }
  if (bytes > 0) {
    state.SetBytesProcessed(state.iterations() * bytes);
  }
}
BENCHMARK(BM_CapnPSerialize);

BENCHMARK_MAIN();
