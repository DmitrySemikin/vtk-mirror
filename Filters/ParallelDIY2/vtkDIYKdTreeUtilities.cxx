/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYKdTreeUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYKdTreeUtilities.h"

#include "vtkAppendFilter.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkDIYUtilities.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTuple.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <map>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/algorithms.hpp)
// clang-format on
namespace
{
struct PointTT
{
  vtkTuple<float, 3> Coords;
  // int Rank;
  // int DatasetIndex;
  // vtkIdType PointIndex;
  float operator[](unsigned int idx) const { return static_cast<float>(this->Coords[idx]); }
};

struct BlockT
{
  std::vector<PointTT> Points;
  std::vector<diy::ContinuousBounds> BlockBounds;

  void AddPoints(int rank, int pts_index, vtkPoints* pts)
  {
    const auto start_offset = this->Points.size();
    this->Points.resize(start_offset + pts->GetNumberOfPoints());

    // FIXME: make this fast & use array dispatch
    for (vtkIdType cc = 0, max = pts->GetNumberOfPoints(); cc < max; ++cc)
    {
      auto& pt = this->Points[cc + start_offset];
      // pt.Rank = rank;
      // pt.DatasetIndex = pts_index;
      // pt.PointIndex = cc;
      const double* coord = pts->GetPoint(cc);
      pt.Coords[0] = static_cast<float>(coord[0]);
      pt.Coords[1] = static_cast<float>(coord[1]);
      pt.Coords[2] = static_cast<float>(coord[2]);
    }
  }
};

unsigned int nextPowerOf2(unsigned int n)
{
  unsigned int count = 0;
  if (n <= 1)
  {
    return 2;
  }

  if (!(n & (n - 1)))
  {
    return n;
  }
  while (n != 0)
  {
    n >>= 1;
    count += 1;
  }
  return (1 << count);
}
}

//----------------------------------------------------------------------------
vtkDIYKdTreeUtilities::vtkDIYKdTreeUtilities()
{
}

//----------------------------------------------------------------------------
vtkDIYKdTreeUtilities::~vtkDIYKdTreeUtilities()
{
}

