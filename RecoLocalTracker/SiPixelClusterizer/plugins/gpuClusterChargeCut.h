#ifndef RecoLocalTracker_SiPixelClusterizer_plugins_gpuClusterChargeCut_h
#define RecoLocalTracker_SiPixelClusterizer_plugins_gpuClusterChargeCut_h

#include <cstdint>
#include <cstdio>

#include "CUDADataFormats/SiPixelCluster/interface/gpuClusteringConstants.h"
#include "Geometry/TrackerGeometryBuilder/interface/pixelTopology.h"
#include "HeterogeneousCore/CUDAUtilities/interface/cuda_assert.h"
#include "HeterogeneousCore/CUDAUtilities/interface/prefixScan.h"

// local include(s)
#include "SiPixelClusterThresholds.h"

namespace gpuClustering {

  template <bool isPhase2>
  __global__ void clusterChargeCut(
      SiPixelClusterThresholds
          clusterThresholds,             // charge cut on cluster in electrons (for layer 1 and for other layers)
      uint16_t* __restrict__ id,         // module id of each pixel (modified if bad cluster)
      uint16_t const* __restrict__ adc,  //  charge of each pixel
      uint32_t const* __restrict__ moduleStart,  // index of the first pixel of each module
      uint32_t* __restrict__ nClustersInModule,  // modified: number of clusters found in each module
      uint32_t const* __restrict__ moduleId,     // module id of each module
      int32_t* __restrict__ clusterId,           // modified: cluster id of each pixel
      uint32_t numElements) {
    __shared__ int32_t charge[maxNumClustersPerModules];
    __shared__ uint8_t ok[maxNumClustersPerModules];
    __shared__ uint16_t newclusId[maxNumClustersPerModules];

    constexpr int startBPIX2 = isPhase2 ? phase2PixelTopology::layerStart[1] : phase1PixelTopology::layerStart[1];
    constexpr int nMaxModules = isPhase2 ? phase2PixelTopology::numberOfModules : phase1PixelTopology::numberOfModules;

    assert(nMaxModules < maxNumModules);
    assert(startBPIX2 < nMaxModules);

    auto firstModule = blockIdx.x;
    auto endModule = moduleStart[0];
    for (auto module = firstModule; module < endModule; module += gridDim.x) {
      auto firstPixel = moduleStart[1 + module];
      auto thisModuleId = id[firstPixel];
      assert(thisModuleId < nMaxModules);
      assert(thisModuleId == moduleId[module]);

      auto nclus = nClustersInModule[thisModuleId];
      if (nclus == 0)
        continue;

      if (threadIdx.x == 0 && nclus > maxNumClustersPerModules)
        printf("Warning too many clusters in module %d in block %d: %d > %d\n",
               thisModuleId,
               blockIdx.x,
               nclus,
               maxNumClustersPerModules);

      auto first = firstPixel + threadIdx.x;

      if (nclus > maxNumClustersPerModules) {
        // remove excess  FIXME find a way to cut charge first....
        for (auto i = first; i < numElements; i += blockDim.x) {
          if (id[i] == invalidModuleId)
            continue;  // not valid
          if (id[i] != thisModuleId)
            break;  // end of module
          if (clusterId[i] >= maxNumClustersPerModules) {
            id[i] = invalidModuleId;
            clusterId[i] = invalidModuleId;
          }
        }
        nclus = maxNumClustersPerModules;
      }

#ifdef GPU_DEBUG
      if (thisModuleId % 100 == 1)
        if (threadIdx.x == 0)
          printf("start cluster charge cut for module %d in block %d\n", thisModuleId, blockIdx.x);
#endif

      assert(nclus <= maxNumClustersPerModules);
      for (auto i = threadIdx.x; i < nclus; i += blockDim.x) {
        charge[i] = 0;
      }
      __syncthreads();

      for (auto i = first; i < numElements; i += blockDim.x) {
        if (id[i] == invalidModuleId)
          continue;  // not valid
        if (id[i] != thisModuleId)
          break;  // end of module
        atomicAdd(&charge[clusterId[i]], adc[i]);
      }
      __syncthreads();

      auto chargeCut = clusterThresholds.getThresholdForLayerOnCondition(thisModuleId < startBPIX2);

      bool good = true;
      for (auto i = threadIdx.x; i < nclus; i += blockDim.x) {
        newclusId[i] = ok[i] = charge[i] >= chargeCut ? 1 : 0;
        if (0 == ok[i])
          good = false;
      }

      // if all clusters above threshold do nothing
      if (__syncthreads_and(good))
        continue;

      // renumber
      __shared__ uint16_t ws[32];
      cms::cuda::blockPrefixScan(newclusId, nclus, ws);

      assert(nclus > newclusId[nclus - 1]);

      nClustersInModule[thisModuleId] = newclusId[nclus - 1];

      // reassign id
      for (auto i = first; i < numElements; i += blockDim.x) {
        if (id[i] == invalidModuleId)
          continue;  // not valid
        if (id[i] != thisModuleId)
          break;  // end of module
        if (0 == ok[clusterId[i]])
          clusterId[i] = id[i] = invalidModuleId;
        else
          clusterId[i] = newclusId[clusterId[i]] - 1;
      }

      //done
      __syncthreads();
    }  // loop on modules
  }

}  // namespace gpuClustering

#endif  // RecoLocalTracker_SiPixelClusterizer_plugins_gpuClusterChargeCut_h
