#include "vtkCylindricalGrid.h"

#include <vtkCellType.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>

#include <map>

class vtkCylindricalGrid::Impl
{
public:
  Impl() {}
  virtual ~Impl() {}

  //@{
  /**
   * Calculates the number of intermediate points which need to be added to ensure all intervals are
   * smaller than the max angle.
   */
  static int CalculateNumberOfIntermediatePointsRequired(double p1, double p2, double maximumAngle);
  //@}

  //@{
  /**
   * Sanitizes polar angles to within a single revolution.
   *
   * If a cell is defined as existing from 390 -> 420 degrees, it simplifies the math to know that
   * is actually starting at 30 degrees.
   */
  static double LocatePolarAngleInFirstRevolution(double p1);
  //@}

  static const double TWO_PI_PRECISION_FACTOR;

  struct Point
  {
    double x, y, z;
    bool operator<(const Point& rhs) const
    {
      if (x != rhs.x)
      {
        return x < rhs.x;
      }
      if (y != rhs.y)
      {
        return y < rhs.y;
      }
      return z < rhs.z;
    }
  };
  typedef std::map<Point, vtkIdType> UniquePointIndexLookupTable;

  static vtkIdType CreateUniquePoint(double r, double p, double z,
    UniquePointIndexLookupTable& pointLookup, vtkSmartPointer<vtkPoints> points);
  static std::vector<vtkIdType> CreatePointLine(double r, double p1, double p2, double z,
    int numIntermediatePoints, UniquePointIndexLookupTable& pointLookup,
    vtkSmartPointer<vtkPoints> points);
};

const double vtkCylindricalGrid::Impl::TWO_PI_PRECISION_FACTOR =
  1.9999999; // to avoid point duplication with precision errors

int vtkCylindricalGrid::Impl::CalculateNumberOfIntermediatePointsRequired(
  double p1, double p2, double maximumAngle)
{
  double delta = p2 - p1;
  double numberOfSegmentsRequired = ceil(abs(delta / maximumAngle));
  return std::max(0, static_cast<int>(numberOfSegmentsRequired - 1));
}

double vtkCylindricalGrid::Impl::LocatePolarAngleInFirstRevolution(double p1)
{
  // Remove Multiple Revolutions
  double pi2 = 2.0 * vtkMath::Pi();
  vtkIdType numberOfRevolutions = static_cast<vtkIdType>(p1 / pi2);
  p1 -= static_cast<double>(numberOfRevolutions) * pi2;

  // Correct for first rotation negative values and numerical precision cases.
  if (p1 < 0.0)
  {
    p1 += pi2;
  }

  return p1;
}

vtkIdType vtkCylindricalGrid::Impl::CreateUniquePoint(double r, double p, double z,
  vtkCylindricalGrid::Impl::UniquePointIndexLookupTable& pointLookup,
  vtkSmartPointer<vtkPoints> points)
{
  p = LocatePolarAngleInFirstRevolution(p);

  Point point{ r, p, z };
  auto itr = pointLookup.find(point);
  if (itr != pointLookup.end())
  {
    return itr->second;
  }
  auto pointID = pointLookup.size();
  pointLookup[point] = pointID;

  double x = r * cos(p);
  double y = r * sin(p);
  points->InsertNextPoint(x, y, z);
  return pointID;
}

std::vector<vtkIdType> vtkCylindricalGrid::Impl::CreatePointLine(double r, double p1, double p2,
  double z, int numIntermediatePoints,
  vtkCylindricalGrid::Impl::UniquePointIndexLookupTable& pointLookup,
  vtkSmartPointer<vtkPoints> points)
{
  std::vector<vtkIdType> pointIDs;
  pointIDs.push_back(CreateUniquePoint(r, p1, z, pointLookup, points));

  if (numIntermediatePoints > 0)
  {
    auto delta = (p2 - p1) / static_cast<double>(numIntermediatePoints + 1);
    for (int intPointIndex = 0; intPointIndex < numIntermediatePoints; intPointIndex++)
    {
      auto pInt = p1 + static_cast<double>(intPointIndex + 1) * delta;
      if (abs(pInt - p1) <= TWO_PI_PRECISION_FACTOR * vtkMath::Pi())
      {
        pointIDs.push_back(CreateUniquePoint(r, pInt, z, pointLookup, points));
      }
      else
      {
        pointIDs.push_back(*pointIDs.begin());
      }
    }
  }

  if (abs(p2 - p1) <= TWO_PI_PRECISION_FACTOR * vtkMath::Pi())
  {
    pointIDs.push_back(CreateUniquePoint(r, p2, z, pointLookup, points));
  }
  else
  {
    pointIDs.push_back(*pointIDs.begin());
  }

  return pointIDs;
}

