#include "vtkCellTextureToPointTextureInternal.h"
#include "vtkMathUtilities.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

/**
   * Go over all cells that have cell textures (each cell has texture
   * coordinates for each point of the cell) and change that into point
   * texture. Duplicate points that have 2 or more different textures
   * comming from different cells.
   */

vtkCellTextureToPointTextureInternal::vtkCellTextureToPointTextureInternal()
{
  this->TexLocator = nullptr;
}

void vtkCellTextureToPointTextureInternal::Initialize(
  int numberOfPoints, float faceTextureTolerance)
{
  this->PointIds.resize(numberOfPoints);
  this->TexLocator = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  this->TexLocator->SetTolerance(faceTextureTolerance);
  vtkNew<vtkPoints> texCoords;
  double bounds[] = {0.0, 1.0, 0.0, 1.0, 0.0, 0.0};
  this->TexLocator->InitPointInsertion(texCoords, bounds);
}

/**
 * Gets a `cell` and `texCoordsCell` and fills in
 * `texCoordsPoints` which is the texture point coordinates array
 * and modifies `cell`. The whole mesh is `output`
 */
void vtkCellTextureToPointTextureInternal::DuplicatePoints(
  vtkPolygon* cell, float* texCoordsCell,
  vtkFloatArray* texCoordsPoints, vtkPolyData *output)
{
  int nverts = cell->GetNumberOfPoints();
  vtkIdType* faceVerts = cell->GetPointIds()->GetPointer(0);
  float tolerance = this->TexLocator->GetTolerance();
  for (int k = 0; k < nverts; k++)
  {
    // new texture stored at the current face
    float newTex[] = {texCoordsCell[k * 2],
      texCoordsCell[k * 2 + 1]};
    // texture stored at faceVerts[k] point
    float currentTex[2];
    texCoordsPoints->GetTypedTuple(faceVerts[k], currentTex);
    double newTex3[] = {newTex[0], newTex[1], 0};
    if (currentTex[0] == -1.0)
    {
      // newly seen texture coordinates for vertex
      texCoordsPoints->SetTuple2(
        faceVerts[k], newTex[0], newTex[1]);
      vtkIdType ti;
      this->TexLocator->InsertUniquePoint(newTex3, ti);
      this->PointIds.resize(
        std::max(ti+1,
                 static_cast<vtkIdType>(this->PointIds.size())));
      this->PointIds[ti].push_back(faceVerts[k]);
    }
    else
    {
      if (! vtkMathUtilities::FuzzyCompare(
            currentTex[0], newTex[0],
            tolerance) ||
          ! vtkMathUtilities::FuzzyCompare(
            currentTex[1], newTex[1],
            tolerance))
      {
        // different texture coordinate
        // than stored at point faceVerts[k]
        vtkIdType ti;
        int inserted = this->TexLocator->InsertUniquePoint(newTex3, ti);
        if (inserted)
        {
          // newly seen texture coordinate for vertex
          // which already has some texture coordinates.
          vtkIdType dp = DuplicatePoint(
            output, cell, k);
          texCoordsPoints->SetTuple2(dp, newTex[0], newTex[1]);
          this->PointIds.resize(
            std::max(
              ti+1, static_cast<vtkIdType>(this->PointIds.size())));
          this->PointIds[ti].push_back(dp);
        }
        else
        {
          size_t sameTexIndex = 0;
          if (this->PointIds[ti].size() > 1)
          {
            double first[3];
            output->GetPoint(faceVerts[k], first);
            for (;sameTexIndex < this->PointIds[ti].size();
                 ++sameTexIndex)
            {
              double second[3];
              output->GetPoint(this->PointIds[ti][sameTexIndex],second);
              if (FuzzyEqual(first, second, tolerance))
              {
                break;
              }
            }
            if (sameTexIndex == this->PointIds[ti].size())
            {
              // newly seen point for this texture coordinate
              vtkIdType dp = DuplicatePoint(
                output, cell, k);
              texCoordsPoints->SetTuple2(dp, newTex[0], newTex[1]);
              this->PointIds[ti].push_back(dp);
            }
          }

          // texture coordinate already seen before, use the vertex
          // associated with these texture coordinates
          vtkIdType vi = this->PointIds[ti][sameTexIndex];
          cell->GetPointIds()->SetId(k, vi);
        }
      }
      // same texture coordinate, nothing to do.
    }
  }
}

/**
 * Create an extra point in 'data' with the same coordinates and data as
 * the point at cellPointIndex inside cell. This is to avoid texture artifacts
 * when you have one point with two different texture values (so the latter
 * value override the first. This results in a texture discontinuity which results
 * in artifacts).
 */
vtkIdType vtkCellTextureToPointTextureInternal::DuplicatePoint(
  vtkPolyData* data, vtkCell* cell, int cellPointIndex)
{
  // get the old point id
  vtkIdList* pointIds = cell->GetPointIds();
  vtkIdType pointId = pointIds->GetId(cellPointIndex);

  // duplicate that point and all associated data
  vtkPoints* points = data->GetPoints();
  double* point = data->GetPoint(pointId);
  vtkIdType newPointId = points->InsertNextPoint(point);
  for (int i = 0; i < data->GetPointData()->GetNumberOfArrays(); ++i)
  {
    vtkDataArray* a = data->GetPointData()->GetArray(i);
    a->InsertTuple(newPointId, a->GetTuple(pointId));
  }
  // make cell use the new point
  pointIds->SetId(cellPointIndex, newPointId);
  return newPointId;
}


/**
 * Compare two points for equality
 */
bool vtkCellTextureToPointTextureInternal::FuzzyEqual(double* f, double* s, double t)
{
  return vtkMathUtilities::FuzzyCompare(f[0], s[0], t) &&
    vtkMathUtilities::FuzzyCompare(f[1], s[1], t) &&
    vtkMathUtilities::FuzzyCompare(f[2], s[2], t);
}
