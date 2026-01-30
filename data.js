window.BENCHMARK_DATA = {
  "lastUpdate": 1769816692731,
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
      }
    ]
  }
}