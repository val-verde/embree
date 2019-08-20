// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //
#pragma once

#if defined(EMBREE_DPCPP_SUPPORT)
#include "../gpu/common.h"
#include "AABB.h"
#include "AABB3f.h"

#define BUILDRECORD_STACK_SIZE 64
#define BINS 16

#define BVH_LEAF_MASK       8
#define BVH_INVALID_NODE    3
#define BVH_NODE_N           16
#define BVH_NODE_N_LOG       4
#define SAH_LOG_BLOCK_SHIFT  2
#define BVH_LEAF_N_MIN       4
#define BVH_LEAF_N_MAX       4


namespace embree
{
  namespace gpu
  {
    
    struct BVHBase
    {
      unsigned long rootNodeOffset; 
      AABB3f bounds;

      unsigned int nodeDataStart;
      unsigned int nodeDataCur;
      unsigned int leafDataStart;
      unsigned int leafDataCur;
      unsigned int proceduralDataStart;
      unsigned int proceduralDataCur;
      unsigned int backPointerDataStart;
      unsigned int backPointerDataEnd;
    };
    
    struct Globals
    {
      /* 1. cacheline */
      struct AABB geometryBounds;
      struct AABB centroidBounds;

      /* 2. cacheline */
      unsigned int node_mem_allocator_start;
      unsigned int node_mem_allocator_cur;
      unsigned int node_mem_allocator_pad[14];

      /* 3. cacheline */
      unsigned int leaf_mem_allocator_start;
      unsigned int leaf_mem_allocator_cur;
      unsigned int procedural_mem_allocator_start;
      unsigned int procedural_mem_allocator_cur;
      unsigned int back_pointer_start;
      unsigned int leaf_mem_allocator[11];

      /* 4. cacheline */
      unsigned int numPrimitives;
      unsigned int numOriginalPrimitives;
      unsigned int numSplittedPrimitives;
      unsigned int init_numPrimitives;
      unsigned int leafPrimType;
      unsigned int leafSize;
      unsigned int numBuildRecords;
      unsigned int numBuildRecords_extended;
      unsigned int totalAllocatedMem;
      float presplitPrioritySum;

      unsigned int sync;
      float probThreshold;
      unsigned int numGlobalBuildRecords;
      unsigned int numGlobalBinaryNodes;
      unsigned int counter;

      /* morton code builder state */
      unsigned int shift;                // used by adaptive mc-builder
      unsigned int shift_mask;           // used by adaptive mc-builder
      unsigned int binary_hierarchy_root;

      inline void init(char *bvh_mem,
		       unsigned int _numPrimitives,
		       unsigned int _node_data_start,
		       unsigned int _leaf_data_start,
		       unsigned int _procedural_data_start,
		       unsigned int _back_pointer_start,
		       unsigned int _totalBytes,
		       unsigned int _leafPrimType,
		       unsigned int _leafSize)
      {
	struct BVHBase *base       = (struct BVHBase*)bvh_mem;
	base->nodeDataStart        = _node_data_start/64;
	base->nodeDataCur          = _node_data_start/64;
	base->leafDataStart        = _leaf_data_start/64;
	base->leafDataCur          = _leaf_data_start/64;
	base->proceduralDataStart  = _procedural_data_start/64;
	base->proceduralDataCur    = _procedural_data_start/64;
	base->backPointerDataStart = _back_pointer_start/64;
	base->backPointerDataEnd   = _totalBytes/64;
	base->rootNodeOffset       = _node_data_start; // FIXME: should be set by builder

	geometryBounds.init();
	centroidBounds.init();

	node_mem_allocator_cur         = _node_data_start;
	node_mem_allocator_start       = _node_data_start;
	leaf_mem_allocator_cur         = _leaf_data_start;
	leaf_mem_allocator_start       = _leaf_data_start;
	procedural_mem_allocator_cur   = _procedural_data_start;
	procedural_mem_allocator_start = _procedural_data_start;
	back_pointer_start             = _back_pointer_start;

	numBuildRecords          = 0;
	numBuildRecords_extended = 0;
	numPrimitives            = _numPrimitives;
	init_numPrimitives       = 0;
	numSplittedPrimitives    = 0;
	totalAllocatedMem        = _totalBytes;
	sync                     = 0;
	probThreshold            = 0.0f;
	leafPrimType             = _leafPrimType;
	leafSize                 = _leafSize;
	numGlobalBuildRecords    = 0;
      }