//----------------------------------------------------------------------------
void vtkDIYKdTreeUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
std::vector<vtkBoundingBox> vtkDIYKdTreeUtilities::GenerateCuts(
  const std::vector<vtkSmartPointer<vtkPoints> >& points, int number_of_partitions,
  vtkMultiProcessController* controller, const double* local_bounds /*=nullptr*/)
{
  if (number_of_partitions == 0)
  {
    return std::vector<vtkBoundingBox>();
  }

  // communicate global bounds and number of blocks.
  vtkBoundingBox bbox;
  if (local_bounds != nullptr)
  {
    bbox.SetBounds(local_bounds);
  }
  if (!bbox.IsValid())
  {
    for (auto& pts : points)
    {
      double bds[6];
      pts->GetBounds(bds);
      bbox.AddBounds(bds);
    }
  }

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);

  // determine global domain bounds.
  vtkDIYUtilities::AllReduce(comm, bbox);

  if (!bbox.IsValid())
  {
    // nothing to split since global bounds are empty.
    return std::vector<vtkBoundingBox>();
  }

  if (number_of_partitions == 1)
  {
    return std::vector<vtkBoundingBox>{ bbox };
  }

  bbox.Inflate(0.1 * bbox.GetDiagonalLength());

  const int num_cuts = ::nextPowerOf2(number_of_partitions);
  if (num_cuts < comm.size())
  {
    // TODO: we need a MxN transfer
    vtkLogF(WARNING,
      "Requested cuts (%d) is less than number of ranks (%d), "
      "current implementation may not load balance correctly.",
      num_cuts, comm.size());
  }

  diy::Master master(comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });

  const diy::ContinuousBounds gdomain = vtkDIYUtilities::Convert(bbox);

  diy::ContiguousAssigner cuts_assigner(comm.size(), num_cuts);

  std::vector<int> gids;
  cuts_assigner.local_gids(comm.rank(), gids);
  for (const int gid : gids)
  {
    auto block = new BlockT();
    if (gid == gids[0])
    {
      for (size_t idx = 0; idx < points.size(); ++idx)
      {
        block->AddPoints(comm.rank(), idx, points[idx]);
      }
    }
    auto link = new diy::RegularContinuousLink(3, gdomain, gdomain);
    master.add(gid, block, link);
  }

  diy::kdtree(master, cuts_assigner, 3, gdomain, &BlockT::Points, /*hist_bins=*/128);

  // collect bounds for all blocks globally.
  diy::all_to_all(master, cuts_assigner, [](void* b, const diy::ReduceProxy& srp) {
    BlockT* block = reinterpret_cast<BlockT*>(b);
    if (srp.round() == 0)
    {
      for (int i = 0; i < srp.out_link().size(); ++i)
      {
        auto link = static_cast<diy::RegularContinuousLink*>(
          srp.master()->link(srp.master()->lid(srp.gid())));
        srp.enqueue(srp.out_link().target(i), link->bounds());
      }
    }
    else
    {
      block->BlockBounds.resize(srp.in_link().size());
      for (int i = 0; i < srp.in_link().size(); ++i)
      {
        assert(i == srp.in_link().target(i).gid);
        srp.dequeue(srp.in_link().target(i).gid, block->BlockBounds[i]);
      }
    }
  });

  std::vector<vtkBoundingBox> cuts(num_cuts);
  if (master.size() > 0)
  {
    const auto b0 = master.get<BlockT>(0);
    for (int gid = 0; gid < num_cuts; ++gid)
    {
      cuts[gid] = vtkDIYUtilities::Convert(b0->BlockBounds[gid]);
    }
  }

  if (num_cuts < comm.size())
  {
    // we have a case where some ranks may not have any blocks and hence will
    // not have the partition information at all. Just broadcast that info to
    // all.
    vtkDIYUtilities::Broadcast(comm, cuts, 0);
  }
  return cuts;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPartitionedDataSet> vtkDIYKdTreeUtilities::Exchange(
  vtkPartitionedDataSet* localParts, vtkMultiProcessController* controller)
{
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  const int nblocks = static_cast<int>(localParts->GetNumberOfPartitions());
#if !defined(NDEBUG)
  {
    // ensure that all ranks report exactly same number of partitions.
    int sumblocks = 0;
    diy::mpi::all_reduce(comm, nblocks, sumblocks, std::plus<int>());
    assert(sumblocks == nblocks * comm.size());
  }
#endif
  diy::ContiguousAssigner block_assigner(comm.size(), nblocks);

  using VectorOfUG = std::vector<vtkSmartPointer<vtkUnstructuredGrid> >;
  using VectorOfVectorOfUG = std::vector<VectorOfUG>;

  diy::Master master(comm, 1, -1, []() { return static_cast<void*>(new VectorOfVectorOfUG()); },
    [](void* b) { delete static_cast<VectorOfVectorOfUG*>(b); });

  diy::ContiguousAssigner assigner(comm.size(), comm.size());
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  assert(master.size() == 1);

  const int myrank = comm.rank();
  diy::all_to_all(master, assigner,
    [&block_assigner, &myrank, localParts](VectorOfVectorOfUG* block, const diy::ReduceProxy& rp) {
      if (rp.in_link().size() == 0)
      {
        // enqueue blocks to send.
        block->resize(localParts->GetNumberOfPartitions());
        for (unsigned int partId = 0; partId < localParts->GetNumberOfPartitions(); ++partId)
        {
          if (auto part = vtkUnstructuredGrid::SafeDownCast(localParts->GetPartition(partId)))
          {
            auto target_rank = block_assigner.rank(partId);
            if (target_rank == myrank)
            {
              // short-circuit messages to self.
              (*block)[partId].push_back(part);
            }
            else
            {
              rp.enqueue(rp.out_link().target(target_rank), partId);
              rp.enqueue<vtkDataSet*>(rp.out_link().target(target_rank), part);
            }
          }
        }
      }
      else
      {
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const int gid = rp.in_link().target(i).gid;
          while (rp.incoming(gid))
          {
            unsigned int partId = 0;
            rp.dequeue(rp.in_link().target(i), partId);

            vtkDataSet* ptr = nullptr;
            rp.dequeue<vtkDataSet*>(rp.in_link().target(i), ptr);

            vtkSmartPointer<vtkUnstructuredGrid> sptr;
            sptr.TakeReference(vtkUnstructuredGrid::SafeDownCast(ptr));
            (*block)[partId].push_back(sptr);
          }
        }
      }
    });

  vtkNew<vtkPartitionedDataSet> result;
  result->SetNumberOfPartitions(localParts->GetNumberOfPartitions());
  auto block0 = *master.block<VectorOfVectorOfUG>(0);
  assert(static_cast<unsigned int>(block0.size()) == result->GetNumberOfPartitions());

  for (unsigned int cc = 0; cc < result->GetNumberOfPartitions(); ++cc)
  {
    if (block0[cc].size() == 1)
    {
      result->SetPartition(cc, block0[cc][0]);
    }
    else if (block0[cc].size() > 1)
    {
      vtkNew<vtkAppendFilter> appender;
      for (auto& ug : block0[cc])
      {
        appender->AddInputDataObject(ug);
      }
      appender->Update();
      result->SetPartition(cc, appender->GetOutputDataObject(0));
    }
  }

  return result;
}