vtkStandardNewMacro(vtkCylindricalGrid);
vtkCylindricalGrid::vtkCylindricalGrid()
  : UseDegrees(true)
{
  // Set the default maximum angle to disable inserting intermediate points. Set slightly
  // larger than one revolution to avoid precision errors on 360 degree cells.
  SetMaximumAngle(361);
}

void vtkCylindricalGrid::SetMaximumAngle(double maxAngle)
{
  if (this->UseDegrees)
  {
    maxAngle = vtkMath::RadiansFromDegrees(maxAngle);
  }

  this->MaximumAngle = std::max(
    vtkMath::RadiansFromDegrees(0.1), std::min(maxAngle, vtkMath::RadiansFromDegrees(361.0)));
}

double vtkCylindricalGrid::GetMaximumAngle()
{
  if (this->UseDegrees)
  {
    return vtkMath::DegreesFromRadians(this->MaximumAngle);
  }
  return this->MaximumAngle;
}

void vtkCylindricalGrid::InsertNextCylindricalCell(
  double r1, double r2, double p1, double p2, double z1)
{
  // Initialize Parameters
  if (this->UseDegrees)
  {
    p1 = vtkMath::RadiansFromDegrees(p1);
    p2 = vtkMath::RadiansFromDegrees(p2);
  }

  auto numIntermediatePoints =
    Impl::CalculateNumberOfIntermediatePointsRequired(p1, p2, this->MaximumAngle);

  // Create Data Structures Required
  auto points = this->GetPoints();
  if (points == NULL)
  {
    auto newPoints = vtkSmartPointer<vtkPoints>::New();
    this->SetPoints(newPoints);
    points = this->GetPoints();
  }
  auto cpo = points->GetNumberOfPoints(); // cell point offset

  // Generate all the required points for the cell and list of indices for making faces
  Impl::UniquePointIndexLookupTable uniquePoints;
  auto innerPoints =
    Impl::CreatePointLine(r1, p1, p2, z1, numIntermediatePoints, uniquePoints, points); // Inner
  auto outerPoints =
    Impl::CreatePointLine(r2, p2, p1, z1, numIntermediatePoints, uniquePoints, points); // Outer

  std::vector<vtkIdType> pointIDs;
  pointIDs.insert(pointIDs.end(), innerPoints.begin(), innerPoints.end());
  pointIDs.insert(pointIDs.end(), outerPoints.begin(), outerPoints.end());

  // Prepare to make faces
  vtkIdType edgePoints = numIntermediatePoints + 2;
  std::vector<vtkIdType> faces;
  faces.push_back(edgePoints - 1);

  // Face
  for (vtkIdType i = 0; i < edgePoints - 1; i++)
  {
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[i]);
    faces.push_back(cpo + pointIDs[i + 1]);
    faces.push_back(cpo + pointIDs[(2 * edgePoints - 1) - i - 1]);
    faces.push_back(cpo + pointIDs[(2 * edgePoints - 1) - i]);
  }

  // Save Cells in Grid
  auto faceList = vtkSmartPointer<vtkIdList>::New();
  for (auto& face : faces)
  {
    faceList->InsertNextId(face);
  }
  InsertNextCell(VTK_POLYHEDRON, faceList);
}