      inline void init(char *bvh_mem,
		       unsigned int _numPrimitives,
		       unsigned int _node_data_start,
		       unsigned int _leaf_data_start,
		       unsigned int _totalBytes)
      {
	init(bvh_mem,_numPrimitives,_node_data_start,_leaf_data_start,0,0,_totalBytes,0,64);
      }
      
      inline void resetGlobalCounters()
      {
	node_mem_allocator_cur = node_mem_allocator_start;
	leaf_mem_allocator_cur = leaf_mem_allocator_start;
	numBuildRecords = 0;	
      }
      
      inline uint alloc_node_mem(const uint size)
      {
	const uint aligned_size = ((size+63)/64)*64; /* allocate in 64 bytes blocks */
	cl::sycl::multi_ptr<unsigned int,cl::sycl::access::address_space::global_space> ptr(&node_mem_allocator_cur);
	cl::sycl::atomic<unsigned int> counter(ptr);
	return atomic_fetch_add(counter,aligned_size);
      }

      inline uint alloc_leaf_mem(const uint size)
      {
	const uint aligned_size = ((size+63)/64)*64; /* allocate in 64 bytes blocks */
	cl::sycl::multi_ptr<unsigned int,cl::sycl::access::address_space::global_space> ptr(&leaf_mem_allocator_cur);
	cl::sycl::atomic<unsigned int> counter(ptr);
	return atomic_fetch_add(counter,aligned_size);
      }
      
    };

    struct Range {
      unsigned int start, end;
    };

    struct Triangle
    {
      unsigned int vtx[3];
      //unsigned int primID;
      //unsigned int geomID;
    };

    struct MortonCodePrimitive
    {
      uint64_t index_code; // 64bit code + index combo
    };

    struct BuildRecord
    {
      struct AABB centroidBounds;
      unsigned int start, end;
      uint *parent;

      inline void init(unsigned int _start, unsigned int _end, AABB &bounds)
      {
	centroidBounds = bounds;
	start = _start;
	end   = _end;
	parent = NULL;	
      }

      inline void init(unsigned int _start, unsigned int _end)
      {
	centroidBounds.init();
	start = _start;
	end   = _end;
	parent = NULL;	
      }
      

      inline void extend(AABB &primref)
      {
	centroidBounds.extend(primref.centroid2());
      }

      inline unsigned int size() const { return end - start; }
    };

    inline const cl::sycl::stream &operator<<(const cl::sycl::stream &out, const BuildRecord& r) {
      return out << " start " << r.start << " end " << r.end << " size " << r.size() << " parent " << r.parent << " bounds " << r.centroidBounds;
    }


    struct BinaryMortonCodeHierarchy
    {
      struct Range range;
      unsigned int leftChild;
      unsigned int rightChild;
      unsigned int flag;
    };

    struct StatStackEntry
    {
      struct AABB aabb;
      unsigned int node;
      unsigned int type;
      unsigned int depth;
      float area;
    };

    struct BuildRecordMorton {
      unsigned int nodeID;
      unsigned int items;
      unsigned int current_index;
      unsigned int parent_index;
    };

    struct Split
    {
      float sah;
      int dim;
      int pos;
    };

    inline const cl::sycl::stream &operator<<(const cl::sycl::stream &out, const Split& s) {
      return out << " sah " << s.sah << " dim " << s.dim << " pos " << s.pos;
    }


    struct BinMapping
    {
      cl::sycl::float4 ofs, scale;

      inline void init(const AABB &centBounds, const uint bins)
      {
	const cl::sycl::float4 eps(1E-34f);
	const cl::sycl::float4 diag = max(eps, centBounds.upper - centBounds.lower);
	scale = (cl::sycl::float4)(0.99f*(float)bins)/diag;
	scale = cl::sycl::select((cl::sycl::float4)(0.0f), scale, (diag > eps));
	ofs  = centBounds.lower;
      }

    };

    inline const cl::sycl::stream &operator<<(const cl::sycl::stream &out, const BinMapping& bm) {
      return out << "ofs " << bm.ofs.xyz() << " scale " << bm.scale.xyz();
    }