//----------------------------------------------------------------------------
bool vtkDIYKdTreeUtilities::GenerateGlobalPointIds(vtkPartitionedDataSet* parts,
  const char* pt_ownership_arrayname, vtkMultiProcessController* controller)
{
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  const int nblocks = static_cast<int>(parts->GetNumberOfPartitions());
#if !defined(NDEBUG)
  {
    // ensure that all ranks report exactly same number of partitions.
    int sumblocks = 0;
    diy::mpi::all_reduce(comm, nblocks, sumblocks, std::plus<int>());
    assert(sumblocks == nblocks * comm.size());
  }
#endif

  std::vector<vtkIdType> point_counts(nblocks, 0);

  // Iterate over each part and count locally owned points.
  for (int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      auto array = vtkIntArray::SafeDownCast(ds->GetPointData()->GetArray(pt_ownership_arrayname));
      assert(array != nullptr);
      auto& count = point_counts[partId];
      for (vtkIdType cc = 0; cc < ds->GetNumberOfPoints(); ++cc)
      {
        count += (array->GetTypedComponent(cc, 0) == partId) ? 1 : 0;
      }
    }
  }

  std::vector<vtkIdType> all_point_counts(nblocks, 0);
  diy::mpi::all_reduce(comm, point_counts, all_point_counts, diy::mpi::maximum<vtkIdType>());

  // compute exclusive scan to determine the global id offsets for each part.
  std::vector<vtkIdType> point_offsets(nblocks, 0);
  for (int cc = 1; cc < nblocks; ++cc)
  {
    point_offsets[cc] = point_offsets[cc - 1] + all_point_counts[cc - 1];
  }

  // now assign global ids for owned points.
  for (int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      vtkNew<vtkIdTypeArray> gpids;
      gpids->SetNumberOfComponents(1);
      gpids->SetNumberOfTuples(ds->GetNumberOfPoints());
      gpids->SetName("vtkGlobalPointIds");
      ds->GetPointData()->SetGlobalIds(gpids);

      auto array = vtkIntArray::SafeDownCast(ds->GetPointData()->GetArray(pt_ownership_arrayname));
      assert(array != nullptr);
      auto start_offset = point_offsets[partId];
      for (vtkIdType cc = 0; cc < ds->GetNumberOfPoints(); ++cc)
      {
        gpids->SetTypedComponent(
          cc, 0, (array->GetTypedComponent(cc, 0) == partId) ? (start_offset++) : -1);
      }
    }
  }

  // now do a `diy::all_to_all` reduction to exchange requests for global ids
  // for points.
  using BlockT = std::map<int, vtkSmartPointer<vtkIdTypeArray> >;

  diy::Master master(comm, 1, -1, []() { return static_cast<void*>(new BlockT); },
    [](void* b) { delete static_cast<BlockT*>(b); });
  diy::ContiguousAssigner assigner(comm.size(), nblocks);

  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  for (const int gid : gids)
  {
    auto block = new BlockT();
    auto l = new diy::Link();
    master.add(gid, block, l);
  }

  // now, communicate between all ranks to request global ids for unowned points.

  diy::all_to_all(master, assigner, [&](BlockT* block, const diy::ReduceProxy& rp) {
    const int gid = rp.gid();
    auto ds = parts->GetPartition(gid);
    if (rp.in_link().size() == 0)
    {
      // enqueue requests for global point ids given point coordinates
      auto ownership_array =
        vtkIntArray::SafeDownCast(ds->GetPointData()->GetArray(pt_ownership_arrayname));
      assert(ownership_array);
      for (vtkIdType cc = 0, max = ds->GetNumberOfPoints(); cc < max; ++cc)
      {
        const auto owner = ownership_array->GetTypedComponent(cc, 0);
        if (owner != gid && owner >= 0)
        {
          auto dest = rp.out_link().target(owner);
          // fixme: respect point type
          double pt[3];
          ds->GetPoint(cc, pt);
          rp.enqueue(dest, pt, 3);
        }
      }
    }
    else
    {
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        const int src = rp.in_link().target(cc).gid;
        assert(src == cc);

        diy::MemoryBuffer& incoming = rp.incoming(src);
        const vtkIdType count = static_cast<vtkIdType>(incoming.size() / (3 * sizeof(double)));
        if (count <= 0)
        {
          continue;
        }

        assert(src != gid); // a block does not make requests to itself

        auto out_array = vtkSmartPointer<vtkIdTypeArray>::New();
        out_array->SetName("vtkGlobalPointIds");
        out_array->SetNumberOfComponents(1);
        out_array->SetNumberOfTuples(count);
        out_array->FillValue(-2);

        auto in_gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
        assert(in_gids != nullptr);

        for (vtkIdType idx = 0; idx < count; ++idx)
        {
          double pt[3];
          rp.dequeue(src, pt, 3);

          auto ptId = ds->FindPoint(pt);
          assert(ptId >= 0); // else bad request!
          auto val = in_gids->GetTypedComponent(ptId, 0);
          assert(val >= 0);
          out_array->SetTypedComponent(idx, 0, val);
        }

        (*block)[src] = out_array;
      }
    }
  });

  // now, reply back using the `outgoing_ids`.
  diy::all_to_all(master, assigner, [&](BlockT* block, const diy::ReduceProxy& rp) {
    const int gid = rp.gid();
    auto ds = parts->GetPartition(gid);
    if (rp.in_link().size() == 0)
    {
      for (auto pair : (*block))
      {
        assert(gid != pair.first);

        auto dest = rp.out_link().target(pair.first);
        auto ids = pair.second.GetPointer();
        assert(ids != nullptr);
        assert(ids->GetNumberOfValues() > 0);
        rp.enqueue(dest, ids->GetPointer(0), ids->GetNumberOfValues());
      }
    }
    else
    {
      for (int cc = 0; cc < rp.in_link().size(); ++cc)
      {
        const int src = rp.in_link().target(cc).gid;
        assert(src == cc);

        if (rp.incoming(src).size() == 0)
        {
          continue;
        }

        assert(src != gid); // a block does not send data to itself

        // enqueue requests for global point ids given point coordinates
        auto ownership_array =
          vtkIntArray::SafeDownCast(ds->GetPointData()->GetArray(pt_ownership_arrayname));
        assert(ownership_array);

        auto gids = vtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds());
        assert(gids != nullptr);

        for (vtkIdType cc = 0, max = ds->GetNumberOfPoints(); cc < max; ++cc)
        {
          const auto owner = ownership_array->GetTypedComponent(cc, 0);
          if (owner == src)
          {
            vtkIdType id;
            rp.dequeue(src, id);
            gids->SetTypedComponent(cc, 0, id);
          }
        }
      }
    }
  });
  //  diy::ContiguousAssigner block_assigner(comm.size(), nblocks);

  return true;
}