void vtkCylindricalGrid::InsertNextCylindricalCell(
  double r1, double r2, double p1, double p2, double z1, double z2)
{
  // Initialize Parameters
  if (this->UseDegrees)
  {
    p1 = vtkMath::RadiansFromDegrees(p1);
    p2 = vtkMath::RadiansFromDegrees(p2);
  }

  auto numIntermediatePoints =
    Impl::CalculateNumberOfIntermediatePointsRequired(p1, p2, this->MaximumAngle);

  // Create Data Structures Required
  auto points = this->GetPoints();
  if (points == NULL)
  {
    auto newPoints = vtkSmartPointer<vtkPoints>::New();
    this->SetPoints(newPoints);
    points = this->GetPoints();
  }
  auto cpo = points->GetNumberOfPoints(); // cell point offset

  // Generate all the required points for the cell and list of indices for making faces
  Impl::UniquePointIndexLookupTable uniquePoints;
  auto innerBottomPoints = Impl::CreatePointLine(
    r1, p1, p2, z1, numIntermediatePoints, uniquePoints, points); // Inner Bottom
  auto outerBottomPoints = Impl::CreatePointLine(
    r2, p2, p1, z1, numIntermediatePoints, uniquePoints, points); // Outer Bottom
  auto innerTopPoints =
    Impl::CreatePointLine(r1, p1, p2, z2, numIntermediatePoints, uniquePoints, points); // Inner Top
  auto outerTopPoints =
    Impl::CreatePointLine(r2, p2, p1, z2, numIntermediatePoints, uniquePoints, points); // Outer Top

  std::vector<vtkIdType> pointIDs;
  pointIDs.insert(pointIDs.end(), innerBottomPoints.begin(), innerBottomPoints.end());
  pointIDs.insert(pointIDs.end(), outerBottomPoints.begin(), outerBottomPoints.end());
  pointIDs.insert(pointIDs.end(), innerTopPoints.begin(), innerTopPoints.end());
  pointIDs.insert(pointIDs.end(), outerTopPoints.begin(), outerTopPoints.end());

  // Prepare to make faces
  vtkIdType edgePoints = numIntermediatePoints + 2;
  std::vector<vtkIdType> faces;
  auto facesTopBottom = 2 * (edgePoints - 1);
  auto facesInsideOutside = 2 * (edgePoints - 1);
  auto facesEnds =
    ((p2 - p1) >= (vtkCylindricalGrid::Impl::TWO_PI_PRECISION_FACTOR * vtkMath::Pi())) ? 0 : 2;
  faces.push_back(facesTopBottom + facesInsideOutside + facesEnds);

  // Bottom
  for (vtkIdType i = 0; i < edgePoints - 1; i++)
  {
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[i]);
    faces.push_back(cpo + pointIDs[i + 1]);
    faces.push_back(cpo + pointIDs[(2 * edgePoints - 1) - i - 1]);
    faces.push_back(cpo + pointIDs[(2 * edgePoints - 1) - i]);
  }

  // Top
  for (vtkIdType i = 0; i < edgePoints - 1; i++)
  {
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[(2 * edgePoints) + i]);
    faces.push_back(cpo + pointIDs[(2 * edgePoints) + i + 1]);
    faces.push_back(cpo + pointIDs[(4 * edgePoints - 1) - i - 1]);
    faces.push_back(cpo + pointIDs[(4 * edgePoints - 1) - i]);
  }

  // Inside
  for (vtkIdType i = 0; i < edgePoints - 1; i++)
  {
    auto bottom = i;
    auto top = 2 * edgePoints + i;
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[bottom]);
    faces.push_back(cpo + pointIDs[bottom + 1]);
    faces.push_back(cpo + pointIDs[top + 1]);
    faces.push_back(cpo + pointIDs[top]);
  }

  // Outside
  for (vtkIdType i = 0; i < edgePoints - 1; i++)
  {
    auto bottom = edgePoints + i;
    auto top = 3 * edgePoints + i;
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[bottom]);
    faces.push_back(cpo + pointIDs[bottom + 1]);
    faces.push_back(cpo + pointIDs[top + 1]);
    faces.push_back(cpo + pointIDs[top]);
  }

  // Caps -> If the cell is not a complete rotation, it needs side faces.
  // Omit if the cell completes a full rotation to avoid inserting unnecessary internal geometry
  if (facesEnds)
  {
    faces.push_back(4);
    faces.push_back(cpo + pointIDs[0]);
    faces.push_back(cpo + pointIDs[2 * edgePoints]);
    faces.push_back(cpo + pointIDs[4 * edgePoints - 1]);
    faces.push_back(cpo + pointIDs[2 * edgePoints - 1]);

    faces.push_back(4);
    faces.push_back(cpo + pointIDs[edgePoints - 1]);
    faces.push_back(cpo + pointIDs[edgePoints]);
    faces.push_back(cpo + pointIDs[3 * edgePoints]);
    faces.push_back(cpo + pointIDs[3 * edgePoints - 1]);
  }

  // Save Cells in Grid
  auto faceList = vtkSmartPointer<vtkIdList>::New();
  for (auto& face : faces)
  {
    faceList->InsertNextId(face);
  }
  InsertNextCell(VTK_POLYHEDRON, faceList);
}