    inline AABB3f convert_AABB3f(const AABB &aabb)
    {
      AABB3f aabb3f;
      aabb3f.lower = aabb.lower.xyz();
      aabb3f.upper = aabb.upper.xyz();
      
      aabb3f.lower.x() = aabb.lower.x();
      aabb3f.lower.y() = aabb.lower.y();
      aabb3f.lower.z() = aabb.lower.z();      
      aabb3f.upper.x() = aabb.upper.x();
      aabb3f.upper.y() = aabb.upper.y();
      aabb3f.upper.z() = aabb.upper.z();
      return aabb3f;
    }
    
    struct BinInfo {
      AABB3f boundsX[BINS];
      AABB3f boundsY[BINS];
      AABB3f boundsZ[BINS];
      cl::sycl::uint3 counts[BINS];

      inline void init()
      {
	for (uint i=0;i<BINS;i++)
	  {
	    boundsX[i].init();
	    boundsY[i].init();
	    boundsZ[i].init();
	    counts[i] = (cl::sycl::uint3)(0);
	  }	
      }

      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline void init(const cl::sycl::intel::sub_group &subgroup)
      {
	for (uint i=subgroup.get_local_id()[0];i<BINS;i+=subgroup.get_local_range().size())
	  {
	    boundsX[i].init();
	    boundsY[i].init();
	    boundsZ[i].init();
	    counts[i] = (cl::sycl::uint3)(0);
	  }
      }


      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline float left_to_right_area16(const cl::sycl::intel::sub_group &sg, const AABB3f &low)
      {
	struct AABB3f low_prefix = low.sub_group_scan_exclusive_min_max(sg);
	return low_prefix.halfArea();
      }

      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline uint left_to_right_counts16(const cl::sycl::intel::sub_group &sg, uint low)
      {
	return sg.exclusive_scan<uint,cl::sycl::intel::plus>(low);
      }


      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline float right_to_left_area16(const cl::sycl::intel::sub_group &sg, const AABB3f &low)
      {
	const uint subgroupLocalID = sg.get_local_id()[0];
	const uint subgroupSize    = sg.get_local_range().size();	
	const uint ID              = subgroupSize - 1 - subgroupLocalID;  
	AABB3f low_reverse         = low.sub_group_broadcast(sg,ID);
	AABB3f low_prefix          = low_reverse.sub_group_scan_inclusive_min_max(sg);
	const float low_area       = sg.broadcast<float>(low_prefix.halfArea(),ID);
	return low_area;
      }

      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline uint right_to_left_counts16(const cl::sycl::intel::sub_group &sg, uint low)
      {
	const uint subgroupLocalID = sg.get_local_id()[0];
	const uint subgroupSize    = sg.get_local_range().size();	
	const uint ID              = subgroupSize - 1 - subgroupLocalID;  
	const uint low_reverse     = sg.broadcast<uint>(low,ID);
	const uint low_prefix      = sg.inclusive_scan<float,cl::sycl::intel::plus>(low_reverse);
	return sg.broadcast<uint>(low_prefix,ID);
      }

      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline ulong getBestSplit(cl::sycl::intel::sub_group &sg, const cl::sycl::float3 sah, uint ID, const cl::sycl::float4 scale, const ulong defaultSplit)
      {
	ulong splitX = (((ulong)as_uint((float)sah.x())) << 32) | ((uint)ID << 2) | 0;
	ulong splitY = (((ulong)as_uint((float)sah.y())) << 32) | ((uint)ID << 2) | 1;
	ulong splitZ = (((ulong)as_uint((float)sah.z())) << 32) | ((uint)ID << 2) | 2;
	
	
	/* ignore zero sized dimensions */
	splitX = cl::sycl::select( splitX, defaultSplit, (ulong)(scale.x() == 0));
	splitY = cl::sycl::select( splitY, defaultSplit, (ulong)(scale.y() == 0));
	splitZ = cl::sycl::select( splitZ, defaultSplit, (ulong)(scale.z() == 0));
	ulong bestSplit = min(min(splitX,splitY),splitZ);
	bestSplit = sg.reduce<ulong,cl::sycl::intel::minimum>(bestSplit);
	return bestSplit;
      }

      
      [[cl::intel_reqd_sub_group_size(BVH_NODE_N)]] inline gpu::Split reduceBinsAndComputeBestSplit16(cl::sycl::intel::sub_group &sg, const cl::sycl::float4 scale, const uint startID, const uint endID,const cl::sycl::stream &out)
      {
	gpu::Split split;	
	
	const uint subgroupLocalID = sg.get_local_id()[0];
	const uint subgroupSize    = sg.get_local_range().size();
	
	const AABB3f &bX      = boundsX[subgroupLocalID];
	const float lr_areaX  = left_to_right_area16(sg,bX);	
	const float rl_areaX  = right_to_left_area16(sg,bX);
	const AABB3f &bY      = boundsY[subgroupLocalID];
	const float lr_areaY  = left_to_right_area16(sg,bY);
	const float rl_areaY  = right_to_left_area16(sg,bY);
	const AABB3f &bZ      = boundsZ[subgroupLocalID];
	const float lr_areaZ  = left_to_right_area16(sg,bZ);
	const float rl_areaZ  = right_to_left_area16(sg,bZ);
	const cl::sycl::uint3 &c = counts[subgroupLocalID];
	const uint lr_countsX = left_to_right_counts16(sg,c.x());
	const uint rl_countsX = right_to_left_counts16(sg,c.x());
	const uint lr_countsY = left_to_right_counts16(sg,c.y());
	const uint rl_countsY = right_to_left_counts16(sg,c.y());  
	const uint lr_countsZ = left_to_right_counts16(sg,c.z());
	const uint rl_countsZ = right_to_left_counts16(sg,c.z());
	
	const uint blocks_shift = SAH_LOG_BLOCK_SHIFT;       
	const uint blocks_add = ((1 << blocks_shift)-1);
	
	const cl::sycl::uint3 lr_count = { (lr_countsX+blocks_add)>>blocks_shift , (lr_countsY+blocks_add)>>blocks_shift, (lr_countsZ+blocks_add)>>blocks_shift };
	
#if 1
	for (uint i=0;i<subgroupSize;i++)
	  if (i == subgroupLocalID)	
	    out << "i " << i << " lr_countsX " << lr_countsX << " wrong lr_count.x() " <<  lr_count.x() << cl::sycl::endl;
	    
#endif

	const cl::sycl::float3 lr_area = { lr_areaX,lr_areaY,lr_areaZ };
	const cl::sycl::float3 rl_area = { rl_areaX,rl_areaY,rl_areaZ };

	//out << "i " << i << " " << min(lr_count_f,cl::sycl::float3(MAXFLOAT)) << " " << cl::sycl::endl; 
	
	const cl::sycl::uint3 rl_count = { (rl_countsX+blocks_add)>>blocks_shift , (rl_countsY+blocks_add)>>blocks_shift, (rl_countsZ+blocks_add)>>blocks_shift };
	const cl::sycl::float3 lr_count_f = { (float)lr_count.x(),(float)lr_count.y(),(float)lr_count.z() };
	const cl::sycl::float3 rl_count_f = { (float)rl_count.x(),(float)rl_count.y(),(float)rl_count.z() };	
	
	cl::sycl::float3 sah = cl::sycl::fma(lr_area,lr_count_f,rl_area*rl_count_f);
       
	/* first bin is invalid */
	const float pos_inf = (float)INFINITY;

	// FIXME: select has issues with data types
	sah.x() = cl::sycl::select( (float)pos_inf, (float)sah.x(), (int)(subgroupLocalID != 0));
	sah.y() = cl::sycl::select( (float)pos_inf, (float)sah.y(), (int)(subgroupLocalID != 0));
	sah.z() = cl::sycl::select( (float)pos_inf, (float)sah.z(), (int)(subgroupLocalID != 0));

	//out << (uint)__builtin_bit_cast(uint,tmp) << " " << cl::sycl::endl;
	
	const uint mid = (startID+endID)/2;

	const uint maxSAH = __builtin_bit_cast(uint,pos_inf);	
	const ulong defaultSplit = (((ulong)maxSAH) << 32) | ((uint)mid << 2) | 0;    
	const ulong bestSplit = getBestSplit(sg, sah, subgroupLocalID, scale, defaultSplit);
	const uint bestSplit32 = (uint)(bestSplit >> 32);
	const float split_sah = __builtin_bit_cast(float,bestSplit32);	

	split.sah = split_sah;
	split.dim = (uint)bestSplit & 3;
	split.pos = (uint)bestSplit >> 2;
	return split;
      }
      
      
    };

