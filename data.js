window.BENCHMARK_DATA = {
  "lastUpdate": 1769187364302,
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
      }
    ]
  }
}