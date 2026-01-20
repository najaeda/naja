// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <string>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "NLName.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"

#include "SNLNetlist0.h"

using namespace naja::NL;

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

BENCHMARK_MAIN();
