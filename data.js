window.BENCHMARK_DATA = {
  "lastUpdate": 1775069001007,
  "repoUrl": "https://github.com/najaeda/naja",
  "entries": {
    "SNL Benchmarks": [
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8aecabcee68458f2389838f86ab8d5d1af06a1d9",
          "message": "add smoke test in docker build test (#327)\n\n* add smoke test in docker build test\n\n* cleaning flows",
          "timestamp": "2026-01-23T17:52:21+01:00",
          "tree_id": "da4d8fbea04c0f5c5c6693a89c6ae7f2da6b321e",
          "url": "https://github.com/najaeda/naja/commit/8aecabcee68458f2389838f86ab8d5d1af06a1d9"
        },
        "date": 1769187363254,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18546.2968472412,
            "unit": "ns/iter",
            "extra": "iterations: 37713\ncpu: 18546.015962665395 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 90277.19020691545,
            "unit": "ns/iter",
            "extra": "iterations: 7781\ncpu: 90250.97108340828 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1193964.589134132,
            "unit": "ns/iter",
            "extra": "iterations: 589\ncpu: 1193594.4906621391 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17309582.1951219,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17307790.878048774 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1451.168940734702,
            "unit": "ns/iter",
            "extra": "iterations: 482542\ncpu: 1450.6785440438348 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14243.095158699767,
            "unit": "ns/iter",
            "extra": "iterations: 49181\ncpu: 14239.870885097898 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216093.3788819867,
            "unit": "ns/iter",
            "extra": "iterations: 3220\ncpu: 216062.3236024845 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 210346.4765931731,
            "unit": "ns/iter",
            "extra": "iterations: 3311\ncpu: 210334.5850196315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 170110.60847333795,
            "unit": "ns/iter",
            "extra": "iterations: 4107\ncpu: 170105.3861699537 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 105136.45782231282,
            "unit": "ns/iter",
            "extra": "iterations: 6686\ncpu: 105131.31453784027 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57161901.09999957,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57156435.79999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24031.684230398794,
            "unit": "ns/iter",
            "extra": "iterations: 29132\ncpu: 24030.978820540942 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167321.93520280733,
            "unit": "ns/iter",
            "extra": "iterations: 4244\ncpu: 164484.28934968158 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "6384a8a7115c9b25cffbb22a00c11d6eecd10bda",
          "message": "xSet msb and set msb on busses (#328)\n\n* can you add associated unit test\n\n* implementation for BusNet\n\n* coverage\n\n* coverage\n\n* coverage\n\n* export to python\n\n* more testing\n\n* support until najaeda level\n\n* improve coverage\n\n* fix for empty busses (no bits)\n\n* boost cleaning\n\n* add early error for getting package on linux\n\n* try to be more resiliient\n\n* go back\n\n* new try\n\n* new attempt\n\n* new try\n\n* re-cleaning",
          "timestamp": "2026-01-27T08:17:22+01:00",
          "tree_id": "89555820944d98b395985809f1d81d68d13d48e4",
          "url": "https://github.com/najaeda/naja/commit/6384a8a7115c9b25cffbb22a00c11d6eecd10bda"
        },
        "date": 1769498480648,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18491.483962915947,
            "unit": "ns/iter",
            "extra": "iterations: 37538\ncpu: 18490.81831743833 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 83702.666387557,
            "unit": "ns/iter",
            "extra": "iterations: 8360\ncpu: 83682.96447368423 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1142456.7843137619,
            "unit": "ns/iter",
            "extra": "iterations: 612\ncpu: 1142389.003267974 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17118129.82926947,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17115534.65853659 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1472.7542961847485,
            "unit": "ns/iter",
            "extra": "iterations: 476993\ncpu: 1472.3138515659557 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14397.532796837853,
            "unit": "ns/iter",
            "extra": "iterations: 48587\ncpu: 14396.64657212834 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 220353.8821506684,
            "unit": "ns/iter",
            "extra": "iterations: 3199\ncpu: 220331.6323851201 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 221163.26644294494,
            "unit": "ns/iter",
            "extra": "iterations: 2980\ncpu: 221156.04999999987 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 177121.5259559466,
            "unit": "ns/iter",
            "extra": "iterations: 3949\ncpu: 177120.73765510242 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 110114.8412823799,
            "unit": "ns/iter",
            "extra": "iterations: 6332\ncpu: 110112.65745420076 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65304869.77778259,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65300436.33333351 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23936.37133694753,
            "unit": "ns/iter",
            "extra": "iterations: 29313\ncpu: 23935.20826936859 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167182.65476903805,
            "unit": "ns/iter",
            "extra": "iterations: 4258\ncpu: 164296.76373884644 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3df926407c3b26e2b6816dfbff69d61a5d93c6e1",
          "message": "B0.3.6 (#329)\n\n* switch to b0.3.6\n\n* more linux distribs tested with Docker\n\n* update\n\n* clean docker build\n\n* use latest\n\n* update version\n\n* switch back to rockylinux\n\n* switch back to rockylinux:9\n\n* setting for rockylinux compile\n\n* new attempt",
          "timestamp": "2026-01-31T00:41:08+01:00",
          "tree_id": "abc16c2aa4cb77609dc1b86856b70e9fb6908e6f",
          "url": "https://github.com/najaeda/naja/commit/3df926407c3b26e2b6816dfbff69d61a5d93c6e1"
        },
        "date": 1769816692126,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18424.09182228578,
            "unit": "ns/iter",
            "extra": "iterations: 37431\ncpu: 18422.051588255727 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 85736.64259530546,
            "unit": "ns/iter",
            "extra": "iterations: 8184\ncpu: 85732.84506353861 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1167395.0450000349,
            "unit": "ns/iter",
            "extra": "iterations: 600\ncpu: 1167336.3466666664 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17384325.560976222,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17382894.70731707 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1502.4185540536457,
            "unit": "ns/iter",
            "extra": "iterations: 466518\ncpu: 1502.3794258742428 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14762.830048641683,
            "unit": "ns/iter",
            "extra": "iterations: 47696\ncpu: 14760.516521301573 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206769.39368670987,
            "unit": "ns/iter",
            "extra": "iterations: 3358\ncpu: 206756.33740321617 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 220078.2773400634,
            "unit": "ns/iter",
            "extra": "iterations: 3173\ncpu: 220046.17239205795 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 174425.03028785173,
            "unit": "ns/iter",
            "extra": "iterations: 3995\ncpu: 174410.67934918642 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 108437.88214395924,
            "unit": "ns/iter",
            "extra": "iterations: 6474\ncpu: 108435.41581711457 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65641815.22221841,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65637838.33333324 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24079.536305557016,
            "unit": "ns/iter",
            "extra": "iterations: 29114\ncpu: 24077.840111286696 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164025.01619843257,
            "unit": "ns/iter",
            "extra": "iterations: 4260\ncpu: 163626.48051644646 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c89decf6faf85d152d38aa561994cab9111a3e29",
          "message": "integrating naja-verilog preprocessor (#330)",
          "timestamp": "2026-02-02T15:27:03+01:00",
          "tree_id": "7dcff09da26ed504aa752df5fc3caaa811ee3f37",
          "url": "https://github.com/najaeda/naja/commit/c89decf6faf85d152d38aa561994cab9111a3e29"
        },
        "date": 1770042662479,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 22382.665706932523,
            "unit": "ns/iter",
            "extra": "iterations: 32648\ncpu: 22375.00928081353 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 96033.43609880505,
            "unit": "ns/iter",
            "extra": "iterations: 7003\ncpu: 96028.08139368842 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1257657.827648079,
            "unit": "ns/iter",
            "extra": "iterations: 557\ncpu: 1257597.3231597845 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19200362.24999914,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19200052.083333332 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1503.1751816805725,
            "unit": "ns/iter",
            "extra": "iterations: 464552\ncpu: 1502.927700235927 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14534.245013188252,
            "unit": "ns/iter",
            "extra": "iterations: 48528\ncpu: 14533.42711424332 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 218902.13520021347,
            "unit": "ns/iter",
            "extra": "iterations: 3321\ncpu: 218881.58084914193 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 222070.10508157726,
            "unit": "ns/iter",
            "extra": "iterations: 3188\ncpu: 222027.20075282306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 175682.0604616152,
            "unit": "ns/iter",
            "extra": "iterations: 3986\ncpu: 175634.0429001505 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 109501.29390625563,
            "unit": "ns/iter",
            "extra": "iterations: 6400\ncpu: 109481.15625000004 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65045299.22221991,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65039870.66666659 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23828.166898752363,
            "unit": "ns/iter",
            "extra": "iterations: 29443\ncpu: 23827.491322215763 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 165866.2923445585,
            "unit": "ns/iter",
            "extra": "iterations: 4180\ncpu: 165735.38660288742 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "43e5519d690220a0f015605038e7f33c7baf3a6d",
          "message": "Liberty zlib support (#331)\n\n* towards 0.4.0 adding zlib support in liberty parsing\n\n* more testing\n\n* install zlib on rockylinux\n\n* cleaning coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-02-03T09:46:11+01:00",
          "tree_id": "a939b506831fd318dce800f3d585f61eb006a838",
          "url": "https://github.com/najaeda/naja/commit/43e5519d690220a0f015605038e7f33c7baf3a6d"
        },
        "date": 1770108617000,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 19537.62726213762,
            "unit": "ns/iter",
            "extra": "iterations: 35199\ncpu: 19536.188386033697 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 90626.48643059944,
            "unit": "ns/iter",
            "extra": "iterations: 7738\ncpu: 90615.70715947275 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1196769.7858407302,
            "unit": "ns/iter",
            "extra": "iterations: 565\ncpu: 1196666.391150442 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17861554.4864857,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 17860576.75675675 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1581.0783709030968,
            "unit": "ns/iter",
            "extra": "iterations: 443264\ncpu: 1580.9866896477038 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15549.163411327272,
            "unit": "ns/iter",
            "extra": "iterations: 45003\ncpu: 15547.192987134204 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 161077.6494916845,
            "unit": "ns/iter",
            "extra": "iterations: 4328\ncpu: 161055.23059149724 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 179916.49600413765,
            "unit": "ns/iter",
            "extra": "iterations: 3879\ncpu: 179910.33513792214 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 141661.10787879338,
            "unit": "ns/iter",
            "extra": "iterations: 4950\ncpu: 141654.0353535352 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 87996.67988848308,
            "unit": "ns/iter",
            "extra": "iterations: 7891\ncpu: 87991.41097452797 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 63518354.99999777,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 63516069.22222213 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25740.833026509972,
            "unit": "ns/iter",
            "extra": "iterations: 27160\ncpu: 25739.4840942563 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 157061.5196942931,
            "unit": "ns/iter",
            "extra": "iterations: 4443\ncpu: 156990.89354043495 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d3f19c7f423f6688988ec12eeacc9d6ba0c97a19",
          "message": "B0.4.1 (#332)\n\n* adding Perf in Python\n\n* improve perf report\n\n* more cleaning\n\n* code cleaning and more perf",
          "timestamp": "2026-02-04T10:40:47+01:00",
          "tree_id": "d9eb6d9e775f3ff90f43177f5a20b280ee27bdde",
          "url": "https://github.com/najaeda/naja/commit/d3f19c7f423f6688988ec12eeacc9d6ba0c97a19"
        },
        "date": 1770198281121,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 19097.19460882723,
            "unit": "ns/iter",
            "extra": "iterations: 36838\ncpu: 19096.808350073294 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 86407.86739103428,
            "unit": "ns/iter",
            "extra": "iterations: 8099\ncpu: 86406.08704778367 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1177060.376884449,
            "unit": "ns/iter",
            "extra": "iterations: 597\ncpu: 1176717.8257956447 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17348367.400001053,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17348065.125 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.6198822804597,
            "unit": "ns/iter",
            "extra": "iterations: 476726\ncpu: 1468.302534369847 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14362.663774359018,
            "unit": "ns/iter",
            "extra": "iterations: 48750\ncpu: 14362.231405128203 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 201089.29062405825,
            "unit": "ns/iter",
            "extra": "iterations: 3317\ncpu: 201068.1872173649 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 219279.27737683602,
            "unit": "ns/iter",
            "extra": "iterations: 3187\ncpu: 219257.46062127396 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173504.27689243734,
            "unit": "ns/iter",
            "extra": "iterations: 4016\ncpu: 173496.00572709175 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 107779.1088864932,
            "unit": "ns/iter",
            "extra": "iterations: 6493\ncpu: 107777.83382103793 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64445178.6666688,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64437441.44444451 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24187.52037424257,
            "unit": "ns/iter",
            "extra": "iterations: 29179\ncpu: 24187.160594948393 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164079.6770354295,
            "unit": "ns/iter",
            "extra": "iterations: 4276\ncpu: 163948.77712814428 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a0e7d5abf641c36a5113c70ed84d8a7970bf184b",
          "message": "switch to macos15 (#334)",
          "timestamp": "2026-02-16T10:31:33+01:00",
          "tree_id": "89699ac1c589f8aed31825993fec1f5469fdf299",
          "url": "https://github.com/najaeda/naja/commit/a0e7d5abf641c36a5113c70ed84d8a7970bf184b"
        },
        "date": 1771234525739,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18515.019379540485,
            "unit": "ns/iter",
            "extra": "iterations: 38133\ncpu: 18514.30031731047 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 85364.48004380497,
            "unit": "ns/iter",
            "extra": "iterations: 8218\ncpu: 85338.76758335362 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1159542.8580858875,
            "unit": "ns/iter",
            "extra": "iterations: 606\ncpu: 1159113.646864686 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17405130.400000248,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17403848.624999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1487.8220124697664,
            "unit": "ns/iter",
            "extra": "iterations: 467853\ncpu: 1487.6972702964385 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14738.835010609571,
            "unit": "ns/iter",
            "extra": "iterations: 47597\ncpu: 14738.185179738226 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214037.9201228998,
            "unit": "ns/iter",
            "extra": "iterations: 3255\ncpu: 214020.0298003072 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 220366.59440778432,
            "unit": "ns/iter",
            "extra": "iterations: 3183\ncpu: 220356.5249764373 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173384.61336633505,
            "unit": "ns/iter",
            "extra": "iterations: 4040\ncpu: 173369.7873762378 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 108667.09035957481,
            "unit": "ns/iter",
            "extra": "iterations: 6452\ncpu: 108664.05207687533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64194998.444444686,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64190902.11111111 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24117.0175486983,
            "unit": "ns/iter",
            "extra": "iterations: 29005\ncpu: 24115.921186002386 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164370.1606847129,
            "unit": "ns/iter",
            "extra": "iterations: 4263\ncpu: 164198.25991084496 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "b35fdd957f286f8344ce678d803c8879ff451fbb",
          "message": "enable assign compression (#335)\n\n* enable assign compression\n\n* new tests\n\n* fix and coverage\n\n* local coverage and testing\n\n* more coverage\n\n* fix license and coverage\n\n* cleaning and coverage",
          "timestamp": "2026-02-18T13:45:26+01:00",
          "tree_id": "f2805ee397832ceec9a84825a81c11cb931ec83f",
          "url": "https://github.com/najaeda/naja/commit/b35fdd957f286f8344ce678d803c8879ff451fbb"
        },
        "date": 1771418971301,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 18593.474217884923,
            "unit": "ns/iter",
            "extra": "iterations: 36983\ncpu: 18593.179893464567 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 84811.07379627212,
            "unit": "ns/iter",
            "extra": "iterations: 8266\ncpu: 84808.20868618437 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1161422.9884298202,
            "unit": "ns/iter",
            "extra": "iterations: 605\ncpu: 1161346.575206611 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17742333.299999304,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17740953.049999997 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1472.0380684110173,
            "unit": "ns/iter",
            "extra": "iterations: 476064\ncpu: 1471.9952086274106 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14395.496461176166,
            "unit": "ns/iter",
            "extra": "iterations: 48745\ncpu: 14394.792963380854 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 201246.49953662435,
            "unit": "ns/iter",
            "extra": "iterations: 3237\ncpu: 200959.78220574607 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 221000.92552846478,
            "unit": "ns/iter",
            "extra": "iterations: 3075\ncpu: 220981.66666666666 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 174807.81608619308,
            "unit": "ns/iter",
            "extra": "iterations: 3991\ncpu: 174803.5968428966 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 109176.65595256288,
            "unit": "ns/iter",
            "extra": "iterations: 6409\ncpu: 109171.373693244 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65649534.222220354,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65647092.666666664 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23926.444742106738,
            "unit": "ns/iter",
            "extra": "iterations: 29489\ncpu: 23925.141917325123 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 164141.1350193136,
            "unit": "ns/iter",
            "extra": "iterations: 4229\ncpu: 163981.7569165468 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "899fc098cc513859e355789b05893c54a58ee153",
          "message": "better reporting and implicit AND support (#336)",
          "timestamp": "2026-02-19T23:33:26+01:00",
          "tree_id": "1b6b4df1d63e0db955f5d1db116ebc5c8a3e8c7e",
          "url": "https://github.com/najaeda/naja/commit/899fc098cc513859e355789b05893c54a58ee153"
        },
        "date": 1771540639086,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 20035.05308617691,
            "unit": "ns/iter",
            "extra": "iterations: 35659\ncpu: 20033.608598109873 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 93839.5617540532,
            "unit": "ns/iter",
            "extra": "iterations: 7457\ncpu: 93808.19350945418 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1141802.7829313318,
            "unit": "ns/iter",
            "extra": "iterations: 539\ncpu: 1141774.8812615955 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17749361.12820441,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17748064.102564104 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1582.7481232086343,
            "unit": "ns/iter",
            "extra": "iterations: 443443\ncpu: 1582.7015828415372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15548.415957328576,
            "unit": "ns/iter",
            "extra": "iterations: 44995\ncpu: 15543.606645182806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 163675.89484483982,
            "unit": "ns/iter",
            "extra": "iterations: 3899\ncpu: 163657.66709412658 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 179411.79350881002,
            "unit": "ns/iter",
            "extra": "iterations: 3913\ncpu: 179357.52900587796 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 141394.23410696033,
            "unit": "ns/iter",
            "extra": "iterations: 4955\ncpu: 141353.27951564087 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 88809.90794451057,
            "unit": "ns/iter",
            "extra": "iterations: 7930\ncpu: 88797.50264817147 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64540182.2222248,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64537502.999999985 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25717.328801261596,
            "unit": "ns/iter",
            "extra": "iterations: 27287\ncpu: 25715.202074247816 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 158022.35637160216,
            "unit": "ns/iter",
            "extra": "iterations: 4442\ncpu: 157910.16434039795 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8ae43f5ce018d863660064ea99759c8a246b565b",
          "message": "Slang testing (#333)\n\n* init sv support\n\n* more testing\n\n* bump slang version\n\n* some progress\n\n* Add FA (Full Adder) primitive to NLDB0\n\n- Ports: A, B, CI (inputs), S (sum), CO (carry-out)\n- Combinatorial arcs from all inputs to both outputs\n- getFASumTruthTable() -> SNLTruthTable(3, 0x96) [XOR-3]\n- getFACoutTruthTable() -> SNLTruthTable(3, 0xE8) [majority]\n- getPrimitiveTruthTable throws for FA (two outputs, use dedicated methods)\n- Unit tests in NLDB0Test and SNLGateTruthTablesTest\n\n* add source info and option for json post elaboration generation\n\n* Use FA\n\n* refactor sv test start action with simulation\n\n* bump slang version\n\n* add system verilog round testing through najaeda\n\n* add license\n\n* use external build for slang\n\n* macos-14 build\n\n* new attempt\n\n* new attempt\n\n* new version\n\n* revert changes on macos build\n\n* cleaning compile\n\n* -fPIC on slang build\n\n* removing macos-14 soon to be discontinued on github\n\n* new attempt\n\n* switch to najaeda fork and add PIC option in slang\n\n* update slang\n\n* new attempt\n\n* new attempt\n\n* update compile\n\n* add boost unordered to windows setup\n\n* new version\n\n* new attempt\n\n* restrain to windows\n\n* FMT clash\n\n* cleaning binary ops tests\n\n* more testing\n\n* error reporting and testing\n\n* cleaning gates support\n\n* NLDB0 cleaning\n\n* test wrong file when loading SV\n\n* coverage\n\n* more testing\n\n* more testing\n\n* coverage\n\n* improve testing and coverage\n\n* logging cleaning\n\n* coverage\n\n* more coverage\n\n* more testing\n\n* mux2 cover\n\n* bump slang version\n\n* license and coverage\n\n* verilator support",
          "timestamp": "2026-02-22T03:08:23+01:00",
          "tree_id": "505f65bf0bac3226f66007b2a185cde3a28b62aa",
          "url": "https://github.com/najaeda/naja/commit/8ae43f5ce018d863660064ea99759c8a246b565b"
        },
        "date": 1771726332313,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 29617.92011566306,
            "unit": "ns/iter",
            "extra": "iterations: 23171\ncpu: 29611.499417375166 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 97138.22367515446,
            "unit": "ns/iter",
            "extra": "iterations: 7265\ncpu: 97087.94439091533 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1194050.8406779773,
            "unit": "ns/iter",
            "extra": "iterations: 590\ncpu: 1193769.2898305086 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17656752.736842774,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17655370.342105262 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1456.5288526349516,
            "unit": "ns/iter",
            "extra": "iterations: 480684\ncpu: 1456.2430806933457 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14248.469869458191,
            "unit": "ns/iter",
            "extra": "iterations: 49103\ncpu: 14247.808158360984 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216242.2778497464,
            "unit": "ns/iter",
            "extra": "iterations: 3088\ncpu: 216221.41418393762 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240742.9858395256,
            "unit": "ns/iter",
            "extra": "iterations: 2966\ncpu: 240263.05461901543 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 189511.29261597272,
            "unit": "ns/iter",
            "extra": "iterations: 3643\ncpu: 189506.7452648918 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 123161.31846235291,
            "unit": "ns/iter",
            "extra": "iterations: 5671\ncpu: 123156.314582966 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64980475.44444161,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64977042.66666678 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24669.47512715116,
            "unit": "ns/iter",
            "extra": "iterations: 28706\ncpu: 24668.841775238612 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 180716.75468378956,
            "unit": "ns/iter",
            "extra": "iterations: 3897\ncpu: 180511.5563253975 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a5a75781b287649f65b3046b93137f9b60147fd0",
          "message": "supporting unquoted enable (#338)",
          "timestamp": "2026-02-25T12:46:33+01:00",
          "tree_id": "590e167d756d66180bfb5dccea56ca630bb00e0a",
          "url": "https://github.com/najaeda/naja/commit/a5a75781b287649f65b3046b93137f9b60147fd0"
        },
        "date": 1772020220679,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 29695.365662192795,
            "unit": "ns/iter",
            "extra": "iterations: 23694\ncpu: 29686.573225289103 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 97585.09275201506,
            "unit": "ns/iter",
            "extra": "iterations: 7202\ncpu: 97557.70410996948 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1182086.777777809,
            "unit": "ns/iter",
            "extra": "iterations: 594\ncpu: 1182043.0538720544 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16845690.634146728,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16845457.68292683 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1469.8569612986082,
            "unit": "ns/iter",
            "extra": "iterations: 480758\ncpu: 1469.8083880039437 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14273.71281811316,
            "unit": "ns/iter",
            "extra": "iterations: 49157\ncpu: 14271.412738775747 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202451.03521537434,
            "unit": "ns/iter",
            "extra": "iterations: 3436\ncpu: 202081.6094295694 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 239322.64376067658,
            "unit": "ns/iter",
            "extra": "iterations: 2925\ncpu: 239290.83487179512 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 191912.30953041618,
            "unit": "ns/iter",
            "extra": "iterations: 3599\ncpu: 191900.8560711308 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 126140.06021272934,
            "unit": "ns/iter",
            "extra": "iterations: 5547\ncpu: 126132.56751397137 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64324226.22222272,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64320422.66666668 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24234.204430424437,
            "unit": "ns/iter",
            "extra": "iterations: 28846\ncpu: 24232.977189211688 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 178906.85725127807,
            "unit": "ns/iter",
            "extra": "iterations: 3930\ncpu: 178835.83460559955 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2a5ef178dcb6fc015f5c17d35c87817e7a9abab3",
          "message": "Sv integration (#337)\n\n* new options better diagnose\n\n* more support\n\n* support packed arrays\n\n* new operator\n\n* add shifter support\n\n* adding dffn/dffr\n\n* new feature\n\n* more support\n\n* support +\n\n* more support\n\n* more support\n\n* bump slang and add perf in SV loading\n\n* adding coverage\n\n* adding in naja_edit\n\n* coverage\n\n* coverage\n\n* refactor code\n\n* coverage\n\n* more coverage\n\n* more coverage\n\n* coverage\n\n* coverage\n\n* cover\n\n* missing license\n\n* more coverage\n\n* coverage\n\n* more coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-02-27T15:12:02+01:00",
          "tree_id": "439bce19fee3e0d2d1f0244df86627af391f0cd5",
          "url": "https://github.com/najaeda/naja/commit/2a5ef178dcb6fc015f5c17d35c87817e7a9abab3"
        },
        "date": 1772201761761,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 33716.915417550066,
            "unit": "ns/iter",
            "extra": "iterations: 20548\ncpu: 33714.598306404514 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 102020.96043377697,
            "unit": "ns/iter",
            "extra": "iterations: 6824\ncpu: 102013.85697538102 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1216171.4473225814,
            "unit": "ns/iter",
            "extra": "iterations: 579\ncpu: 1216165.164075993 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17493149.65853755,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17476866.682926834 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1465.7514932667325,
            "unit": "ns/iter",
            "extra": "iterations: 478816\ncpu: 1465.6716776381745 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14618.01425247287,
            "unit": "ns/iter",
            "extra": "iterations: 48132\ncpu: 14615.309565361918 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 226571.81494212046,
            "unit": "ns/iter",
            "extra": "iterations: 3199\ncpu: 226572.30415754905 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242927.40561131292,
            "unit": "ns/iter",
            "extra": "iterations: 2887\ncpu: 242892.3134741946 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198284.24781759086,
            "unit": "ns/iter",
            "extra": "iterations: 3551\ncpu: 198247.32131793836 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 131329.09996253782,
            "unit": "ns/iter",
            "extra": "iterations: 5342\ncpu: 131325.7023586673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64201362.444439135,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64199810.00000005 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24956.067589135262,
            "unit": "ns/iter",
            "extra": "iterations: 28185\ncpu: 24955.693312045434 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 184586.78272492843,
            "unit": "ns/iter",
            "extra": "iterations: 3797\ncpu: 184413.33737155198 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e854ad98634b4caa35aa36552de1e38ee17f5f6a",
          "message": "Liberty: bus direction coming from child pins (#339)\n\n* add test and support\n\n* new testing",
          "timestamp": "2026-03-04T14:50:20+01:00",
          "tree_id": "dabb346e4feea145071bbce56356661f8cf98587",
          "url": "https://github.com/najaeda/naja/commit/e854ad98634b4caa35aa36552de1e38ee17f5f6a"
        },
        "date": 1772632460345,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 33128.91581108801,
            "unit": "ns/iter",
            "extra": "iterations: 20454\ncpu: 33128.401290701084 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104182.29715223095,
            "unit": "ns/iter",
            "extra": "iterations: 6707\ncpu: 104182.12121663935 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1238482.2353982423,
            "unit": "ns/iter",
            "extra": "iterations: 565\ncpu: 1238464.8371681415 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17702619.52499936,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17701818.94999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1463.1731292672755,
            "unit": "ns/iter",
            "extra": "iterations: 478342\ncpu: 1463.1851081443815 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14353.310974110693,
            "unit": "ns/iter",
            "extra": "iterations: 48824\ncpu: 14352.305177781427 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 213430.29827914605,
            "unit": "ns/iter",
            "extra": "iterations: 3138\ncpu: 213424.3467176547 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240134.63602057987,
            "unit": "ns/iter",
            "extra": "iterations: 2915\ncpu: 240111.22504288162 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 194155.90596394203,
            "unit": "ns/iter",
            "extra": "iterations: 3605\ncpu: 194127.23495145648 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 127218.21649109249,
            "unit": "ns/iter",
            "extra": "iterations: 5506\ncpu: 127213.55793679625 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65353155.88889211,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65350555.55555541 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25069.068234455986,
            "unit": "ns/iter",
            "extra": "iterations: 28065\ncpu: 25067.314626759293 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 186720.6383652408,
            "unit": "ns/iter",
            "extra": "iterations: 3769\ncpu: 186195.49854073676 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "20f660677b572158bf3d3f8019ce5e356e3b94b7",
          "message": "B0.5.1 (#340)",
          "timestamp": "2026-03-18T02:19:44+01:00",
          "tree_id": "e315345a6e1169989d0b15b842f5a8acf1220aa1",
          "url": "https://github.com/najaeda/naja/commit/20f660677b572158bf3d3f8019ce5e356e3b94b7"
        },
        "date": 1773797020287,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 47011.23020527768,
            "unit": "ns/iter",
            "extra": "iterations: 15004\ncpu: 47010.232071447615 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111556.99012738948,
            "unit": "ns/iter",
            "extra": "iterations: 6280\ncpu: 111555.81289808918 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1380619.618577103,
            "unit": "ns/iter",
            "extra": "iterations: 506\ncpu: 1380596.458498024 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17109668.048780207,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17109090.46341463 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1478.5915502172597,
            "unit": "ns/iter",
            "extra": "iterations: 473811\ncpu: 1478.5346351182227 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14484.685114385198,
            "unit": "ns/iter",
            "extra": "iterations: 48389\ncpu: 14484.014734753759 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 197441.7341772117,
            "unit": "ns/iter",
            "extra": "iterations: 3555\ncpu: 197436.72573839672 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259742.28518380917,
            "unit": "ns/iter",
            "extra": "iterations: 2693\ncpu: 259736.1318232455 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214770.29199999938,
            "unit": "ns/iter",
            "extra": "iterations: 3250\ncpu: 214764.97138461535 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 149462.37033107126,
            "unit": "ns/iter",
            "extra": "iterations: 4712\ncpu: 149461.12818336152 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66328702.00000223,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66326279.888888866 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24522.334716506615,
            "unit": "ns/iter",
            "extra": "iterations: 28678\ncpu: 24522.336111304878 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 202676.45444182435,
            "unit": "ns/iter",
            "extra": "iterations: 3468\ncpu: 202537.42070361238 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "a7792f81f69fe0c3486c30a7c45d18b45b146d5d",
          "message": "rework naja-regress (#341)\n\n* rework naja-regress\n\n* code cleaning and NajaEdit cleaning",
          "timestamp": "2026-03-18T19:56:08+01:00",
          "tree_id": "b3786ace26ec3d2579afef2ce5b43c30ba03f6cd",
          "url": "https://github.com/najaeda/naja/commit/a7792f81f69fe0c3486c30a7c45d18b45b146d5d"
        },
        "date": 1773860419672,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 42035.89472745626,
            "unit": "ns/iter",
            "extra": "iterations: 16823\ncpu: 42035.67265053796 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 109645.42013942942,
            "unit": "ns/iter",
            "extra": "iterations: 6455\ncpu: 109644.66893880712 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1194932.0856164242,
            "unit": "ns/iter",
            "extra": "iterations: 584\ncpu: 1194670.7619863017 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 15596391.400000207,
            "unit": "ns/iter",
            "extra": "iterations: 45\ncpu: 15593960.822222227 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.9466178200112,
            "unit": "ns/iter",
            "extra": "iterations: 568036\ncpu: 1223.8970839876342 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12565.248338736694,
            "unit": "ns/iter",
            "extra": "iterations: 55831\ncpu: 12562.851587827561 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130610.87149704975,
            "unit": "ns/iter",
            "extra": "iterations: 5424\ncpu: 130607.35232300874 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 213446.53665241797,
            "unit": "ns/iter",
            "extra": "iterations: 3274\ncpu: 213442.31734880898 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 173352.19552238702,
            "unit": "ns/iter",
            "extra": "iterations: 4020\ncpu: 173352.19701492533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 112396.62826889448,
            "unit": "ns/iter",
            "extra": "iterations: 6233\ncpu: 112390.64784213077 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 61877357.555557765,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 61873181.333333224 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24367.81547660756,
            "unit": "ns/iter",
            "extra": "iterations: 28598\ncpu: 24366.37464158337 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 131792.53279303218,
            "unit": "ns/iter",
            "extra": "iterations: 5306\ncpu: 131755.20523936985 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "f2aa8ad570e32ebdd74eff9d4e0991e59c0e25d2",
          "message": "new options in dump verilog to dump or not RTL informations (#342)\n\n* new options in dump verilog to dump or not RTL informations\n\n* add TT for not\n\n* new attempt",
          "timestamp": "2026-03-19T23:27:43+01:00",
          "tree_id": "023e03fc938fe4bf91e1e55edba3e1ca89f224ca",
          "url": "https://github.com/najaeda/naja/commit/f2aa8ad570e32ebdd74eff9d4e0991e59c0e25d2"
        },
        "date": 1773959503020,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45086.47846233981,
            "unit": "ns/iter",
            "extra": "iterations: 15322\ncpu: 45075.02617151809 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111023.18269841195,
            "unit": "ns/iter",
            "extra": "iterations: 6300\ncpu: 111017.81555555557 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1372692.18590999,
            "unit": "ns/iter",
            "extra": "iterations: 511\ncpu: 1372699.3189823874 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16859460.333333366,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16856231.64285714 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1468.2057840924945,
            "unit": "ns/iter",
            "extra": "iterations: 475926\ncpu: 1467.9701403159306 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14391.386186050277,
            "unit": "ns/iter",
            "extra": "iterations: 48632\ncpu: 14388.161580852111 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 195689.1290420993,
            "unit": "ns/iter",
            "extra": "iterations: 3278\ncpu: 195681.0018303846 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 260730.32692306678,
            "unit": "ns/iter",
            "extra": "iterations: 2704\ncpu: 260725.3820266274 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212157.24012158453,
            "unit": "ns/iter",
            "extra": "iterations: 3290\ncpu: 212158.05866261417 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 146678.7676810141,
            "unit": "ns/iter",
            "extra": "iterations: 4765\ncpu: 146679.40692549845 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66167727.44444512,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66168176.333333254 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25001.9850346458,
            "unit": "ns/iter",
            "extra": "iterations: 27998\ncpu: 25001.77534109579 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 199957.7595885323,
            "unit": "ns/iter",
            "extra": "iterations: 3494\ncpu: 199842.57441327267 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "19c944a2abc9303c7c8d07cfc976ec4536af51d4",
          "message": "codecov run cleaning (#343)\n\n* codecov run cleaning\n\n* update naja-verilog\n\n* update slang and add multiple combinations for compiling on ubuntu\n\n* new testing\n\n* update coverage\n\n* update coverage\n\n* switch to gcovr\n\n* cleaning\n\n* cleaning coverage\n\n* new attempt\n\n* new setting\n\n* update config file",
          "timestamp": "2026-03-20T22:55:10+01:00",
          "tree_id": "61ad335a9001f3ea60b84726d431f769aff1fa0e",
          "url": "https://github.com/najaeda/naja/commit/19c944a2abc9303c7c8d07cfc976ec4536af51d4"
        },
        "date": 1774043957102,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 44583.68034357775,
            "unit": "ns/iter",
            "extra": "iterations: 15135\ncpu: 44582.18566237199 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 110734.06931476707,
            "unit": "ns/iter",
            "extra": "iterations: 6319\ncpu: 110727.66719417632 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1368632.5957031897,
            "unit": "ns/iter",
            "extra": "iterations: 512\ncpu: 1368519.6093750005 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16866902.43902221,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16864972.780487806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1499.7689109399935,
            "unit": "ns/iter",
            "extra": "iterations: 466595\ncpu: 1499.700843343799 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14705.079072980752,
            "unit": "ns/iter",
            "extra": "iterations: 47766\ncpu: 14704.538877025512 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 210032.77391038724,
            "unit": "ns/iter",
            "extra": "iterations: 3304\ncpu: 210025.41918886188 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 258335.18620942437,
            "unit": "ns/iter",
            "extra": "iterations: 2712\ncpu: 258331.06637168172 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212264.46940642333,
            "unit": "ns/iter",
            "extra": "iterations: 3285\ncpu: 212256.68371385074 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147723.91621052427,
            "unit": "ns/iter",
            "extra": "iterations: 4750\ncpu: 147720.8160000002 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65804122.44444586,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65803576.11111115 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24901.400064029913,
            "unit": "ns/iter",
            "extra": "iterations: 28113\ncpu: 24901.106569914256 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 200449.344748138,
            "unit": "ns/iter",
            "extra": "iterations: 3504\ncpu: 200298.0479451981 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "800693539f4283942f0c968c0fbf84220475dfe1",
          "message": "install gcovr with python",
          "timestamp": "2026-03-20T23:46:45+01:00",
          "tree_id": "d386ca34a08a02390dda6cfa9ffc8ed56ff72051",
          "url": "https://github.com/najaeda/naja/commit/800693539f4283942f0c968c0fbf84220475dfe1"
        },
        "date": 1774047063081,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45587.29832373064,
            "unit": "ns/iter",
            "extra": "iterations: 15272\ncpu: 45577.32968831849 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 110145.69055476386,
            "unit": "ns/iter",
            "extra": "iterations: 6363\ncpu: 110114.67122426529 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1389604.2848723114,
            "unit": "ns/iter",
            "extra": "iterations: 509\ncpu: 1389541.4597249515 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16975446.58536738,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16974871.121951208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.072711736044,
            "unit": "ns/iter",
            "extra": "iterations: 466926\ncpu: 1500.0475107404588 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14687.258514550567,
            "unit": "ns/iter",
            "extra": "iterations: 47595\ncpu: 14686.780901355181 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211505.46427592429,
            "unit": "ns/iter",
            "extra": "iterations: 3653\ncpu: 211501.9170544758 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 254186.7971543268,
            "unit": "ns/iter",
            "extra": "iterations: 2741\ncpu: 254166.8139365193 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 205851.1311378794,
            "unit": "ns/iter",
            "extra": "iterations: 3401\ncpu: 205846.30667450753 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140725.37928270333,
            "unit": "ns/iter",
            "extra": "iterations: 4991\ncpu: 140718.62111801255 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64248681.111103505,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64246348.444444366 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 28999.85624419352,
            "unit": "ns/iter",
            "extra": "iterations: 27978\ncpu: 28997.89252269641 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 199697.03077670682,
            "unit": "ns/iter",
            "extra": "iterations: 3509\ncpu: 199537.16699911805 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "3698bfe3c599ce9bbea43555e63aada4faa1a9c2",
          "message": "cleaning codecov",
          "timestamp": "2026-03-21T00:18:22+01:00",
          "tree_id": "7f2ab41fe4ec480e7d84651f8db9f8f0e6a87678",
          "url": "https://github.com/najaeda/naja/commit/3698bfe3c599ce9bbea43555e63aada4faa1a9c2"
        },
        "date": 1774049022879,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45532.62520918227,
            "unit": "ns/iter",
            "extra": "iterations: 14939\ncpu: 45530.71095789544 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111605.43148502665,
            "unit": "ns/iter",
            "extra": "iterations: 6276\ncpu: 111598.44534735504 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1389998.4356434701,
            "unit": "ns/iter",
            "extra": "iterations: 505\ncpu: 1389932.544554455 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17412551.29268239,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17410486.902439017 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1520.4231368148544,
            "unit": "ns/iter",
            "extra": "iterations: 455309\ncpu: 1520.336727365372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15006.046021237156,
            "unit": "ns/iter",
            "extra": "iterations: 46522\ncpu: 15005.855294269393 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 215190.9780117546,
            "unit": "ns/iter",
            "extra": "iterations: 3229\ncpu: 215183.70672034682 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259934.25565863226,
            "unit": "ns/iter",
            "extra": "iterations: 2695\ncpu: 259924.7833024117 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 215319.4201550409,
            "unit": "ns/iter",
            "extra": "iterations: 3225\ncpu: 215309.0226356589 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 148302.43888769497,
            "unit": "ns/iter",
            "extra": "iterations: 4639\ncpu: 148298.0993748652 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 68860626.55555254,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 68854462.0000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25076.5538119813,
            "unit": "ns/iter",
            "extra": "iterations: 27912\ncpu: 25075.02246345655 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 201541.9734945776,
            "unit": "ns/iter",
            "extra": "iterations: 3471\ncpu: 201392.82886774727 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "committer": {
            "email": "christophe.alex@gmail.com",
            "name": "xtof",
            "username": "xtofalex"
          },
          "distinct": true,
          "id": "c4a6dcba1181705bfaa77db67dbb5868de83d5c8",
          "message": "coverage cleaning",
          "timestamp": "2026-03-21T00:39:45+01:00",
          "tree_id": "e27c7f34044ca06bd46141a56f9d7c06657ceccc",
          "url": "https://github.com/najaeda/naja/commit/c4a6dcba1181705bfaa77db67dbb5868de83d5c8"
        },
        "date": 1774050929139,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45868.69555174392,
            "unit": "ns/iter",
            "extra": "iterations: 15152\ncpu: 45867.74346620908 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 112891.80495654982,
            "unit": "ns/iter",
            "extra": "iterations: 6214\ncpu: 112889.97682652078 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1386265.5821781973,
            "unit": "ns/iter",
            "extra": "iterations: 505\ncpu: 1386257.8277227723 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17158176.536584992,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17145730.85365854 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.56106993854,
            "unit": "ns/iter",
            "extra": "iterations: 466195\ncpu: 1500.537279464601 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14716.404788384298,
            "unit": "ns/iter",
            "extra": "iterations: 47657\ncpu: 14715.951885347391 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 191617.12020810263,
            "unit": "ns/iter",
            "extra": "iterations: 3652\ncpu: 191605.16100766708 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 255703.71794872987,
            "unit": "ns/iter",
            "extra": "iterations: 2730\ncpu: 255665.98095238107 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 208793.27784396973,
            "unit": "ns/iter",
            "extra": "iterations: 3358\ncpu: 208781.41751042297 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141076.1746513085,
            "unit": "ns/iter",
            "extra": "iterations: 4947\ncpu: 141071.2862340815 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64794996.999997534,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64792479.77777764 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25137.746455794153,
            "unit": "ns/iter",
            "extra": "iterations: 28074\ncpu: 25136.652026786363 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 202400.7610267179,
            "unit": "ns/iter",
            "extra": "iterations: 3469\ncpu: 202292.85471323095 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d529aacf231568235d88843e76688b851ebaa66b",
          "message": "new exclusion pattern (#344)",
          "timestamp": "2026-03-21T15:32:35+01:00",
          "tree_id": "5b678e9c2ca8a4140e9dcb1c31bb31cc8d359329",
          "url": "https://github.com/najaeda/naja/commit/d529aacf231568235d88843e76688b851ebaa66b"
        },
        "date": 1774104429273,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 44787.404220983306,
            "unit": "ns/iter",
            "extra": "iterations: 15494\ncpu: 44782.42138892475 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111047.25603558657,
            "unit": "ns/iter",
            "extra": "iterations: 6296\ncpu: 111036.36308767472 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1380311.0389863914,
            "unit": "ns/iter",
            "extra": "iterations: 513\ncpu: 1380156.3859649121 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17563360.90243888,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17562635.95121951 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1502.1184481490002,
            "unit": "ns/iter",
            "extra": "iterations: 465689\ncpu: 1502.0517448339995 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14701.756726056665,
            "unit": "ns/iter",
            "extra": "iterations: 47539\ncpu: 14700.537537600714 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 196122.0342172718,
            "unit": "ns/iter",
            "extra": "iterations: 3507\ncpu: 196110.6395779869 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 259332.44903986677,
            "unit": "ns/iter",
            "extra": "iterations: 2708\ncpu: 259300.58197932053 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214531.6373928954,
            "unit": "ns/iter",
            "extra": "iterations: 3268\ncpu: 214517.41554467572 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 148340.7327130421,
            "unit": "ns/iter",
            "extra": "iterations: 4729\ncpu: 148307.8012264748 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65942314.11111196,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65938498.777777754 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25014.67253470666,
            "unit": "ns/iter",
            "extra": "iterations: 27948\ncpu: 25012.171854873297 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203018.83583350392,
            "unit": "ns/iter",
            "extra": "iterations: 3466\ncpu: 202782.63906521592 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "0d8fe466ac566208680f5f5d1692a3e346ead691",
          "message": "add blackbox detection in SV constructor (#346)\n\n* add blackbox detection in SV constructor\n\n* clean coverage",
          "timestamp": "2026-03-23T13:20:50+01:00",
          "tree_id": "54db2c9e9add7d991d062f3371b5a0b4fbe8d3d4",
          "url": "https://github.com/najaeda/naja/commit/0d8fe466ac566208680f5f5d1692a3e346ead691"
        },
        "date": 1774268702261,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 45790.252927401474,
            "unit": "ns/iter",
            "extra": "iterations: 15372\ncpu: 45788.19607077804 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 111538.33343948874,
            "unit": "ns/iter",
            "extra": "iterations: 6280\ncpu: 111526.52261146496 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1384445.0670611542,
            "unit": "ns/iter",
            "extra": "iterations: 507\ncpu: 1384325.4654832345 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16893214.73170727,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 16892544.34146342 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1501.4966825438034,
            "unit": "ns/iter",
            "extra": "iterations: 465869\ncpu: 1501.478924332806 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14674.485384163789,
            "unit": "ns/iter",
            "extra": "iterations: 47688\ncpu: 14673.414443885264 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 214230.68874723554,
            "unit": "ns/iter",
            "extra": "iterations: 3608\ncpu: 214226.51773835922 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 261973.19042232854,
            "unit": "ns/iter",
            "extra": "iterations: 2652\ncpu: 261954.2209653091 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214518.8690184062,
            "unit": "ns/iter",
            "extra": "iterations: 3260\ncpu: 214502.80276073635 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147512.1657551508,
            "unit": "ns/iter",
            "extra": "iterations: 4754\ncpu: 147508.0925536389 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 66342689.22222318,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 66342361.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25203.976826268146,
            "unit": "ns/iter",
            "extra": "iterations: 28049\ncpu: 25203.592534493222 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203367.86316355743,
            "unit": "ns/iter",
            "extra": "iterations: 3464\ncpu: 203257.74480368927 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "24e307110d1e4cad5b9eaea86105c772560aab8a",
          "message": "Rtl memory inferring (#345)\n\n* First steps\n\n* naja_mem in progress\n\n* bused mux2\n\n* clean coverage\n\n* perf coverage\n\n* save current state\n\n* cleaning and coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* clean coverage\n\n* NLDB0 optims and SV Constructor coverage\n\n* code cleaning\n\n* coverage\n\n* cleaning and coverage\n\n* cleaning\n\n* coverage\n\n* cleaning and coverage",
          "timestamp": "2026-03-30T18:26:24+02:00",
          "tree_id": "51371b56505f4e88f5a0aa26a1cababfb4f9282c",
          "url": "https://github.com/najaeda/naja/commit/24e307110d1e4cad5b9eaea86105c772560aab8a"
        },
        "date": 1774888218582,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 36322.87213750092,
            "unit": "ns/iter",
            "extra": "iterations: 19607\ncpu: 36316.99535880042 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104369.02310674099,
            "unit": "ns/iter",
            "extra": "iterations: 6708\ncpu: 104369.25193798449 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1190567.7741936056,
            "unit": "ns/iter",
            "extra": "iterations: 589\ncpu: 1190550.4550084886 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16092817.51162799,
            "unit": "ns/iter",
            "extra": "iterations: 43\ncpu: 16092845.255813956 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1222.0546768178767,
            "unit": "ns/iter",
            "extra": "iterations: 574119\ncpu: 1221.8165606781865 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12602.754064784727,
            "unit": "ns/iter",
            "extra": "iterations: 55661\ncpu: 12602.66790032518 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 129368.35363128356,
            "unit": "ns/iter",
            "extra": "iterations: 5370\ncpu: 129367.99664804473 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 217836.75116640108,
            "unit": "ns/iter",
            "extra": "iterations: 3215\ncpu: 217836.70451010886 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 176596.57836198705,
            "unit": "ns/iter",
            "extra": "iterations: 3956\ncpu: 176595.71941354877 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 115992.87566225213,
            "unit": "ns/iter",
            "extra": "iterations: 6040\ncpu: 115985.15993377482 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 62022773.19999894,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 62020734.500000075 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24242.143674370225,
            "unit": "ns/iter",
            "extra": "iterations: 27270\ncpu: 24242.002603593734 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 126900.63530430071,
            "unit": "ns/iter",
            "extra": "iterations: 5495\ncpu: 126662.8687898404 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "7b13cb0aeff525802fd22e30af6f00a5db18c964",
          "message": "towards v0.5.3 (#347)\n\n* towards v0.5.3\n\n* fix typo",
          "timestamp": "2026-03-31T15:46:56+02:00",
          "tree_id": "997bf35c560e676044c183483efc7f0f43ea9143",
          "url": "https://github.com/najaeda/naja/commit/7b13cb0aeff525802fd22e30af6f00a5db18c964"
        },
        "date": 1774965049494,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 40037.202550007656,
            "unit": "ns/iter",
            "extra": "iterations: 16549\ncpu: 40033.55018430117 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 105488.10514372183,
            "unit": "ns/iter",
            "extra": "iterations: 6610\ncpu: 105466.93933434189 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1218926.085763254,
            "unit": "ns/iter",
            "extra": "iterations: 583\ncpu: 1218808.6981132072 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16764001.404762246,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16763639.571428569 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1460.9071504735753,
            "unit": "ns/iter",
            "extra": "iterations: 479227\ncpu: 1460.83908252248 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14372.228050356924,
            "unit": "ns/iter",
            "extra": "iterations: 48691\ncpu: 14371.376024316614 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 210896.08686502976,
            "unit": "ns/iter",
            "extra": "iterations: 3327\ncpu: 210890.98707544323 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 256916.15522938356,
            "unit": "ns/iter",
            "extra": "iterations: 2725\ncpu: 256893.2873394495 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 212463.25553870236,
            "unit": "ns/iter",
            "extra": "iterations: 3295\ncpu: 212446.2321699542 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144349.192339894,
            "unit": "ns/iter",
            "extra": "iterations: 4778\ncpu: 144339.00439514418 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64392694.88888992,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64391494.222222194 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24217.05179351843,
            "unit": "ns/iter",
            "extra": "iterations: 28826\ncpu: 24216.003920072144 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 194916.91394955898,
            "unit": "ns/iter",
            "extra": "iterations: 3591\ncpu: 194758.53689778486 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "christophe.alexandre@keplertech.io",
            "name": "Christophe Alexandre",
            "username": "xtofalex"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "c74285ebf18152a4aaa320472533d0330452bfc0",
          "message": "B0.5.4 (#348)\n\n* Fix Perf on macos\n\n* Fix bloating of memory gen\n\n* code coverage exclusion",
          "timestamp": "2026-04-01T20:38:54+02:00",
          "tree_id": "611fb8f8289acde572e40ca9823efc3051f69635",
          "url": "https://github.com/najaeda/naja/commit/c74285ebf18152a4aaa320472533d0330452bfc0"
        },
        "date": 1775068999986,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 38755.89421515604,
            "unit": "ns/iter",
            "extra": "iterations: 17857\ncpu: 38748.82040656325 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 104897.89580341581,
            "unit": "ns/iter",
            "extra": "iterations: 6267\ncpu: 104895.99664911442 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1207937.3510272575,
            "unit": "ns/iter",
            "extra": "iterations: 584\ncpu: 1207834.8116438356 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18067087.00000096,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18062063.743589748 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1461.4701464489362,
            "unit": "ns/iter",
            "extra": "iterations: 478938\ncpu: 1460.9278466106261 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14451.243155070455,
            "unit": "ns/iter",
            "extra": "iterations: 48430\ncpu: 14449.440119760467 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208869.44707272583,
            "unit": "ns/iter",
            "extra": "iterations: 3382\ncpu: 208852.98196333533 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 257730.09793816667,
            "unit": "ns/iter",
            "extra": "iterations: 2716\ncpu: 257657.30081001492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 211385.5128282632,
            "unit": "ns/iter",
            "extra": "iterations: 3313\ncpu: 211361.311500151 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 143372.12454062395,
            "unit": "ns/iter",
            "extra": "iterations: 4898\ncpu: 143355.3983258475 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65536782.1111036,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65535100.66666668 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24461.21185545173,
            "unit": "ns/iter",
            "extra": "iterations: 28392\ncpu: 24458.732142857112 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 196663.227347664,
            "unit": "ns/iter",
            "extra": "iterations: 3554\ncpu: 196523.61142375128 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}