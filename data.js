window.BENCHMARK_DATA = {
  "lastUpdate": 1781342187377,
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
          "id": "348eb18cb7dd4410ccdb483bdec35b98f70fea22",
          "message": "switching to 0.5.4",
          "timestamp": "2026-04-02T18:23:33+02:00",
          "tree_id": "3950721ba3fa858fd760c1ce0141da3eea95742f",
          "url": "https://github.com/najaeda/naja/commit/348eb18cb7dd4410ccdb483bdec35b98f70fea22"
        },
        "date": 1775147258424,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 40247.19890299647,
            "unit": "ns/iter",
            "extra": "iterations: 17320\ncpu: 40246.05848729792 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 106637.47166362408,
            "unit": "ns/iter",
            "extra": "iterations: 6564\ncpu: 106636.28915295552 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1215708.2055268008,
            "unit": "ns/iter",
            "extra": "iterations: 579\ncpu: 1215695.2590673578 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 16745568.476188682,
            "unit": "ns/iter",
            "extra": "iterations: 42\ncpu: 16745405.309523815 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1478.0924994729971,
            "unit": "ns/iter",
            "extra": "iterations: 474500\ncpu: 1478.0932623814542 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14832.517647798688,
            "unit": "ns/iter",
            "extra": "iterations: 47683\ncpu: 14832.201539332671 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 200722.91344490295,
            "unit": "ns/iter",
            "extra": "iterations: 3466\ncpu: 200717.41431044438 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 254671.4227967837,
            "unit": "ns/iter",
            "extra": "iterations: 2746\ncpu: 254665.4067734887 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 209664.57342450527,
            "unit": "ns/iter",
            "extra": "iterations: 3364\ncpu: 209654.99227110576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144328.2519021188,
            "unit": "ns/iter",
            "extra": "iterations: 4863\ncpu: 144320.2687641373 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 64388394.33332709,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 64386375.888888925 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24466.045689357517,
            "unit": "ns/iter",
            "extra": "iterations: 28650\ncpu: 24465.17815008723 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 194675.98718288424,
            "unit": "ns/iter",
            "extra": "iterations: 3589\ncpu: 194569.18612428056 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5136b9f699b7cb3caedefe6678a19603f9204e35",
          "message": "work towards full RTL support + fix in tt",
          "timestamp": "2026-04-07T01:25:37+02:00",
          "tree_id": "5d1b72bb79599ceb48606b217b0b923ca9b8d5ee",
          "url": "https://github.com/najaeda/naja/commit/5136b9f699b7cb3caedefe6678a19603f9204e35"
        },
        "date": 1775518189467,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 43198.07769393366,
            "unit": "ns/iter",
            "extra": "iterations: 16964\ncpu: 43117.81684744164 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 107746.74016355618,
            "unit": "ns/iter",
            "extra": "iterations: 6481\ncpu: 107744.13655300111 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1220449.9825784916,
            "unit": "ns/iter",
            "extra": "iterations: 574\ncpu: 1220409.0087108011 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17098834.926830534,
            "unit": "ns/iter",
            "extra": "iterations: 41\ncpu: 17098818.000000007 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1453.7031523116957,
            "unit": "ns/iter",
            "extra": "iterations: 481139\ncpu: 1453.6253078632162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14260.98192219108,
            "unit": "ns/iter",
            "extra": "iterations: 49121\ncpu: 14260.650902872494 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 195732.60267859357,
            "unit": "ns/iter",
            "extra": "iterations: 3584\ncpu: 195726.82979910713 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 261190.6969810201,
            "unit": "ns/iter",
            "extra": "iterations: 2683\ncpu: 261172.6243011556 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 214152.62854528063,
            "unit": "ns/iter",
            "extra": "iterations: 3279\ncpu: 214140.62275083835 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147012.7036493063,
            "unit": "ns/iter",
            "extra": "iterations: 4768\ncpu: 147005.10528523487 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65311943.88888808,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65311143.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24719.305014121266,
            "unit": "ns/iter",
            "extra": "iterations: 29038\ncpu: 24718.614539568847 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 203924.4918959616,
            "unit": "ns/iter",
            "extra": "iterations: 3456\ncpu: 203365.58478010356 ns\nthreads: 1"
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
          "id": "319865a0f291ad7ad501fd8e77767700ae5168c1",
          "message": "B0.5.5 (#350)\n\n* coverage process cleaning\n\n* switch back to lcov\n\n* add expected failing test\n\n* extend always support in SV\n\n* coverage settings\n\n* update coverage",
          "timestamp": "2026-04-07T16:46:42+02:00",
          "tree_id": "f23f7e94a3390ac12a39f9f39caa496451c06288",
          "url": "https://github.com/najaeda/naja/commit/319865a0f291ad7ad501fd8e77767700ae5168c1"
        },
        "date": 1775573475905,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 41597.39720630353,
            "unit": "ns/iter",
            "extra": "iterations: 16752\ncpu: 41597.725465616044 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 108071.52694332272,
            "unit": "ns/iter",
            "extra": "iterations: 6458\ncpu: 108016.80984825021 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1227854.4624783588,
            "unit": "ns/iter",
            "extra": "iterations: 573\ncpu: 1227531.8603839446 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19080234.00000201,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 19073199.62162162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1451.92949587462,
            "unit": "ns/iter",
            "extra": "iterations: 482142\ncpu: 1451.7891285140063 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14291.79660499306,
            "unit": "ns/iter",
            "extra": "iterations: 49072\ncpu: 14291.426291979118 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 213106.66860998038,
            "unit": "ns/iter",
            "extra": "iterations: 3259\ncpu: 213102.90702669523 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 257074.11378385185,
            "unit": "ns/iter",
            "extra": "iterations: 2619\ncpu: 257049.41771668577 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 210888.30207393607,
            "unit": "ns/iter",
            "extra": "iterations: 3327\ncpu: 210866.43552750233 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 144785.8052620822,
            "unit": "ns/iter",
            "extra": "iterations: 4827\ncpu: 144786.91278226636 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 65341307.111111745,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 65336704.11111123 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24020.85549152656,
            "unit": "ns/iter",
            "extra": "iterations: 29154\ncpu: 24020.68755573852 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 198590.25912089972,
            "unit": "ns/iter",
            "extra": "iterations: 3535\ncpu: 198504.37114567877 ns\nthreads: 1"
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
          "id": "f348ecc80d49fb655dcdbf0048223f5e246cd94a",
          "message": "Adding Bundle Term in SNL (#351)",
          "timestamp": "2026-04-11T23:49:51+02:00",
          "tree_id": "8eadd722fe2d23435e5d7c6f01b8cdf0df61bcd9",
          "url": "https://github.com/najaeda/naja/commit/f348ecc80d49fb655dcdbf0048223f5e246cd94a"
        },
        "date": 1775944453389,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64996.426398684394,
            "unit": "ns/iter",
            "extra": "iterations: 11046\ncpu: 64989.44713018288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 164933.14155252115,
            "unit": "ns/iter",
            "extra": "iterations: 4161\ncpu: 164926.72098053352 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1641747.8470590569,
            "unit": "ns/iter",
            "extra": "iterations: 425\ncpu: 1641654.8776470588 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17581747.575002283,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17581592 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1486.1296427258658,
            "unit": "ns/iter",
            "extra": "iterations: 470871\ncpu: 1486.084095219285 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14574.9662813882,
            "unit": "ns/iter",
            "extra": "iterations: 48104\ncpu: 14574.726280558782 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212957.62029644963,
            "unit": "ns/iter",
            "extra": "iterations: 3508\ncpu: 212950.90165336395 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 286096.14221680927,
            "unit": "ns/iter",
            "extra": "iterations: 2454\ncpu: 286093.63406682963 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 235181.66028874475,
            "unit": "ns/iter",
            "extra": "iterations: 2979\ncpu: 235177.60792212136 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165003.76375559688,
            "unit": "ns/iter",
            "extra": "iterations: 4271\ncpu: 164995.41676422357 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72219894.87499058,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72214325.25000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 23981.09325573553,
            "unit": "ns/iter",
            "extra": "iterations: 29210\ncpu: 23978.862444368333 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 222792.5047753115,
            "unit": "ns/iter",
            "extra": "iterations: 3142\ncpu: 222702.2883513775 ns\nthreads: 1"
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
          "id": "763b85e2d9cb7f726be465db7dddef4d3f38a6d9",
          "message": "work on common usage for najaeda and naja_edit level for primitives (#352)\n\n* work on common usage for najaeda and naja_edit level for primitives\n\n* clean coverage\n\n* cleaning najaeda build and remove xilinx.py from primitives installation\n\n* clean SNLPyLoader\n\n* cleaning files\n\n* add rule in python exclude coverage\n\n* coverage cleaning",
          "timestamp": "2026-04-14T14:02:44+02:00",
          "tree_id": "a8cdd49df8114d2f1a6267928051545e81cc1bc9",
          "url": "https://github.com/najaeda/naja/commit/763b85e2d9cb7f726be465db7dddef4d3f38a6d9"
        },
        "date": 1776168415340,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 62236.70442930054,
            "unit": "ns/iter",
            "extra": "iterations: 11740\ncpu: 62237.41132879046 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155640.78810068732,
            "unit": "ns/iter",
            "extra": "iterations: 4370\ncpu: 155611.81739130436 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1513303.3844492007,
            "unit": "ns/iter",
            "extra": "iterations: 463\ncpu: 1513111.9416846645 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18290730.025640666,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18288285.33333333 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1228.8254266720628,
            "unit": "ns/iter",
            "extra": "iterations: 570110\ncpu: 1228.534891512164 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12660.938754474711,
            "unit": "ns/iter",
            "extra": "iterations: 55302\ncpu: 12660.411431774619 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 137300.61786371964,
            "unit": "ns/iter",
            "extra": "iterations: 5430\ncpu: 137296.28342541435 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 247924.53455571647,
            "unit": "ns/iter",
            "extra": "iterations: 2836\ncpu: 247913.0744005641 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 204392.87842563615,
            "unit": "ns/iter",
            "extra": "iterations: 3430\ncpu: 204388.2903790086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 136604.9978649068,
            "unit": "ns/iter",
            "extra": "iterations: 5152\ncpu: 136606.42701863346 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73869684.5555498,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 73867951.88888879 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24726.298417268194,
            "unit": "ns/iter",
            "extra": "iterations: 27800\ncpu: 24726.002589928077 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 153483.11488579746,
            "unit": "ns/iter",
            "extra": "iterations: 4561\ncpu: 153428.16684934928 ns\nthreads: 1"
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
          "id": "3075cd639bb0db400d16815bd1ea895b01a747d1",
          "message": "refactor in order to remove repeating nl directories",
          "timestamp": "2026-04-15T12:06:40+02:00",
          "tree_id": "57ee4c1fcaa9cce5ba6c3d38169e019275facd24",
          "url": "https://github.com/najaeda/naja/commit/3075cd639bb0db400d16815bd1ea895b01a747d1"
        },
        "date": 1776247848307,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 59545.17317854893,
            "unit": "ns/iter",
            "extra": "iterations: 11543\ncpu: 59541.197175777525 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 160501.54770397602,
            "unit": "ns/iter",
            "extra": "iterations: 4486\ncpu: 160490.90347748552 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1513121.7840172108,
            "unit": "ns/iter",
            "extra": "iterations: 463\ncpu: 1512783.0518358527 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17446474.099998,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17444461.049999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1223.968511889785,
            "unit": "ns/iter",
            "extra": "iterations: 571295\ncpu: 1223.8781557689108 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12555.50000906254,
            "unit": "ns/iter",
            "extra": "iterations: 55169\ncpu: 12554.139607388208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130764.95830176071,
            "unit": "ns/iter",
            "extra": "iterations: 5276\ncpu: 130757.11144806672 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 251113.60480113598,
            "unit": "ns/iter",
            "extra": "iterations: 2791\ncpu: 251094.67610175576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 209363.69670658803,
            "unit": "ns/iter",
            "extra": "iterations: 3340\ncpu: 209348.48712574854 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141611.1891837058,
            "unit": "ns/iter",
            "extra": "iterations: 4937\ncpu: 141602.47822564305 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72016325.11110903,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 72011463.8888888 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24724.000634229775,
            "unit": "ns/iter",
            "extra": "iterations: 28381\ncpu: 24721.92442126773 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 154554.833041978,
            "unit": "ns/iter",
            "extra": "iterations: 4558\ncpu: 154458.77446247733 ns\nthreads: 1"
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
          "id": "9c1cf1da9c1e8fb4d37ebb8f06b61ef9f1f85bb3",
          "message": "Liberty bundle no pin (#353)\n\n* liberty-bundle-no-pin\n\n* adding failing test\n\n* bypass \"internal\" bundle pins in liberty parser",
          "timestamp": "2026-04-16T10:14:57+02:00",
          "tree_id": "da7db4c97febe290437d1003809b7b7ce1fbe6d2",
          "url": "https://github.com/najaeda/naja/commit/9c1cf1da9c1e8fb4d37ebb8f06b61ef9f1f85bb3"
        },
        "date": 1776327542293,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64217.97211413795,
            "unit": "ns/iter",
            "extra": "iterations: 10794\ncpu: 64212.77682045582 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 174025.32170640194,
            "unit": "ns/iter",
            "extra": "iterations: 3985\ncpu: 174013.3982434128 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1591131.5221445437,
            "unit": "ns/iter",
            "extra": "iterations: 429\ncpu: 1591021.1025641027 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17797056.76923034,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17796286.1025641 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1599.6937348092274,
            "unit": "ns/iter",
            "extra": "iterations: 437353\ncpu: 1599.5076105571477 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15743.76636102153,
            "unit": "ns/iter",
            "extra": "iterations: 44496\ncpu: 15742.190017080178 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169502.77742717927,
            "unit": "ns/iter",
            "extra": "iterations: 4120\ncpu: 169479.56480582518 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243851.40689176464,
            "unit": "ns/iter",
            "extra": "iterations: 2873\ncpu: 243817.00974591012 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 197037.9777715362,
            "unit": "ns/iter",
            "extra": "iterations: 3554\ncpu: 197006.0785030952 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 135187.00038646976,
            "unit": "ns/iter",
            "extra": "iterations: 5175\ncpu: 135175.24173913043 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74053597.62500297,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74043340.49999984 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25214.661140940763,
            "unit": "ns/iter",
            "extra": "iterations: 27749\ncpu: 25213.203646978243 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 214989.53702566633,
            "unit": "ns/iter",
            "extra": "iterations: 3268\ncpu: 214869.8271113782 ns\nthreads: 1"
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
          "id": "cb3aa27eb920d33d41bed9d8e8694b6833da6d19",
          "message": "Sv updates (#354)\n\n* report more unsupported SV elements\n\n* more SV support\n\n* negedge flops\n\n* support '|'\n\n* more sv support\n\n* more SV support\n\n* new SV supports : latch added in NLDB0\n\n* SV parsing\n\n* more SV support\n\n* add coverage scripts, switch to 0.6.0, more coverage\n\n* improve coverage\n\n* cleaning\n\n* coverage\n\n* more testing\n\n* coverage\n\n* coverage\n\n* bump slang version\n\n* exclude from cov\n\n* improve coverage\n\n* coverage\n\n* more coverage\n\n* more coverage\n\n* more coverage\n\n* cleaning",
          "timestamp": "2026-04-23T01:02:53+02:00",
          "tree_id": "2ce7a66f43c578ad163936e7a5755fda67be2663",
          "url": "https://github.com/najaeda/naja/commit/cb3aa27eb920d33d41bed9d8e8694b6833da6d19"
        },
        "date": 1776899224477,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63294.089237671564,
            "unit": "ns/iter",
            "extra": "iterations: 11150\ncpu: 63285.53192825112 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 171594.13522537658,
            "unit": "ns/iter",
            "extra": "iterations: 4193\ncpu: 171553.25232530406 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1654805.4573459863,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1654494.5426540284 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17818277.435897775,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17816546.871794872 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1491.4078913394399,
            "unit": "ns/iter",
            "extra": "iterations: 468615\ncpu: 1491.3160312836771 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14703.762041561507,
            "unit": "ns/iter",
            "extra": "iterations: 47689\ncpu: 14683.048669504504 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 191340.00858614605,
            "unit": "ns/iter",
            "extra": "iterations: 3494\ncpu: 191329.477676016 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 285509.9946960403,
            "unit": "ns/iter",
            "extra": "iterations: 2451\ncpu: 285489.7474500204 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 238432.10094851328,
            "unit": "ns/iter",
            "extra": "iterations: 2952\ncpu: 238424.24559620576 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 162550.05815569652,
            "unit": "ns/iter",
            "extra": "iterations: 4316\ncpu: 162545.08572752532 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73078455.75000016,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73072785.49999996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24129.475016289907,
            "unit": "ns/iter",
            "extra": "iterations: 29159\ncpu: 24128.84107822631 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224600.24567084917,
            "unit": "ns/iter",
            "extra": "iterations: 3118\ncpu: 224513.430083381 ns\nthreads: 1"
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
          "id": "162f008f1dadc054864254180e2e6ab4f9efd25a",
          "message": "fix always_comb overrides, improve VRLDumper layout (#355)\n\n* fix always_comb overrides, improve VRLDumper layout\n\n* clean dumper",
          "timestamp": "2026-04-23T22:40:25+02:00",
          "tree_id": "a96a20bfff22273c9e2d4d29f2752c4df5ced87c",
          "url": "https://github.com/najaeda/naja/commit/162f008f1dadc054864254180e2e6ab4f9efd25a"
        },
        "date": 1776977123207,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64362.00874206292,
            "unit": "ns/iter",
            "extra": "iterations: 10867\ncpu: 64345.403883316474 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166084.065248592,
            "unit": "ns/iter",
            "extra": "iterations: 4184\ncpu: 166078.49737093694 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1659928.9239902885,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1659866.729216152 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18400840.777777225,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18400248.16666667 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1492.1198568988452,
            "unit": "ns/iter",
            "extra": "iterations: 469318\ncpu: 1491.916214592237 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14730.918100827816,
            "unit": "ns/iter",
            "extra": "iterations: 47705\ncpu: 14730.020941201126 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202230.5137588838,
            "unit": "ns/iter",
            "extra": "iterations: 3525\ncpu: 202206.4014184396 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296992.6012711975,
            "unit": "ns/iter",
            "extra": "iterations: 2360\ncpu: 296964.47923728806 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 246119.3064516178,
            "unit": "ns/iter",
            "extra": "iterations: 2852\ncpu: 246072.68127629734 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 172228.12306937046,
            "unit": "ns/iter",
            "extra": "iterations: 4079\ncpu: 172210.33145378742 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74341238.87500732,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74328964.25000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24096.976497709245,
            "unit": "ns/iter",
            "extra": "iterations: 29061\ncpu: 24094.69994150236 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231032.61043292246,
            "unit": "ns/iter",
            "extra": "iterations: 3029\ncpu: 230818.22020466847 ns\nthreads: 1"
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
          "id": "2681f9d395472e3bdd615189d04a7e51e3d4f976",
          "message": "tests and fixes for always overrides (#356)\n\n* tests and fixes for always overrides\n\n* improve coverage\n\n* new option to dump assign as instances for debug in VRL dumper and new fix in SV Constructor\n\n* improve coverage\n\n* coverage again",
          "timestamp": "2026-04-25T03:51:49+02:00",
          "tree_id": "4192f080c4f4007cf9c14d3385c51d18ea31ff77",
          "url": "https://github.com/najaeda/naja/commit/2681f9d395472e3bdd615189d04a7e51e3d4f976"
        },
        "date": 1777082149841,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66445.43010752842,
            "unit": "ns/iter",
            "extra": "iterations: 10416\ncpu: 66440.01862519201 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165311.0002384877,
            "unit": "ns/iter",
            "extra": "iterations: 4193\ncpu: 165310.11853088482 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1646631.1009389437,
            "unit": "ns/iter",
            "extra": "iterations: 426\ncpu: 1646608.0704225346 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17699206.474999584,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17698942.6 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1494.5861528238875,
            "unit": "ns/iter",
            "extra": "iterations: 468081\ncpu: 1494.5321942142489 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14736.283822851907,
            "unit": "ns/iter",
            "extra": "iterations: 47734\ncpu: 14735.67886621694 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211195.5457725902,
            "unit": "ns/iter",
            "extra": "iterations: 3430\ncpu: 211178.27288629743 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292110.85910222976,
            "unit": "ns/iter",
            "extra": "iterations: 2406\ncpu: 292102.5083125522 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 239869.6689466469,
            "unit": "ns/iter",
            "extra": "iterations: 2924\ncpu: 239853.8242134063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165353.1948512139,
            "unit": "ns/iter",
            "extra": "iterations: 4234\ncpu: 165343.3656117146 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73544483.75000544,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73534218.00000004 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24367.481804044484,
            "unit": "ns/iter",
            "extra": "iterations: 28935\ncpu: 24365.64544669088 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228217.15344432998,
            "unit": "ns/iter",
            "extra": "iterations: 3063\ncpu: 228111.25106103916 ns\nthreads: 1"
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
          "id": "a90c4027da7ca387bf43fd82926c2b27f3d2f4ed",
          "message": "mem inference fix (#357)\n\n* mem inference fix\n\n* coverage\n\n* upgrade wheels internal actions\n\nCo-authored-by: Copilot <copilot@github.com>\n\n* coverage\n\n* coverage\n\n---------\n\nCo-authored-by: Copilot <copilot@github.com>",
          "timestamp": "2026-04-25T14:37:48+02:00",
          "tree_id": "3186d8623f2d3ecef8c58906c934562eca9d995e",
          "url": "https://github.com/najaeda/naja/commit/a90c4027da7ca387bf43fd82926c2b27f3d2f4ed"
        },
        "date": 1777120909689,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67282.57349916549,
            "unit": "ns/iter",
            "extra": "iterations: 10211\ncpu: 67279.63705807464 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166634.763908707,
            "unit": "ns/iter",
            "extra": "iterations: 4206\ncpu: 166627.06680932 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658622.1398103635,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1658624.5734597158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17784576.179487944,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17784325.128205128 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1517.7144164244532,
            "unit": "ns/iter",
            "extra": "iterations: 461217\ncpu: 1517.6978515536061 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15022.77027316801,
            "unit": "ns/iter",
            "extra": "iterations: 46638\ncpu: 15021.322076418372 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208076.10309910844,
            "unit": "ns/iter",
            "extra": "iterations: 3259\ncpu: 208064.04725375897 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 289462.38205978984,
            "unit": "ns/iter",
            "extra": "iterations: 2408\ncpu: 289444.8255813955 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 240204.88808788822,
            "unit": "ns/iter",
            "extra": "iterations: 2913\ncpu: 240196.7562650189 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 165486.7263033135,
            "unit": "ns/iter",
            "extra": "iterations: 4220\ncpu: 165470.07156398133 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73145154.87500018,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72949655.49999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24027.00355098943,
            "unit": "ns/iter",
            "extra": "iterations: 29006\ncpu: 24025.333241398326 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228077.87792885245,
            "unit": "ns/iter",
            "extra": "iterations: 3072\ncpu: 227886.9189453169 ns\nthreads: 1"
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
          "id": "907187b45ae424e70f8a88074e16f98db86e0e2b",
          "message": "Sv assign fix (#358)\n\n* Fix createAssignInstance\n\n* new fixes and starting SV regression by cloning existing repos\n\n* coverage",
          "timestamp": "2026-04-26T15:49:15+02:00",
          "tree_id": "3e525bef4610c7efac2c1b090d39c0ac7d69367e",
          "url": "https://github.com/najaeda/naja/commit/907187b45ae424e70f8a88074e16f98db86e0e2b"
        },
        "date": 1777211613203,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66288.89758753439,
            "unit": "ns/iter",
            "extra": "iterations: 10653\ncpu: 66270.88970243125 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 169120.99830754605,
            "unit": "ns/iter",
            "extra": "iterations: 4136\ncpu: 169122.25048355895 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1704282.1868932212,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1704239.4563106797 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17929223.43589855,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17929380.076923076 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1497.8539527281566,
            "unit": "ns/iter",
            "extra": "iterations: 467424\ncpu: 1497.7745237728493 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14602.479475580229,
            "unit": "ns/iter",
            "extra": "iterations: 47748\ncpu: 14601.5719192427 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 216351.9657902222,
            "unit": "ns/iter",
            "extra": "iterations: 3537\ncpu: 216328.08934124958 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 298695.8850278807,
            "unit": "ns/iter",
            "extra": "iterations: 2331\ncpu: 298672.50922350946 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 270330.2392817768,
            "unit": "ns/iter",
            "extra": "iterations: 2729\ncpu: 270318.570172224 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 175136.8413340107,
            "unit": "ns/iter",
            "extra": "iterations: 3958\ncpu: 175128.52223345096 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74351926.37500165,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74347494.37499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24199.332583414154,
            "unit": "ns/iter",
            "extra": "iterations: 28892\ncpu: 24198.74685033924 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231477.62865613916,
            "unit": "ns/iter",
            "extra": "iterations: 3043\ncpu: 231294.9408478525 ns\nthreads: 1"
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
          "id": "4fc095dfdd5ff3f51d8dd9e67dd66fbb585ce58f",
          "message": "bp regress",
          "timestamp": "2026-04-26T16:50:05+02:00",
          "tree_id": "20ea8e8acc20c1cd2ba0a4981a04409c2a415b2f",
          "url": "https://github.com/najaeda/naja/commit/4fc095dfdd5ff3f51d8dd9e67dd66fbb585ce58f"
        },
        "date": 1777215237503,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 49738.50055086524,
            "unit": "ns/iter",
            "extra": "iterations: 13615\ncpu: 49736.57605582079 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 130965.99411541077,
            "unit": "ns/iter",
            "extra": "iterations: 5268\ncpu: 130956.5 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1236731.7476979312,
            "unit": "ns/iter",
            "extra": "iterations: 543\ncpu: 1236616.4972375687 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14253182.940000214,
            "unit": "ns/iter",
            "extra": "iterations: 50\ncpu: 14251921.579999998 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1250.8185288451377,
            "unit": "ns/iter",
            "extra": "iterations: 561654\ncpu: 1250.7406285720388 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12271.414958291762,
            "unit": "ns/iter",
            "extra": "iterations: 56584\ncpu: 12270.787254347533 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 130809.67537594622,
            "unit": "ns/iter",
            "extra": "iterations: 5320\ncpu: 130798.47274436093 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 193192.60488743556,
            "unit": "ns/iter",
            "extra": "iterations: 3642\ncpu: 193187.5354200987 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 151907.0484968222,
            "unit": "ns/iter",
            "extra": "iterations: 4557\ncpu: 151894.8951064296 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 104990.50292836923,
            "unit": "ns/iter",
            "extra": "iterations: 6659\ncpu: 104981.75762126448 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57698597.000000976,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57697909.600000046 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20056.375489180704,
            "unit": "ns/iter",
            "extra": "iterations: 34752\ncpu: 20055.09915976061 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 167825.08688452095,
            "unit": "ns/iter",
            "extra": "iterations: 4178\ncpu: 167764.95955002742 ns\nthreads: 1"
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
          "id": "0157dd45161580a776b520cbb3415992fe754e8a",
          "message": "Sv fix range selects (#359)\n\n* Fix for Dynamic element-select compound assignment\n\n* update black-parrot setup\n\nCo-authored-by: Copilot <copilot@github.com>\n\n* coverage and rework SV regress\n\n---------\n\nCo-authored-by: Copilot <copilot@github.com>",
          "timestamp": "2026-04-26T21:57:18+02:00",
          "tree_id": "eff5c7907c5098f97cd5c5f3a9c831a34c05f75a",
          "url": "https://github.com/najaeda/naja/commit/0157dd45161580a776b520cbb3415992fe754e8a"
        },
        "date": 1777233694443,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63930.19478699557,
            "unit": "ns/iter",
            "extra": "iterations: 10704\ncpu: 63926.3389387145 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165374.4107903257,
            "unit": "ns/iter",
            "extra": "iterations: 4226\ncpu: 165371.5468528159 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1618808.5953492487,
            "unit": "ns/iter",
            "extra": "iterations: 430\ncpu: 1618798.4209302324 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18332517.473685294,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18332522.947368417 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.788929608292,
            "unit": "ns/iter",
            "extra": "iterations: 433860\ncpu: 1608.671518462176 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15786.846152113345,
            "unit": "ns/iter",
            "extra": "iterations: 44284\ncpu: 15786.073028633356 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168532.49570615197,
            "unit": "ns/iter",
            "extra": "iterations: 4192\ncpu: 168518.61784351134 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 245684.53632860424,
            "unit": "ns/iter",
            "extra": "iterations: 2849\ncpu: 245660.7490347492 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 201172.47419728056,
            "unit": "ns/iter",
            "extra": "iterations: 3488\ncpu: 201155.8491972477 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140587.54432993475,
            "unit": "ns/iter",
            "extra": "iterations: 4850\ncpu: 140579.17525773184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74778770.37501912,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74779725.375 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25933.09697327522,
            "unit": "ns/iter",
            "extra": "iterations: 27059\ncpu: 25931.694704164944 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 216078.55407555477,
            "unit": "ns/iter",
            "extra": "iterations: 3236\ncpu: 215972.3828801309 ns\nthreads: 1"
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
          "id": "783b5de40080b91fabb873435f3584535c9b5400",
          "message": "Sv elaboration multidriver fixes (#360)",
          "timestamp": "2026-04-27T23:47:29+02:00",
          "tree_id": "b77ed50e7275e14068acc144f2d403984905f40d",
          "url": "https://github.com/najaeda/naja/commit/783b5de40080b91fabb873435f3584535c9b5400"
        },
        "date": 1777326696021,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 61144.96064837713,
            "unit": "ns/iter",
            "extra": "iterations: 10241\ncpu: 61129.25036617519 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155797.840863563,
            "unit": "ns/iter",
            "extra": "iterations: 4493\ncpu: 155752.43957266855 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1581562.8036528982,
            "unit": "ns/iter",
            "extra": "iterations: 438\ncpu: 1581427.1986301367 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18484805.48717828,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18444184.666666668 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1607.7774445857651,
            "unit": "ns/iter",
            "extra": "iterations: 434184\ncpu: 1607.7304161369384 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15830.3843352773,
            "unit": "ns/iter",
            "extra": "iterations: 44214\ncpu: 15829.588614466025 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 168202.00406503517,
            "unit": "ns/iter",
            "extra": "iterations: 4182\ncpu: 168195.76996652308 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243227.0715029535,
            "unit": "ns/iter",
            "extra": "iterations: 2881\ncpu: 243217.5439083652 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 200432.44014890736,
            "unit": "ns/iter",
            "extra": "iterations: 3492\ncpu: 200423.1537800689 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140063.5572457923,
            "unit": "ns/iter",
            "extra": "iterations: 4996\ncpu: 140058.5806645315 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73821953.62499999,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73817535.50000015 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26391.65218198593,
            "unit": "ns/iter",
            "extra": "iterations: 26925\ncpu: 26389.845942432716 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 217333.8105091169,
            "unit": "ns/iter",
            "extra": "iterations: 3235\ncpu: 217289.61947456113 ns\nthreads: 1"
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
          "id": "6c3ee26cebd7b7738158f90fcded32461917f3bc",
          "message": "clean coverage and more sv regress (#361)",
          "timestamp": "2026-04-28T11:14:35+02:00",
          "tree_id": "4a9fe5bfd38c88d7373619877f86e4f621b3e64a",
          "url": "https://github.com/najaeda/naja/commit/6c3ee26cebd7b7738158f90fcded32461917f3bc"
        },
        "date": 1777368878654,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63985.15117744859,
            "unit": "ns/iter",
            "extra": "iterations: 10319\ncpu: 63982.168039538716 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166221.7482202358,
            "unit": "ns/iter",
            "extra": "iterations: 4214\ncpu: 166217.09729473188 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1587837.4426602481,
            "unit": "ns/iter",
            "extra": "iterations: 436\ncpu: 1587759.6857798162 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18052756.43589947,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18051919.538461532 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1607.9067440467286,
            "unit": "ns/iter",
            "extra": "iterations: 432841\ncpu: 1607.8733876873962 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15836.076499030387,
            "unit": "ns/iter",
            "extra": "iterations: 44262\ncpu: 15835.281957435278 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 176367.60109017635,
            "unit": "ns/iter",
            "extra": "iterations: 4036\ncpu: 176365.74033696728 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 244681.91692957806,
            "unit": "ns/iter",
            "extra": "iterations: 2853\ncpu: 244666.03259726582 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 200537.04507612504,
            "unit": "ns/iter",
            "extra": "iterations: 3483\ncpu: 200528.87367212182 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 141395.50603134287,
            "unit": "ns/iter",
            "extra": "iterations: 4974\ncpu: 141387.45195014053 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73603288.3750113,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73592281.87499988 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25846.981652726943,
            "unit": "ns/iter",
            "extra": "iterations: 27143\ncpu: 25845.559628633524 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225652.14368846657,
            "unit": "ns/iter",
            "extra": "iterations: 3097\ncpu: 225534.30513400355 ns\nthreads: 1"
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
          "id": "ffd9b13a4d022c262a140faea2b938f8a53ed767",
          "message": "SV fixes and more regress (#362)\n\n* SV fixes and more regress\n\n* cleaning\n\n* coverage\n\n* new fixes",
          "timestamp": "2026-04-29T09:39:10+02:00",
          "tree_id": "7e9559a1f5a2ea71f65ecb81ff7af1e227189c99",
          "url": "https://github.com/najaeda/naja/commit/ffd9b13a4d022c262a140faea2b938f8a53ed767"
        },
        "date": 1777448604988,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63430.35274934361,
            "unit": "ns/iter",
            "extra": "iterations: 11039\ncpu: 63427.34595524957 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165951.15942029233,
            "unit": "ns/iter",
            "extra": "iterations: 4278\ncpu: 165945.56545114538 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1647723.7146226957,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1647698.4268867918 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17621052.349997513,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17619986.250000007 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1494.296907379059,
            "unit": "ns/iter",
            "extra": "iterations: 469537\ncpu: 1494.1977139181797 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14654.676635319669,
            "unit": "ns/iter",
            "extra": "iterations: 47850\ncpu: 14654.262006269604 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211001.9849537181,
            "unit": "ns/iter",
            "extra": "iterations: 3456\ncpu: 210997.71383101828 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 286965.4399670907,
            "unit": "ns/iter",
            "extra": "iterations: 2432\ncpu: 286949.53741776285 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 237191.2468675926,
            "unit": "ns/iter",
            "extra": "iterations: 2953\ncpu: 237180.57433118898 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 162994.60176170702,
            "unit": "ns/iter",
            "extra": "iterations: 4314\ncpu: 162986.94668521098 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73764964.74999782,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73761364.87499996 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24089.55481269814,
            "unit": "ns/iter",
            "extra": "iterations: 29017\ncpu: 24087.09191163801 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 223437.91602796077,
            "unit": "ns/iter",
            "extra": "iterations: 3132\ncpu: 223342.98020434624 ns\nthreads: 1"
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
          "id": "a472a822064d5a2d942bebc89ef873676ea076c9",
          "message": "sim fixes and ibex hello world (#363)",
          "timestamp": "2026-05-01T11:23:11+02:00",
          "tree_id": "61749b68d9cd089543528647a29429877cf621b0",
          "url": "https://github.com/najaeda/naja/commit/a472a822064d5a2d942bebc89ef873676ea076c9"
        },
        "date": 1777627641742,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65101.91273996536,
            "unit": "ns/iter",
            "extra": "iterations: 10887\ncpu: 65096.42105263158 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 164071.9695937749,
            "unit": "ns/iter",
            "extra": "iterations: 4111\ncpu: 164070.75188518612 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1643683.5938967485,
            "unit": "ns/iter",
            "extra": "iterations: 426\ncpu: 1643576.1079812204 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17728224.15384722,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17726552.97435897 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1506.598434325141,
            "unit": "ns/iter",
            "extra": "iterations: 467530\ncpu: 1505.1690608089323 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14701.922143727326,
            "unit": "ns/iter",
            "extra": "iterations: 47562\ncpu: 14701.600100920905 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 207757.18216823292,
            "unit": "ns/iter",
            "extra": "iterations: 3376\ncpu: 207735.84004739326 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 293343.0058552846,
            "unit": "ns/iter",
            "extra": "iterations: 2391\ncpu: 293334.97114178166 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 243032.0521408843,
            "unit": "ns/iter",
            "extra": "iterations: 2896\ncpu: 243011.44751381225 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 169885.32905569675,
            "unit": "ns/iter",
            "extra": "iterations: 4130\ncpu: 169878.0426150121 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72350346.74999952,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72348230.00000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24122.303338369915,
            "unit": "ns/iter",
            "extra": "iterations: 29116\ncpu: 24121.006250858623 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 222907.60451545217,
            "unit": "ns/iter",
            "extra": "iterations: 3143\ncpu: 222751.35475656533 ns\nthreads: 1"
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
          "id": "83a2dbaa96f77f824d49e902a292c2e27eb5d10f",
          "message": "clean coverage (#365)",
          "timestamp": "2026-05-02T22:28:56+02:00",
          "tree_id": "875ea6d0b19e5f3289522d8a5e768271890e8d77",
          "url": "https://github.com/najaeda/naja/commit/83a2dbaa96f77f824d49e902a292c2e27eb5d10f"
        },
        "date": 1777754032713,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64802.40595683264,
            "unit": "ns/iter",
            "extra": "iterations: 10979\ncpu: 64794.657072593145 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166158.71480939613,
            "unit": "ns/iter",
            "extra": "iterations: 4092\ncpu: 166150.5146627566 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658027.8625591989,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1657820.9834123214 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17973838.769232012,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17971590.461538456 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1505.1090120090316,
            "unit": "ns/iter",
            "extra": "iterations: 465490\ncpu: 1504.9878472147623 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14658.346132886902,
            "unit": "ns/iter",
            "extra": "iterations: 47710\ncpu: 14655.467009012786 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212235.57480085216,
            "unit": "ns/iter",
            "extra": "iterations: 3389\ncpu: 212208.9589849511 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292103.6544240376,
            "unit": "ns/iter",
            "extra": "iterations: 2396\ncpu: 292083.4632721201 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 242139.22954777774,
            "unit": "ns/iter",
            "extra": "iterations: 2897\ncpu: 242116.77597514712 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 170164.25699805465,
            "unit": "ns/iter",
            "extra": "iterations: 4144\ncpu: 170152.55743243246 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74696074.6250096,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74694466.75000001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24109.88249672801,
            "unit": "ns/iter",
            "extra": "iterations: 29046\ncpu: 24107.71972044346 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224087.3783409607,
            "unit": "ns/iter",
            "extra": "iterations: 3140\ncpu: 223981.79904459108 ns\nthreads: 1"
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
          "id": "9dde4931c2c2492d162c4a2989c6736508dc5952",
          "message": "cleaning regress (#366)\n\n* cleaning regress\n\n* add round script to verify SV loading - CVA6 loading\n\n* fix SV\n\n* SV fix\n\n* fix multi driver\n\n* compute coverange with clang\n\n* fix coverage\n\n* new attempt\n\n* coverage\n\n* new testing",
          "timestamp": "2026-05-04T23:05:23+02:00",
          "tree_id": "fdfe2546cc4612e929db6b50ff3700d4870e433a",
          "url": "https://github.com/najaeda/naja/commit/9dde4931c2c2492d162c4a2989c6736508dc5952"
        },
        "date": 1777929082012,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66445.29497850491,
            "unit": "ns/iter",
            "extra": "iterations: 10933\ncpu: 66424.8613372359 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166669.39733206175,
            "unit": "ns/iter",
            "extra": "iterations: 4198\ncpu: 166620.12243925678 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1665143.9857819306,
            "unit": "ns/iter",
            "extra": "iterations: 422\ncpu: 1664757.6279620857 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17729596.384615667,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 17729151.589743588 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1498.989876104578,
            "unit": "ns/iter",
            "extra": "iterations: 468298\ncpu: 1498.9612810646208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14785.089008851175,
            "unit": "ns/iter",
            "extra": "iterations: 47793\ncpu: 14784.630887368434 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 202019.56215887377,
            "unit": "ns/iter",
            "extra": "iterations: 3298\ncpu: 202019.3599150999 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 288082.16220665636,
            "unit": "ns/iter",
            "extra": "iterations: 2429\ncpu: 288085.144092219 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 238309.7568212817,
            "unit": "ns/iter",
            "extra": "iterations: 2932\ncpu: 238303.83799454296 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 164261.55289940457,
            "unit": "ns/iter",
            "extra": "iterations: 4225\ncpu: 164261.03857988166 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72283337.99999832,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72279950.25 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24126.11891798755,
            "unit": "ns/iter",
            "extra": "iterations: 29020\ncpu: 24125.931564438317 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 226344.76880814857,
            "unit": "ns/iter",
            "extra": "iterations: 3097\ncpu: 226173.2102034208 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "noam.chn1@gmail.com",
            "name": "Noam Cohen",
            "username": "nanocoh"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "05a14c34844b62a2e40e8ff459d0cf7bb5d6d9ee",
          "message": "memory modeling (#364)",
          "timestamp": "2026-05-05T09:47:35+02:00",
          "tree_id": "3f530f55aba064adb86a5d804a7157c4180a194d",
          "url": "https://github.com/najaeda/naja/commit/05a14c34844b62a2e40e8ff459d0cf7bb5d6d9ee"
        },
        "date": 1777967509005,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 68850.65832291047,
            "unit": "ns/iter",
            "extra": "iterations: 10387\ncpu: 68845.31828246848 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 169191.8628115307,
            "unit": "ns/iter",
            "extra": "iterations: 4133\ncpu: 169147.99806436003 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1662839.8622327899,
            "unit": "ns/iter",
            "extra": "iterations: 421\ncpu: 1662785.0546318288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18162168.153848004,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18161307.58974358 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1493.0967169589794,
            "unit": "ns/iter",
            "extra": "iterations: 468925\ncpu: 1493.077340726128 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14660.215318918343,
            "unit": "ns/iter",
            "extra": "iterations: 47771\ncpu: 14659.136379812015 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206853.15352816696,
            "unit": "ns/iter",
            "extra": "iterations: 3387\ncpu: 206820.86920578673 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 298380.5255754512,
            "unit": "ns/iter",
            "extra": "iterations: 2346\ncpu: 298296.1884057971 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 247021.5408016902,
            "unit": "ns/iter",
            "extra": "iterations: 2794\ncpu: 246989.17752326385 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 173405.18961231835,
            "unit": "ns/iter",
            "extra": "iterations: 4024\ncpu: 173390.02037773374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73098221.12499376,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73084998.37499993 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24237.24224913209,
            "unit": "ns/iter",
            "extra": "iterations: 28900\ncpu: 24234.92055363323 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 227557.9571009058,
            "unit": "ns/iter",
            "extra": "iterations: 3077\ncpu: 226801.33994147487 ns\nthreads: 1"
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
          "id": "cfefd37c8e2c5d64d574105b83f86641792e70ce",
          "message": "Cv32e40p simulation (#367)\n\n* adding simulation setup\n\n* fix simulation\n\n* sim again\n\n* update regress\n\n* clean getString\n\n* name assign\n\n* clean test\n\n* cleaning and coverage\n\n* cleaning\n\n* test bundle terms\n\n* setting global coverage\n\n* add license",
          "timestamp": "2026-05-07T10:37:58+02:00",
          "tree_id": "85830e878a67aae4df9620a219ce532975af08ff",
          "url": "https://github.com/najaeda/naja/commit/cfefd37c8e2c5d64d574105b83f86641792e70ce"
        },
        "date": 1778143338594,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 69754.9769589849,
            "unit": "ns/iter",
            "extra": "iterations: 10069\ncpu: 69750.01072599068 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 167537.4248568605,
            "unit": "ns/iter",
            "extra": "iterations: 4192\ncpu: 167535.8373091603 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1658631.3466980453,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1658629.9033018867 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18449255.02702633,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18447721.216216207 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1509.3277644641062,
            "unit": "ns/iter",
            "extra": "iterations: 466103\ncpu: 1508.9952714314213 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14717.557545026368,
            "unit": "ns/iter",
            "extra": "iterations: 47528\ncpu: 14705.090809627996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 212574.67580491543,
            "unit": "ns/iter",
            "extra": "iterations: 3137\ncpu: 212535.62958240343 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292869.19616027095,
            "unit": "ns/iter",
            "extra": "iterations: 2396\ncpu: 292847.85225375637 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 241846.55498105392,
            "unit": "ns/iter",
            "extra": "iterations: 2901\ncpu: 241821.70492933484 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 167951.53849844166,
            "unit": "ns/iter",
            "extra": "iterations: 4169\ncpu: 167941.15543295737 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72230230.0000024,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72226162.25 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24138.492338946948,
            "unit": "ns/iter",
            "extra": "iterations: 29043\ncpu: 24137.187928244315 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 229203.69356971834,
            "unit": "ns/iter",
            "extra": "iterations: 3048\ncpu: 229107.83497374287 ns\nthreads: 1"
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
          "id": "6a3d849c8c0f4fedb93c6a5be0d07ac4e5b1df60",
          "message": "new cv32e40p sim and fix for bug detected with the design (#368)",
          "timestamp": "2026-05-07T21:46:03+02:00",
          "tree_id": "79f1b8a83e5a9512c30ceed9a24e32382a58e23a",
          "url": "https://github.com/najaeda/naja/commit/6a3d849c8c0f4fedb93c6a5be0d07ac4e5b1df60"
        },
        "date": 1778183417863,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67912.50979637541,
            "unit": "ns/iter",
            "extra": "iterations: 10412\ncpu: 67892.19746446409 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166710.77539342578,
            "unit": "ns/iter",
            "extra": "iterations: 4194\ncpu: 166701.43800667624 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1644363.6861822596,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1644320.5573770485 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17587399.12500005,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17585670.825 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1588.202610963497,
            "unit": "ns/iter",
            "extra": "iterations: 442059\ncpu: 1587.7579825317428 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15572.718072828622,
            "unit": "ns/iter",
            "extra": "iterations: 45040\ncpu: 15570.907504440489 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 208025.90504099915,
            "unit": "ns/iter",
            "extra": "iterations: 3412\ncpu: 208004.44812426754 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296326.364179049,
            "unit": "ns/iter",
            "extra": "iterations: 2345\ncpu: 296301.09936034086 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 247847.85714290992,
            "unit": "ns/iter",
            "extra": "iterations: 2842\ncpu: 247629.51337086546 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 174900.57525671917,
            "unit": "ns/iter",
            "extra": "iterations: 3993\ncpu: 174892.10493363408 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72906311.49999171,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72669894.37500015 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24106.015097721138,
            "unit": "ns/iter",
            "extra": "iterations: 29011\ncpu: 24105.005928785606 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 228710.61038201992,
            "unit": "ns/iter",
            "extra": "iterations: 3062\ncpu: 228061.6463095866 ns\nthreads: 1"
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
          "id": "af1d00128ca51f9182c7f37cdbd5ce3faac4a9cc",
          "message": "IBEX sim and fix (#369)",
          "timestamp": "2026-05-08T10:54:00+02:00",
          "tree_id": "30c067cdfc2b4f57ba952146cc9945e565a9196f",
          "url": "https://github.com/najaeda/naja/commit/af1d00128ca51f9182c7f37cdbd5ce3faac4a9cc"
        },
        "date": 1778231324730,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 51177.94448068902,
            "unit": "ns/iter",
            "extra": "iterations: 13797\ncpu: 51173.509023700804 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 131676.43695897478,
            "unit": "ns/iter",
            "extra": "iterations: 5314\ncpu: 131670.1802785096 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1224155.9982143713,
            "unit": "ns/iter",
            "extra": "iterations: 560\ncpu: 1224031.4303571428 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 13686646.098039107,
            "unit": "ns/iter",
            "extra": "iterations: 51\ncpu: 13685452.352941174 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1269.6179137060485,
            "unit": "ns/iter",
            "extra": "iterations: 557908\ncpu: 1269.4472206887162 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12427.16530375975,
            "unit": "ns/iter",
            "extra": "iterations: 53562\ncpu: 12425.907453045067 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 133388.56502751706,
            "unit": "ns/iter",
            "extra": "iterations: 5267\ncpu: 133374.10708183027 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 186813.30647750272,
            "unit": "ns/iter",
            "extra": "iterations: 3736\ncpu: 186809.20449678806 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 152899.60528613676,
            "unit": "ns/iter",
            "extra": "iterations: 4578\ncpu: 152881.3997378767 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 104743.77313838765,
            "unit": "ns/iter",
            "extra": "iterations: 6634\ncpu: 104736.21359662343 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 56542619.499998644,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 56538230.80000003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 20488.46210683098,
            "unit": "ns/iter",
            "extra": "iterations: 34241\ncpu: 20486.594813235573 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 168152.67312281163,
            "unit": "ns/iter",
            "extra": "iterations: 4182\ncpu: 167970.5277378991 ns\nthreads: 1"
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
          "id": "dbbac1c86735a01104c9651dfbe2f110df50f5a7",
          "message": "more ibex sim",
          "timestamp": "2026-05-08T11:39:54+02:00",
          "tree_id": "25ebf688b233fdfc607f8a47ed075804db04fedf",
          "url": "https://github.com/najaeda/naja/commit/dbbac1c86735a01104c9651dfbe2f110df50f5a7"
        },
        "date": 1778234251363,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 60259.047533251316,
            "unit": "ns/iter",
            "extra": "iterations: 11655\ncpu: 60254.8906906907 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155900.38852934886,
            "unit": "ns/iter",
            "extra": "iterations: 4481\ncpu: 155889.62776166038 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1518824.0865800902,
            "unit": "ns/iter",
            "extra": "iterations: 462\ncpu: 1518715.8051948044 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17825708.50000127,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17824586.19999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1217.8182625328773,
            "unit": "ns/iter",
            "extra": "iterations: 575539\ncpu: 1217.8016033665826 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12650.076904931122,
            "unit": "ns/iter",
            "extra": "iterations: 55107\ncpu: 12649.052298256109 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 129179.92204251229,
            "unit": "ns/iter",
            "extra": "iterations: 5503\ncpu: 129166.17790296208 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 247363.70685993772,
            "unit": "ns/iter",
            "extra": "iterations: 2828\ncpu: 247339.4614568598 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 203117.6142029018,
            "unit": "ns/iter",
            "extra": "iterations: 3450\ncpu: 203100.8857971015 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 136445.94330399827,
            "unit": "ns/iter",
            "extra": "iterations: 5115\ncpu: 136428.47585532742 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72842500.55555376,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 72837140.33333331 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24180.369529085132,
            "unit": "ns/iter",
            "extra": "iterations: 28880\ncpu: 24177.99103185592 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 155041.6091938398,
            "unit": "ns/iter",
            "extra": "iterations: 4547\ncpu: 154736.85374973525 ns\nthreads: 1"
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
          "id": "002deb8d229e62e8f0fc256f10064412b824e980",
          "message": "More SV sims (#370)",
          "timestamp": "2026-05-09T15:23:26+02:00",
          "tree_id": "e65cfbbfd53ba17ba29436d73351ee866de992e9",
          "url": "https://github.com/najaeda/naja/commit/002deb8d229e62e8f0fc256f10064412b824e980"
        },
        "date": 1778333252528,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66270.68826815285,
            "unit": "ns/iter",
            "extra": "iterations: 10740\ncpu: 66248.83249534451 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165382.06310336044,
            "unit": "ns/iter",
            "extra": "iterations: 4247\ncpu: 165335.11914292438 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1642182.8454332573,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1641726.0327868857 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17368797.400000345,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17368415.574999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1491.597666797099,
            "unit": "ns/iter",
            "extra": "iterations: 467769\ncpu: 1491.5278075289298 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14617.583565721578,
            "unit": "ns/iter",
            "extra": "iterations: 47693\ncpu: 14617.288029689891 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 211254.29985096972,
            "unit": "ns/iter",
            "extra": "iterations: 3355\ncpu: 211255.53800298058 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 296301.7428209657,
            "unit": "ns/iter",
            "extra": "iterations: 2368\ncpu: 296292.2461993246 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 244824.03155680993,
            "unit": "ns/iter",
            "extra": "iterations: 2852\ncpu: 244815.94319775593 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 172808.45458996136,
            "unit": "ns/iter",
            "extra": "iterations: 4085\ncpu: 172801.17625458984 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 71581757.24999438,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 71574267.50000018 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24104.91974906592,
            "unit": "ns/iter",
            "extra": "iterations: 29171\ncpu: 24104.538719961572 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224354.83407907214,
            "unit": "ns/iter",
            "extra": "iterations: 3128\ncpu: 224230.05019180072 ns\nthreads: 1"
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
          "id": "acd6ffaf1e6a64912b7139c8fea9a6fd736376f4",
          "message": "addinx new axi bench and towards 0.6.3 (#371)",
          "timestamp": "2026-05-14T15:32:02+02:00",
          "tree_id": "b87c48602ed81b8a72e8dc65ce9e518e7f823159",
          "url": "https://github.com/najaeda/naja/commit/acd6ffaf1e6a64912b7139c8fea9a6fd736376f4"
        },
        "date": 1778765777385,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64203.228613296946,
            "unit": "ns/iter",
            "extra": "iterations: 10918\ncpu: 64179.14480674116 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165270.38539697498,
            "unit": "ns/iter",
            "extra": "iterations: 4232\ncpu: 165264.91351606805 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1568808.348519298,
            "unit": "ns/iter",
            "extra": "iterations: 439\ncpu: 1568696.0751708425 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17564481.299999103,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17563827.249999996 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1622.9057842871102,
            "unit": "ns/iter",
            "extra": "iterations: 432603\ncpu: 1622.087720149883 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16012.180478652119,
            "unit": "ns/iter",
            "extra": "iterations: 43706\ncpu: 16010.888756692453 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173417.49754300856,
            "unit": "ns/iter",
            "extra": "iterations: 4070\ncpu: 173395.67813267803 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 240324.37339355596,
            "unit": "ns/iter",
            "extra": "iterations: 2879\ncpu: 240311.0211879123 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 195364.41085923064,
            "unit": "ns/iter",
            "extra": "iterations: 3573\ncpu: 195331.66442765167 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 135582.46952161263,
            "unit": "ns/iter",
            "extra": "iterations: 5184\ncpu: 135568.64949845657 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74290677.00000757,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74280224.37500003 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26275.454650858366,
            "unit": "ns/iter",
            "extra": "iterations: 26737\ncpu: 26272.792235478948 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 216517.76255276418,
            "unit": "ns/iter",
            "extra": "iterations: 3226\ncpu: 216383.65654060172 ns\nthreads: 1"
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
          "id": "17544c56bc1234953396bee2ca0c9e352771d71d",
          "message": "B0.6.4 (#372)",
          "timestamp": "2026-05-17T00:07:16+02:00",
          "tree_id": "dcdc0c35ade858a23f60f084af3504cfff98bf96",
          "url": "https://github.com/najaeda/naja/commit/17544c56bc1234953396bee2ca0c9e352771d71d"
        },
        "date": 1778969481323,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65980.14330365245,
            "unit": "ns/iter",
            "extra": "iterations: 10558\ncpu: 65972.47253267665 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 161131.24687216157,
            "unit": "ns/iter",
            "extra": "iterations: 3277\ncpu: 161115.91547146783 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1568373.42953019,
            "unit": "ns/iter",
            "extra": "iterations: 447\ncpu: 1568320.4250559288 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17592918.125001233,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17592204.525 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1618.956286420517,
            "unit": "ns/iter",
            "extra": "iterations: 433824\ncpu: 1618.9188334439768 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16012.45214982527,
            "unit": "ns/iter",
            "extra": "iterations: 43678\ncpu: 16011.917303905868 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 173592.59941802817,
            "unit": "ns/iter",
            "extra": "iterations: 4124\ncpu: 173587.96047526662 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 250925.2540308258,
            "unit": "ns/iter",
            "extra": "iterations: 2791\ncpu: 250919.48907201728 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 207841.0568720493,
            "unit": "ns/iter",
            "extra": "iterations: 3376\ncpu: 207835.4567535545 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 147142.5204038645,
            "unit": "ns/iter",
            "extra": "iterations: 4754\ncpu: 147135.12010938174 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74414226.74999388,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74408623.37500009 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26307.43138435804,
            "unit": "ns/iter",
            "extra": "iterations: 26561\ncpu: 26306.71085426003 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224617.72820723,
            "unit": "ns/iter",
            "extra": "iterations: 3109\ncpu: 224503.57060146885 ns\nthreads: 1"
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
          "id": "7c620f8b3ab850ef20e9c9094df8b11ac8f68900",
          "message": "Tutorials readme reworked (#374)\n\n* prepare next version and simplify README\n\n* merging tutorials in repo in progress\n\n* integrate tutorials in naja\n\n* updates to README\n\n* clean licenses\n\n* setup najaeda to test tutorials\n\n* fix tutorial\n\n* don't change version",
          "timestamp": "2026-05-18T12:11:28+02:00",
          "tree_id": "d8301627ebb4c5ce4528e212606477d2fc96a7db",
          "url": "https://github.com/najaeda/naja/commit/7c620f8b3ab850ef20e9c9094df8b11ac8f68900"
        },
        "date": 1779099385419,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 67083.72104180636,
            "unit": "ns/iter",
            "extra": "iterations: 10213\ncpu: 67082.15313815726 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 175237.0410307699,
            "unit": "ns/iter",
            "extra": "iterations: 3997\ncpu: 175227.8628971729 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1679805.6043167165,
            "unit": "ns/iter",
            "extra": "iterations: 417\ncpu: 1679724.2829736213 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18819332.861110322,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 18817621.77777778 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.8827710904682,
            "unit": "ns/iter",
            "extra": "iterations: 435814\ncpu: 1608.8127297425037 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15808.37285643204,
            "unit": "ns/iter",
            "extra": "iterations: 44202\ncpu: 15807.994796615545 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169542.19834104966,
            "unit": "ns/iter",
            "extra": "iterations: 4099\ncpu: 169533.78506952917 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 244892.76745808125,
            "unit": "ns/iter",
            "extra": "iterations: 2864\ncpu: 244880.21194134103 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 199584.17997720052,
            "unit": "ns/iter",
            "extra": "iterations: 3506\ncpu: 199568.9663434111 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 139869.35721392787,
            "unit": "ns/iter",
            "extra": "iterations: 5025\ncpu: 139866.0927363184 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74420057.62500514,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74411487.12500012 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26898.578802370994,
            "unit": "ns/iter",
            "extra": "iterations: 26135\ncpu: 26897.48433135645 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224688.13540191992,
            "unit": "ns/iter",
            "extra": "iterations: 3124\ncpu: 224614.22791291645 ns\nthreads: 1"
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
          "id": "3634212302c1916a81bf9c8de672e66bde7e3a81",
          "message": "Fix for SV (#375)",
          "timestamp": "2026-05-19T07:33:06+02:00",
          "tree_id": "0787e8c2967eb03306e8732ce74cbfdfa5fa4d2f",
          "url": "https://github.com/najaeda/naja/commit/3634212302c1916a81bf9c8de672e66bde7e3a81"
        },
        "date": 1779169024521,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66227.59052374613,
            "unit": "ns/iter",
            "extra": "iterations: 10616\ncpu: 66227.26525998492 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 167519.85274233096,
            "unit": "ns/iter",
            "extra": "iterations: 3993\ncpu: 167510.07162534434 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1644323.2763464863,
            "unit": "ns/iter",
            "extra": "iterations: 427\ncpu: 1644242.5011709605 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17778545.72500246,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17778292.89999999 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1499.8809501900205,
            "unit": "ns/iter",
            "extra": "iterations: 467275\ncpu: 1499.8484875073555 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14696.245271136688,
            "unit": "ns/iter",
            "extra": "iterations: 47633\ncpu: 14695.829844855445 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 207863.0689558295,
            "unit": "ns/iter",
            "extra": "iterations: 3553\ncpu: 207859.32395159034 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 295121.4587888674,
            "unit": "ns/iter",
            "extra": "iterations: 2378\ncpu: 295106.0815811606 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 244856.1006969346,
            "unit": "ns/iter",
            "extra": "iterations: 2870\ncpu: 244856.82717770006 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 169846.3176328207,
            "unit": "ns/iter",
            "extra": "iterations: 4140\ncpu: 169836.0041062799 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73042692.12500003,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73037226.3750001 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24189.819570697753,
            "unit": "ns/iter",
            "extra": "iterations: 28931\ncpu: 24188.581417856265 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 233304.5716638935,
            "unit": "ns/iter",
            "extra": "iterations: 3000\ncpu: 233196.91166670728 ns\nthreads: 1"
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
          "id": "6d7199df68d6fb326bb3c62c65a9bdf759460c18",
          "message": "B0.6.6 (#376)\n\n* prepare next version\n\n* remove PINMISSING suppression warning in verilator linting\n\n* more SV support\n\n* edit IBEX linting\n\n* more avoided warnings\n\n* more suppression\n\n* more SV support\n\n* Fix more SV and tutorials\n\n* new support and cleaning\n\n* more SV support\n\n* next row of support\n\n* more support\n\n* improve coverage\n\n* coverage\n\n* coverage\n\n* coverage\n\n* coverage",
          "timestamp": "2026-05-21T15:52:30+02:00",
          "tree_id": "7c7012a74a21c42ed82c7a2923fb628995da0d61",
          "url": "https://github.com/najaeda/naja/commit/6d7199df68d6fb326bb3c62c65a9bdf759460c18"
        },
        "date": 1779371794416,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 62751.62666029851,
            "unit": "ns/iter",
            "extra": "iterations: 10465\ncpu: 62749.19034878165 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 154432.5795154114,
            "unit": "ns/iter",
            "extra": "iterations: 4540\ncpu: 154423.35374449336 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1559159.5501112763,
            "unit": "ns/iter",
            "extra": "iterations: 449\ncpu: 1559084.4565701562 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17319812.62500142,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17316640.1 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1608.816908904764,
            "unit": "ns/iter",
            "extra": "iterations: 434008\ncpu: 1608.6466009843145 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15845.566482088323,
            "unit": "ns/iter",
            "extra": "iterations: 44245\ncpu: 15843.827076505822 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169523.1939291759,
            "unit": "ns/iter",
            "extra": "iterations: 4151\ncpu: 169505.58949650684 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242123.0606687434,
            "unit": "ns/iter",
            "extra": "iterations: 2901\ncpu: 241928.1447776628 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198215.22115386577,
            "unit": "ns/iter",
            "extra": "iterations: 3536\ncpu: 198199.48755656095 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 137354.17583495812,
            "unit": "ns/iter",
            "extra": "iterations: 5090\ncpu: 137352.51434184695 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74588179.74999477,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74584053.62499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26224.147117219985,
            "unit": "ns/iter",
            "extra": "iterations: 26693\ncpu: 26223.38216011685 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225218.88949700422,
            "unit": "ns/iter",
            "extra": "iterations: 3104\ncpu: 224994.51449740244 ns\nthreads: 1"
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
          "id": "8fa27fca8bfe64dd5f19229546491e3301b93024",
          "message": "test and fix for the bug (#378)\n\n* test and fix for the bug\n\n* coverage\n\n* Trigger CI\n\n* Trigger CI",
          "timestamp": "2026-05-26T16:50:48+02:00",
          "tree_id": "bafec6b8b66fe5371cca74ceb70abff5809d9243",
          "url": "https://github.com/najaeda/naja/commit/8fa27fca8bfe64dd5f19229546491e3301b93024"
        },
        "date": 1779807296320,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 66317.43007990801,
            "unit": "ns/iter",
            "extra": "iterations: 10512\ncpu: 66297.38061263318 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 155026.2799822622,
            "unit": "ns/iter",
            "extra": "iterations: 4511\ncpu: 155019.38062513858 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1560913.5666666892,
            "unit": "ns/iter",
            "extra": "iterations: 450\ncpu: 1560854.1977777774 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17645993.600000054,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17645409.724999994 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1604.4662643641816,
            "unit": "ns/iter",
            "extra": "iterations: 436156\ncpu: 1604.430942139968 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15796.447381501412,
            "unit": "ns/iter",
            "extra": "iterations: 44262\ncpu: 15795.046315123569 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 172703.0081200691,
            "unit": "ns/iter",
            "extra": "iterations: 4064\ncpu: 172687.83686023628 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 242933.4833625119,
            "unit": "ns/iter",
            "extra": "iterations: 2855\ncpu: 242920.0739054289 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198268.9177538287,
            "unit": "ns/iter",
            "extra": "iterations: 3526\ncpu: 198254.2532614861 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 138505.16116926546,
            "unit": "ns/iter",
            "extra": "iterations: 5063\ncpu: 138484.38336954374 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74675650.12499477,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74666502.24999994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26756.768899451396,
            "unit": "ns/iter",
            "extra": "iterations: 26932\ncpu: 26754.532080796085 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 223112.72733033373,
            "unit": "ns/iter",
            "extra": "iterations: 3154\ncpu: 222894.1122384599 ns\nthreads: 1"
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
          "id": "d4bab5e687cedb120217261e42209ee53abd03f2",
          "message": "B0.6.7 (#377)\n\n* Towards 0.6.7 and more SV support\n\n* divmod prim\n\n* more SV support\n\n* support more latch cases\n\n* new latch support\n\n* new support\n\n* next row of support\n\n* next support\n\n* Trigger CI\n\n* Trigger CI\n\n* new row of support\n\n* next row\n\n* more support\n\n* new set of support\n\n* update workflow\n\n* update tutorial\n\n* clean dot output\n\n* parallel tests\n\n* precompute timing modeling as it is not thread safe\n\n* Revert \"precompute timing modeling as it is not thread safe\"\n\nThis reverts commit 8bc5be5e2ff72e62cbe455ab680369fc319f2221.\n\n* cleaning workflows and adding //\n\n* test with tsan\n\n* test cleaning\n\n* edit tsan options\n\n* coverage\n\n* tsan avoiding TBB noise\n\n* mt safety fix for NLName\n\n* revert test change\n\n* Revert \"mt safety fix for NLName\"\n\n* clean dot usage through option in tests\n\n* set -> unique vector for ll\n\n* preprocess design before multi-threading operation\n\n* clean tsan\n\n* cleaning\n\n* rerun wheels\n\n* run wheels\n\n* coverage\n\n* coverage\n\n* coverage\n\n---------\n\nCo-authored-by: Noam Cohen <noam.chn1@gmail.com>",
          "timestamp": "2026-06-01T22:02:34+02:00",
          "tree_id": "edebf00c874a20531b0e5d90ab6b5e6e7f9b2b6f",
          "url": "https://github.com/najaeda/naja/commit/d4bab5e687cedb120217261e42209ee53abd03f2"
        },
        "date": 1780344287275,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 64654.09025669152,
            "unit": "ns/iter",
            "extra": "iterations: 10869\ncpu: 64644.5771460116 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 166232.24613555634,
            "unit": "ns/iter",
            "extra": "iterations: 4205\ncpu: 166224.21569560052 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1577295.9225513358,
            "unit": "ns/iter",
            "extra": "iterations: 439\ncpu: 1577200.0842824602 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17987414.73684261,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 17986966.789473683 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1611.8903845226664,
            "unit": "ns/iter",
            "extra": "iterations: 435787\ncpu: 1611.7881602709574 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15875.0420071066,
            "unit": "ns/iter",
            "extra": "iterations: 44183\ncpu: 15874.004707692991 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171539.0678997409,
            "unit": "ns/iter",
            "extra": "iterations: 4109\ncpu: 171528.07544414702 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 243494.7500881203,
            "unit": "ns/iter",
            "extra": "iterations: 2837\ncpu: 243477.86640817756 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 198730.4209631658,
            "unit": "ns/iter",
            "extra": "iterations: 3530\ncpu: 198718.30623229465 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 140394.7392354052,
            "unit": "ns/iter",
            "extra": "iterations: 4970\ncpu: 140380.36458752514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74395187.62500086,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74396279.87499997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 26045.12090457713,
            "unit": "ns/iter",
            "extra": "iterations: 27195\ncpu: 26043.985364956763 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224637.9570808623,
            "unit": "ns/iter",
            "extra": "iterations: 3099\ncpu: 224580.17554049642 ns\nthreads: 1"
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
          "id": "12b3e86bcb5326b06ecfe97864e865dda459402b",
          "message": "add rl8 support (#380)\n\n* add rl8 support\n\n* new attempt\n\n* adapt parser to rockylinux 8\n\n* use concurrent_hash_map from tbb\n\n* compile oneTBB\n\n* fix curl",
          "timestamp": "2026-06-03T10:44:19+02:00",
          "tree_id": "bf6aeb3d76a7bacb62986ca59bbdcc37ed6dbe59",
          "url": "https://github.com/najaeda/naja/commit/12b3e86bcb5326b06ecfe97864e865dda459402b"
        },
        "date": 1780476383835,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 50980.80970000183,
            "unit": "ns/iter",
            "extra": "iterations: 10000\ncpu: 50959.79029999999 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 131178.2512333899,
            "unit": "ns/iter",
            "extra": "iterations: 5270\ncpu: 131142.8322580645 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1263827.4405851292,
            "unit": "ns/iter",
            "extra": "iterations: 547\ncpu: 1263755.7824497255 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 14267429.755101817,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 14266901.653061224 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1246.360376499326,
            "unit": "ns/iter",
            "extra": "iterations: 562126\ncpu: 1246.1646943923606 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 12269.764356435553,
            "unit": "ns/iter",
            "extra": "iterations: 57065\ncpu: 12267.10092000351 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 132667.74849171014,
            "unit": "ns/iter",
            "extra": "iterations: 5304\ncpu: 132631.33163650084 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 188870.44160388454,
            "unit": "ns/iter",
            "extra": "iterations: 3716\ncpu: 188836.75242195913 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 156102.79018349186,
            "unit": "ns/iter",
            "extra": "iterations: 4523\ncpu: 156092.72009728063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 107101.3735456145,
            "unit": "ns/iter",
            "extra": "iterations: 6532\ncpu: 107094.59522351493 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 57463293.49999541,
            "unit": "ns/iter",
            "extra": "iterations: 10\ncpu: 57457056.799999855 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 19903.02375385187,
            "unit": "ns/iter",
            "extra": "iterations: 35068\ncpu: 19902.390127751773 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 174124.1503487878,
            "unit": "ns/iter",
            "extra": "iterations: 4004\ncpu: 173978.53221775775 ns\nthreads: 1"
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
          "id": "7576360808d16632847c1d02a9282531baf24143",
          "message": "Remove Codacy badge from README\n\nRemoved Codacy badge from README.md",
          "timestamp": "2026-06-03T13:23:40+02:00",
          "tree_id": "d556d8a309063e3b746c48e3a2a4d8351f9418ec",
          "url": "https://github.com/najaeda/naja/commit/7576360808d16632847c1d02a9282531baf24143"
        },
        "date": 1780485962763,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 65819.69705884489,
            "unit": "ns/iter",
            "extra": "iterations: 10540\ncpu: 65818.26034155599 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 165561.66241735657,
            "unit": "ns/iter",
            "extra": "iterations: 4236\ncpu: 165532.49834749763 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1638830.6674527165,
            "unit": "ns/iter",
            "extra": "iterations: 424\ncpu: 1638730.3396226414 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17316708.07499768,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17316250.575000003 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1493.5113322325385,
            "unit": "ns/iter",
            "extra": "iterations: 469325\ncpu: 1493.4764566132208 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14956.763678993679,
            "unit": "ns/iter",
            "extra": "iterations: 46915\ncpu: 14956.043845252052 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206983.54161950067,
            "unit": "ns/iter",
            "extra": "iterations: 3532\ncpu: 206980.3550396375 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 293139.078562507,
            "unit": "ns/iter",
            "extra": "iterations: 2393\ncpu: 293118.12118679495 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 241254.23177442068,
            "unit": "ns/iter",
            "extra": "iterations: 2908\ncpu: 241242.31189821186 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 170549.4035884853,
            "unit": "ns/iter",
            "extra": "iterations: 4180\ncpu: 170540.63444976063 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73294902.12498513,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73068958.12499991 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24017.222979915907,
            "unit": "ns/iter",
            "extra": "iterations: 29182\ncpu: 24016.45510931392 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 231533.46211837113,
            "unit": "ns/iter",
            "extra": "iterations: 3049\ncpu: 230555.940964238 ns\nthreads: 1"
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
          "id": "e591f54bfe52d019e5790fd374a21a9cf7379440",
          "message": "run megaboom regress on docker image (#381)",
          "timestamp": "2026-06-04T08:42:03+02:00",
          "tree_id": "019bc7cd91be5356472b139b8b33ae613a27086a",
          "url": "https://github.com/najaeda/naja/commit/e591f54bfe52d019e5790fd374a21a9cf7379440"
        },
        "date": 1780555469498,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 59651.79185172498,
            "unit": "ns/iter",
            "extra": "iterations: 11708\ncpu: 59649.48556542536 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 162587.87027954744,
            "unit": "ns/iter",
            "extra": "iterations: 4078\ncpu: 162578.20769985282 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1584511.9954955175,
            "unit": "ns/iter",
            "extra": "iterations: 444\ncpu: 1584153.4234234234 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17505475.92500027,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17481799.050000004 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1605.6964400256627,
            "unit": "ns/iter",
            "extra": "iterations: 436520\ncpu: 1605.566860624943 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15991.551091920288,
            "unit": "ns/iter",
            "extra": "iterations: 44234\ncpu: 15990.090021250613 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171502.60414129094,
            "unit": "ns/iter",
            "extra": "iterations: 4105\ncpu: 171494.43775883075 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 249894.59479686336,
            "unit": "ns/iter",
            "extra": "iterations: 2806\ncpu: 249868.611903065 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 205004.72291730394,
            "unit": "ns/iter",
            "extra": "iterations: 3277\ncpu: 204988.5950564541 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 145210.96969696757,
            "unit": "ns/iter",
            "extra": "iterations: 4818\ncpu: 145203.6307596514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72714737.12499699,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72714247.12499997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25960.182537324086,
            "unit": "ns/iter",
            "extra": "iterations: 26926\ncpu: 25959.038067295554 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 224432.7730175034,
            "unit": "ns/iter",
            "extra": "iterations: 3128\ncpu: 224384.39578003366 ns\nthreads: 1"
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
          "id": "eaba93f5efe0fc2f57d39466c45855ef9495a298",
          "message": "disable some parts of the workflows in forks when they can only work … (#382)\n\n* disable some parts of the workflows in forks when they can only work in central repo\n\n* protect forks against non runnable runs (upload package, ...)\n\n* add cva6 sim\n\n* clean regress\n\n* missing package\n\n* add local sim\n\n* add authorization\n\n* update sv regress\n\n* remove cva6 for the moment\n\n* add boost",
          "timestamp": "2026-06-05T12:16:08+02:00",
          "tree_id": "7b46b1614e7708772bde258c98f60553a963af87",
          "url": "https://github.com/najaeda/naja/commit/eaba93f5efe0fc2f57d39466c45855ef9495a298"
        },
        "date": 1780654717133,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 63937.58602150415,
            "unit": "ns/iter",
            "extra": "iterations: 11346\ncpu: 63925.08487572714 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 162234.6332331864,
            "unit": "ns/iter",
            "extra": "iterations: 4327\ncpu: 162199.4118326785 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1617285.693023246,
            "unit": "ns/iter",
            "extra": "iterations: 430\ncpu: 1616977.9953488375 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19930717.60526277,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 19929690.605263166 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1634.9100018599058,
            "unit": "ns/iter",
            "extra": "iterations: 430120\ncpu: 1634.8492583465072 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16053.053307053884,
            "unit": "ns/iter",
            "extra": "iterations: 43634\ncpu: 16051.93523399184 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 171883.2315412175,
            "unit": "ns/iter",
            "extra": "iterations: 4185\ncpu: 171868.3545997612 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 250661.20490621196,
            "unit": "ns/iter",
            "extra": "iterations: 2772\ncpu: 250639.4942279944 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 206491.4841199164,
            "unit": "ns/iter",
            "extra": "iterations: 3369\ncpu: 206476.2428020186 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 146916.18685702232,
            "unit": "ns/iter",
            "extra": "iterations: 4763\ncpu: 146809.78479949638 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73717483.12499803,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73710155.99999997 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25619.593191964337,
            "unit": "ns/iter",
            "extra": "iterations: 26880\ncpu: 25618.413988095264 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 225662.84896004616,
            "unit": "ns/iter",
            "extra": "iterations: 3125\ncpu: 225580.7552000175 ns\nthreads: 1"
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
          "id": "122ec5103f815f248a3179a52515fcf357898823",
          "message": "primitive FFs and Latches (#383)\n\n* primitive FFs and Latches\n\n* fix tests\n\n* reorganize db0\n\n* clean test\n\n* coverage",
          "timestamp": "2026-06-05T16:49:36+02:00",
          "tree_id": "bf5930873791a19ade9b4137b5790a78d5aa758b",
          "url": "https://github.com/najaeda/naja/commit/122ec5103f815f248a3179a52515fcf357898823"
        },
        "date": 1780671128091,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 103216.31660976334,
            "unit": "ns/iter",
            "extra": "iterations: 6737\ncpu: 103203.6302508535 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 210414.06988601582,
            "unit": "ns/iter",
            "extra": "iterations: 3334\ncpu: 210385.58728254348 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1700947.0782396535,
            "unit": "ns/iter",
            "extra": "iterations: 409\ncpu: 1700855.2396088017 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19748242.97222237,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19747350.30555556 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1627.9543117301719,
            "unit": "ns/iter",
            "extra": "iterations: 436217\ncpu: 1627.8033341204036 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15972.83524711744,
            "unit": "ns/iter",
            "extra": "iterations: 43805\ncpu: 15972.704029220416 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 170639.73168908985,
            "unit": "ns/iter",
            "extra": "iterations: 4014\ncpu: 170639.7097658197 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 297770.41972186934,
            "unit": "ns/iter",
            "extra": "iterations: 2373\ncpu: 297776.4707121784 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 251960.64549105373,
            "unit": "ns/iter",
            "extra": "iterations: 2739\ncpu: 251959.49726177458 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 190890.78401312948,
            "unit": "ns/iter",
            "extra": "iterations: 3653\ncpu: 190892.1226389271 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73730052.87499979,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73721957.00000006 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25660.032514052727,
            "unit": "ns/iter",
            "extra": "iterations: 27219\ncpu: 25660.47121496013 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 261905.02442731237,
            "unit": "ns/iter",
            "extra": "iterations: 2661\ncpu: 261810.26907180325 ns\nthreads: 1"
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
          "id": "c998ec4df1cccf0dfd5c76776ec5d38670bddd1e",
          "message": "optimize cva6 regress (#384)",
          "timestamp": "2026-06-07T12:41:01+02:00",
          "tree_id": "5543380799ac32bc21eab108c0faaa644b075fa9",
          "url": "https://github.com/najaeda/naja/commit/c998ec4df1cccf0dfd5c76776ec5d38670bddd1e"
        },
        "date": 1780829007290,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 101326.1105020221,
            "unit": "ns/iter",
            "extra": "iterations: 6932\ncpu: 101303.32775533758 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 203668.20985506583,
            "unit": "ns/iter",
            "extra": "iterations: 3450\ncpu: 203543.9611594203 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1680933.4463007313,
            "unit": "ns/iter",
            "extra": "iterations: 419\ncpu: 1680688.0095465397 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18998628.8648648,
            "unit": "ns/iter",
            "extra": "iterations: 37\ncpu: 18997295.837837838 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1610.2897355099633,
            "unit": "ns/iter",
            "extra": "iterations: 435404\ncpu: 1610.2525447630246 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 16320.242798353489,
            "unit": "ns/iter",
            "extra": "iterations: 44226\ncpu: 16319.903314792211 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169576.6309148254,
            "unit": "ns/iter",
            "extra": "iterations: 4121\ncpu: 169572.5535064306 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 297572.224152553,
            "unit": "ns/iter",
            "extra": "iterations: 2360\ncpu: 297558.84491525416 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 253597.17555313668,
            "unit": "ns/iter",
            "extra": "iterations: 2757\ncpu: 253579.92564381612 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 190491.6150068346,
            "unit": "ns/iter",
            "extra": "iterations: 3665\ncpu: 190482.42237380633 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 75159254.87500396,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 75155382.12499994 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25651.623926919103,
            "unit": "ns/iter",
            "extra": "iterations: 27258\ncpu: 25650.01383080195 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 262737.33545718365,
            "unit": "ns/iter",
            "extra": "iterations: 2668\ncpu: 262638.4047975958 ns\nthreads: 1"
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
          "id": "3a42cb4e3ecd872ae924e9920732a5dc64368e5b",
          "message": "compress registers - switch to 0.7.0 version (#385)",
          "timestamp": "2026-06-08T15:00:22+02:00",
          "tree_id": "f098713e1b828a200b79c84850ddc97166cd4589",
          "url": "https://github.com/najaeda/naja/commit/3a42cb4e3ecd872ae924e9920732a5dc64368e5b"
        },
        "date": 1780923784703,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 106270.4411983924,
            "unit": "ns/iter",
            "extra": "iterations: 6709\ncpu: 106212.71903413326 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 214272.46774192736,
            "unit": "ns/iter",
            "extra": "iterations: 3286\ncpu: 214269.2072428485 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1756650.5714284517,
            "unit": "ns/iter",
            "extra": "iterations: 399\ncpu: 1756649.2506265664 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 19002526.055557054,
            "unit": "ns/iter",
            "extra": "iterations: 36\ncpu: 19001972.22222223 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1495.1361539618217,
            "unit": "ns/iter",
            "extra": "iterations: 467390\ncpu: 1494.5212841524212 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14728.747530813032,
            "unit": "ns/iter",
            "extra": "iterations: 47384\ncpu: 14725.983791997294 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 200968.40126655647,
            "unit": "ns/iter",
            "extra": "iterations: 3474\ncpu: 200949.0374208407 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 336892.6458733549,
            "unit": "ns/iter",
            "extra": "iterations: 2084\ncpu: 336845.6684261034 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 290130.7712765723,
            "unit": "ns/iter",
            "extra": "iterations: 2444\ncpu: 290088.7565466447 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 211722.64426276943,
            "unit": "ns/iter",
            "extra": "iterations: 3303\ncpu: 211697.86527399332 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 73369667.5000033,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 73363046.37500013 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24041.81188119002,
            "unit": "ns/iter",
            "extra": "iterations: 28785\ncpu: 24037.873406288014 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 272368.869005806,
            "unit": "ns/iter",
            "extra": "iterations: 2565\ncpu: 272357.09239768254 ns\nthreads: 1"
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
          "id": "71bfd75d36570d5730c705229967aeb7647f66cc",
          "message": "package as a bundle (#386)\n\n* package as a bundle\n\n* fix path\n\n* verifications and cleaning\n\n* some cleaning\n\n* Fix\n\n* new cleaning",
          "timestamp": "2026-06-12T12:19:56+02:00",
          "tree_id": "552ad0bee1d47fb00593daa79693430e31446814",
          "url": "https://github.com/najaeda/naja/commit/71bfd75d36570d5730c705229967aeb7647f66cc"
        },
        "date": 1781259768052,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 105443.60389706724,
            "unit": "ns/iter",
            "extra": "iterations: 6723\ncpu: 105435.84917447568 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 205886.99143783952,
            "unit": "ns/iter",
            "extra": "iterations: 3387\ncpu: 205875.8727487452 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1725150.6938272158,
            "unit": "ns/iter",
            "extra": "iterations: 405\ncpu: 1725086.1580246913 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 17708660.449999057,
            "unit": "ns/iter",
            "extra": "iterations: 40\ncpu: 17708202.249999993 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1495.4904743190511,
            "unit": "ns/iter",
            "extra": "iterations: 468208\ncpu: 1495.4105333526982 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14614.530029873024,
            "unit": "ns/iter",
            "extra": "iterations: 47869\ncpu: 14613.61054126889 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 206558.4345612515,
            "unit": "ns/iter",
            "extra": "iterations: 3339\ncpu: 206544.38993710701 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 333310.6010485989,
            "unit": "ns/iter",
            "extra": "iterations: 2098\ncpu: 333280.6310772164 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 284007.37581170164,
            "unit": "ns/iter",
            "extra": "iterations: 2464\ncpu: 283979.4115259744 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 209685.6974789808,
            "unit": "ns/iter",
            "extra": "iterations: 3332\ncpu: 209672.78151260514 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 72250374.12499802,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 72242392.62499999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24281.622970117187,
            "unit": "ns/iter",
            "extra": "iterations: 28881\ncpu: 24279.625947854984 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 266055.73038834735,
            "unit": "ns/iter",
            "extra": "iterations: 2626\ncpu: 265959.9485910406 ns\nthreads: 1"
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
          "id": "e09c942c2c8a24406bd4f0ba949220a82a5f9fc4",
          "message": "Log cleaning (#387)",
          "timestamp": "2026-06-13T08:07:03+02:00",
          "tree_id": "a4f67d6ba6074ae331f894bd9b8ddce2c03c0335",
          "url": "https://github.com/najaeda/naja/commit/e09c942c2c8a24406bd4f0ba949220a82a5f9fc4"
        },
        "date": 1781330976891,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 102215.39283625752,
            "unit": "ns/iter",
            "extra": "iterations: 6840\ncpu: 102209.68435672516 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 205233.23990637698,
            "unit": "ns/iter",
            "extra": "iterations: 3418\ncpu: 205214.48303101235 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1669385.070388357,
            "unit": "ns/iter",
            "extra": "iterations: 412\ncpu: 1669258.2791262127 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18447750.499999557,
            "unit": "ns/iter",
            "extra": "iterations: 38\ncpu: 18445404.02631578 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1609.7697850394186,
            "unit": "ns/iter",
            "extra": "iterations: 431614\ncpu: 1609.508454313346 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 15840.817178024785,
            "unit": "ns/iter",
            "extra": "iterations: 44196\ncpu: 15835.777604308072 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 169964.80919426825,
            "unit": "ns/iter",
            "extra": "iterations: 4046\ncpu: 169930.25160652498 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 292459.74070981244,
            "unit": "ns/iter",
            "extra": "iterations: 2395\ncpu: 292419.99248434225 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 250813.52712049257,
            "unit": "ns/iter",
            "extra": "iterations: 2747\ncpu: 250793.5547870408 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 189070.80735930585,
            "unit": "ns/iter",
            "extra": "iterations: 3696\ncpu: 189052.56331168834 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 74608974.3749998,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 74610041.99999999 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 25818.194519036024,
            "unit": "ns/iter",
            "extra": "iterations: 27185\ncpu: 25815.530513150603 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 264740.62386706774,
            "unit": "ns/iter",
            "extra": "iterations: 2648\ncpu: 264650.8674470895 ns\nthreads: 1"
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
          "id": "7a6775acacdded3892cb0628d4cb157074173e0e",
          "message": "Liberty filtering (#388)",
          "timestamp": "2026-06-13T11:14:11+02:00",
          "tree_id": "2fa4a6a1e6264ec93fe2935c98c1827f7aca43e3",
          "url": "https://github.com/najaeda/naja/commit/7a6775acacdded3892cb0628d4cb157074173e0e"
        },
        "date": 1781342186691,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "BM_CreateNetlist0",
            "value": 104100.77748650937,
            "unit": "ns/iter",
            "extra": "iterations: 6485\ncpu: 104084.47124132614 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/100",
            "value": 207708.4802962485,
            "unit": "ns/iter",
            "extra": "iterations: 3375\ncpu: 207665.48681481488 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/1000",
            "value": 1745754.321782082,
            "unit": "ns/iter",
            "extra": "iterations: 404\ncpu: 1743277.8638613871 ns\nthreads: 1"
          },
          {
            "name": "BM_CreateInstances/10000",
            "value": 18001762.12820285,
            "unit": "ns/iter",
            "extra": "iterations: 39\ncpu: 18001490.307692308 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/100",
            "value": 1500.7030678155277,
            "unit": "ns/iter",
            "extra": "iterations: 478712\ncpu: 1500.6601359481263 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/1000",
            "value": 14607.765352908094,
            "unit": "ns/iter",
            "extra": "iterations: 48639\ncpu: 14606.225518616746 ns\nthreads: 1"
          },
          {
            "name": "BM_TraversalInstances/10000",
            "value": 204653.49264703714,
            "unit": "ns/iter",
            "extra": "iterations: 3536\ncpu: 204634.3885746608 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates0",
            "value": 335571.278618688,
            "unit": "ns/iter",
            "extra": "iterations: 2114\ncpu: 335569.29328287597 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/FullAdder",
            "value": 294091.53758923896,
            "unit": "ns/iter",
            "extra": "iterations: 2381\ncpu: 294083.27005459863 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/Gates2",
            "value": 213828.99726692637,
            "unit": "ns/iter",
            "extra": "iterations: 3293\ncpu: 213827.7102945638 ns\nthreads: 1"
          },
          {
            "name": "BM_LoadVerilogFile/LargeHierGates",
            "value": 69895318.3749893,
            "unit": "ns/iter",
            "extra": "iterations: 8\ncpu: 69891582.25000013 ns\nthreads: 1"
          },
          {
            "name": "BM_HierarchyTraversal",
            "value": 24156.83827657623,
            "unit": "ns/iter",
            "extra": "iterations: 29569\ncpu: 24156.056951537103 ns\nthreads: 1"
          },
          {
            "name": "BM_CapnPSerialize",
            "value": 270505.2868672522,
            "unit": "ns/iter",
            "extra": "iterations: 2604\ncpu: 270411.6524577748 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}