//----------------------------------------------------------------------------
bool vtkDIYKdTreeUtilities::GenerateGlobalCellIds(
  vtkPartitionedDataSet* parts, vtkMultiProcessController* controller)
{
  // We need to generate global cells ids. The algorithm is simple.
  // 1. globally count non-ghost cells and then determine what range of gids
  //    each block will assign to its non-ghost cells
  // 2. each block then locally assign gids to its non-ghost cells.

  // the thing to remember that the parts here are not yet split based on cuts, as a result they
  // are not uniquely assigned among ranks. Thus number of partitions on all ranks may be different

  auto nblocks = parts->GetNumberOfPartitions();
  std::vector<vtkIdType> local_cell_counts(nblocks, 0);

  // Iterate over each part and count non-ghost cells.
  for (int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      auto& count = local_cell_counts[partId];

      auto ghostcells = vtkUnsignedCharArray::SafeDownCast(
        ds->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
      if (ghostcells)
      {
        for (vtkIdType cc = 0; cc < ds->GetNumberOfCells(); ++cc)
        {
          const bool is_ghost =
            (ghostcells->GetTypedComponent(cc, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0;
          count += is_ghost ? 0 : 1;
        }
      }
      else
      {
        count += ds->GetNumberOfCells();
      }
    }
  }

  const vtkIdType total_local_cells =
    std::accumulate(local_cell_counts.begin(), local_cell_counts.end(), 0);
  vtkIdType global_offset = 0;

  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  diy::mpi::scan(comm, total_local_cells, global_offset, std::plus<vtkIdType>());
  // convert to exclusive scan since mpi_scan is inclusive.
  global_offset -= total_local_cells;

  // compute exclusive scan to determine the global id offsets for each local partition.
  std::vector<vtkIdType> local_cell_offsets(nblocks, 0);
  local_cell_offsets[0] = global_offset;
  for (int cc = 1; cc < nblocks; ++cc)
  {
    local_cell_offsets[cc] = local_cell_offsets[cc - 1] + local_cell_counts[cc - 1];
  }

  // now assign global ids for non-ghost cells alone.
  for (int partId = 0; partId < nblocks; ++partId)
  {
    if (auto ds = parts->GetPartition(partId))
    {
      const auto numCells = ds->GetNumberOfCells();

      vtkNew<vtkIdTypeArray> gids;
      gids->SetName("vtkGlobalCellIds");
      gids->SetNumberOfTuples(numCells);
      auto ghostcells = vtkUnsignedCharArray::SafeDownCast(
        ds->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
      auto id = local_cell_offsets[partId];
      if (ghostcells)
      {
        for (vtkIdType cc = 0; cc < numCells; ++cc)
        {
          const bool is_ghost =
            (ghostcells->GetTypedComponent(cc, 0) & vtkDataSetAttributes::DUPLICATECELL) != 0;
          gids->SetTypedComponent(cc, 0, is_ghost ? -1 : id++);
        }
      }
      else
      {
        for (vtkIdType cc = 0; cc < numCells; ++cc)
        {
          gids->SetTypedComponent(cc, 0, id++);
        }
      }

      ds->GetCellData()->SetGlobalIds(gids);
    }
  }

  return true;
}
