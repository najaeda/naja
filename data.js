window.BENCHMARK_DATA = {
  "lastUpdate": 1771540639598,
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
      }
    ]
  }
}