    struct BinInfo2 {
      struct AABB3f boundsX[BINS*2];
      struct AABB3f boundsY[BINS*2];
      struct AABB3f boundsZ[BINS*2];
      cl::sycl::uint3 counts[BINS*2];
    };

    inline uint encodeOffset(char *bvh_mem, uint *parent, uint global_child_offset)
    {
      ulong global_parent_offset = (ulong)parent - (ulong)bvh_mem;
      global_parent_offset = global_parent_offset & (~(64-1));
      uint relative_offset = global_child_offset - global_parent_offset;
      return relative_offset;
    }

    inline uint createLeaf(const Globals &globals,		 
			   const uint start,
			   const uint items,
			   const uint stride)
    {
      const uint offset = globals.leaf_mem_allocator[1] + start * stride;
      const unsigned int final = offset | BVH_LEAF_MASK | (items-1);
      return final;
    }

    inline uint createNode(cl::sycl::intel::sub_group &sg, Globals &globals, const uint ID, struct AABB *childrenAABB, uint numChildren, char *bvh_mem)
    {
#if 0      
      const uint subgroupLocalID = get_sub_group_local_id();

      uint node_offset = 0;
      if (subgroupLocalID == 0)
	node_offset = alloc_node_mem(globals,sizeof(struct BVHNodeN));
      node_offset = sub_group_broadcast(node_offset,0);
	  
      global struct BVHNodeN *node = (global struct BVHNodeN*)(bvh_mem + node_offset);

      if (subgroupLocalID < numChildren)
	setBVHNodeN(node,&childrenAABB[ID],subgroupLocalID);
  
      if (subgroupLocalID >= numChildren && subgroupLocalID < BVH_NODE_N)
	initBVHNodeN(node,subgroupLocalID);	      

      return node_offset;
#else
      return 0; // FIXME
#endif      
    }
    
    struct Quad1
    {
      cl::sycl::float4 v0,v2,v1,v3; //v1v3 loaded once
    };


    /* ======================================================================== */
    /* ============================== BVH NODES =============================== */
    /* ======================================================================== */

    struct BVHNodeN
    {              
      uint offset[BVH_NODE_N];  
      uint parent[BVH_NODE_N]; 
      float lower_x[BVH_NODE_N]; 
      float upper_x[BVH_NODE_N]; 
      float lower_y[BVH_NODE_N]; 
      float upper_y[BVH_NODE_N]; 
      float lower_z[BVH_NODE_N]; 
      float upper_z[BVH_NODE_N]; 

      inline void initBVHNodeN(uint slotID)
      {
	const float pos_inf =  INFINITY;
	const float neg_inf = -INFINITY;	
	offset[slotID]  =  (uint)(-1);  
	parent[slotID]  =  (uint)(-1); 
	lower_x[slotID] =  pos_inf; 
	upper_x[slotID] =  neg_inf;
	lower_y[slotID] =  pos_inf; 
	upper_y[slotID] =  neg_inf;
	lower_z[slotID] =  pos_inf; 
	upper_z[slotID] =  neg_inf;  
      }


      inline void setBVHNodeN(const struct AABB &aabb, uint slot)
      {
	lower_x[slot] = aabb.lower.x();
	lower_y[slot] = aabb.lower.y();
	lower_z[slot] = aabb.lower.z();
	upper_x[slot] = aabb.upper.x();
	upper_y[slot] = aabb.upper.y();
	upper_z[slot] = aabb.upper.z();
      }

      inline void setBVHNodeN_offset(const struct AABB &aabb, const uint _offset, const uint _parent, uint slot)
      {
	offset[slot] = _offset;
	parent[slot] = _parent;  
	lower_x[slot] = aabb.lower.x();
	lower_y[slot] = aabb.lower.y();
	lower_z[slot] = aabb.lower.z();
	upper_x[slot] = aabb.upper.x();
	upper_y[slot] = aabb.upper.y();
	upper_z[slot] = aabb.upper.z();
      }


    };

    inline const cl::sycl::stream &operator<<(const cl::sycl::stream &out, const BVHNodeN& node) {
      for (uint i=0;i<BVH_NODE_N;i++)
	{
	  out << " i " << i << " offset " << node.offset[i] << " lower_x " << node.lower_x[i] << " upper_x " << node.upper_x[i] << " lower_y " << node.lower_y[i] << " upper_y " << node.upper_y[i] << " lower_z " << node.lower_z[i] << " upper_z " << node.upper_z[i];
	}      
      return out; 
    }

    
  };
};

#endif
