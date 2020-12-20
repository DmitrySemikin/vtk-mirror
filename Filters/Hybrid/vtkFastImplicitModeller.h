/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastImplicitModeller.h

=========================================================================*/
/**
 * @class   vtkFastImplicitModeller
 * @brief   A class for fast implicit modelling of vtkPolyData, similar to vtkImplicitModeller
 *
 * This class is an improved version of vtkImplicitModeller.
 * It constructs a vtkImageData voxel space from a
 * vtkPolyData triangulated mesh. As for the original
 * vtkImplicitModeller, the user can define how the voxel
 * space is adjusted to extend the bounding box of the
 * input mesh using AdjustBoundsOn and SetAdjustDistance.
 * Non-float output types can be scaled to the
 * CapValue by turning ScaleToMaximumDistance On.
 * Selection of signed/unsigned distance is possible via the
 * ComputeSigned switch. Make sure to select a signed output
 * type when computing signed distance, or the sign will be
 * lost. A negative distance means the voxel is inside
 * the mesh.
 *
 * @warning For the inverse process (from vtkImageData to vtkPolyData), we recommend using vtkFlyingEdges3D rather than vtkMarchingCubes.
 * The first is an optimized, parallel version of the latter and is definitely more suitable for realtime applications.
 *
 * @warning The algorithm is affected by vertex order in triangles, as it (the vertex order) is used to determine the inner and outer sides of the mesh.
 * If vtkFastImplicitModeller does not give the expected result, try using vtkReverseSense on the input polydata. You can also write OBJ files with
 * flipped vertex order using vtkOBJWriterInstance->SetFlipVertexOrder(true);
*/

#ifndef VTKFASTIMPLICITMODELLER_H
#define VTKFASTIMPLICITMODELLER_H

#define __min(a,b) std::min(a, b)
#define __max(a,b) std::max(a, b)

#include <vtkImageAlgorithm.h>
#include <vtkSmartPointer.h>
#include <vtkFiltersHybridModule.h>
#include <vtkPolyData.h>

class vtkFastImplicitModeller : public vtkImageAlgorithm
{
public:
    vtkTypeMacro(vtkFastImplicitModeller,vtkImageAlgorithm);
    void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

    /// Static constructor (Factory pattern)
    static vtkFastImplicitModeller *New();

    /// Compute ModelBounds from input geometry. If input is not specified, the input of the filter will be used.
    double ComputeModelBounds(vtkPolyData *input = nullptr);

    /// Get the i-j-k dimensions on which to sample distance function.
    vtkGetVectorMacro(SampleDimensions,int,3);
    /// Set the i-j-k dimensions on which to sample distance function.
    void SetSampleDimensions(int i, int j, int k);
    /// Set the i-j-k dimensions on which to sample distance function.
    void SetSampleDimensions(int dim[3]);
    //@}

    //@{
    /**
     * Set / get the distance away from surface of input geometry to
     * sample. This value is specified as a percentage of the length of
     * the diagonal of the input data bounding box.
     * Smaller values make large increases in performance.
     */
    vtkSetClampMacro(MaximumDistance,double,0.0,1.0);
    vtkGetMacro(MaximumDistance,double);
    //@}

    //@{
    /**
     * Set / get the region in space in which to perform the sampling. If
     * not specified, it will be computed automatically.
     */
    vtkSetVector6Macro(ModelBounds,double);
    vtkGetVectorMacro(ModelBounds,double,6);
    //@}

    //@{
    /**
     * Control how the model bounds are computed. If the ivar AdjustBounds
     * is set, then the bounds specified (or computed automatically) is modified
     * by the fraction given by AdjustDistance. This means that the model
     * bounds is expanded in each of the x-y-z directions.
     */
    vtkSetMacro(AdjustBounds,int);
    vtkGetMacro(AdjustBounds,int);
    vtkBooleanMacro(AdjustBounds,int);
    //@}

    //@{
    /**
     * Specify the amount to grow the model bounds (if the ivar AdjustBounds
     * is set). The value is a fraction of the maximum length of the sides
     * of the box specified by the model bounds.
     */
    vtkSetClampMacro(AdjustDistance,double,-1.0,1.0);
    vtkGetMacro(AdjustDistance,double);
    //@}

    //@{
    /**
     * The outer boundary of the structured point set can be assigned a
     * particular value. This can be used to close or "cap" all surfaces.
     */
    vtkSetMacro(Capping,int);
    vtkGetMacro(Capping,int);
    vtkBooleanMacro(Capping,int);
    //@}

    //@{
    /**
     * Specify the capping value to use. The CapValue is also used as an
     * initial distance value at each point in the dataset.
     */
    void SetCapValue(double value);
    vtkGetMacro(CapValue,double);
    //@}

    //@{
    /**
     * Specify whether we are computing a signed or unsigned distance.
     */
    vtkSetMacro(ComputeSigned,int);
    vtkGetMacro(ComputeSigned,int);
    vtkBooleanMacro(ComputeSigned,int);
    //@}

    //@{
    /**
     * If a non-floating output type is specified, the output distances can be
     * scaled to use the entire positive scalar range of the output type
     * specified (up to the CapValue which is equal to the max for the type
     * unless modified by the user).  For example, if ScaleToMaximumDistance
     * is On and the OutputScalarType is UnsignedChar the distances saved in the
     * output would be linearly scaled between 0 (for distances "very close" to
     * the surface) and 255 (at the specifed maximum distance)... assuming the
     * CapValue is not changed from 255.
     */
    vtkSetMacro(ScaleToMaximumDistance, int);
    vtkGetMacro(ScaleToMaximumDistance, int);
    vtkBooleanMacro(ScaleToMaximumDistance,int);
    //@}

    //@{
    /**
    * Set the desired output scalar type.
    */

#define signErrorMacro vtkWarningMacro(<<"Signed distance stored in unsigned type. Sign will be lost.");

    void SetOutputScalarType(int type);
    vtkGetMacro(OutputScalarType,int);
    void SetOutputScalarTypeToFloat(){this->SetOutputScalarType(VTK_FLOAT);};
    void SetOutputScalarTypeToDouble(){this->SetOutputScalarType(VTK_DOUBLE);};
    void SetOutputScalarTypeToInt(){this->SetOutputScalarType(VTK_INT);};
    void SetOutputScalarTypeToUnsignedInt(){if (ComputeSigned) signErrorMacro; this->SetOutputScalarType(VTK_UNSIGNED_INT);};
    void SetOutputScalarTypeToLong(){this->SetOutputScalarType(VTK_LONG);};
    void SetOutputScalarTypeToUnsignedLong(){if (ComputeSigned) signErrorMacro; this->SetOutputScalarType(VTK_UNSIGNED_LONG);};
    void SetOutputScalarTypeToShort(){this->SetOutputScalarType(VTK_SHORT);};
    void SetOutputScalarTypeToUnsignedShort(){if (ComputeSigned) signErrorMacro; this->SetOutputScalarType(VTK_UNSIGNED_SHORT);};
    void SetOutputScalarTypeToChar() {this->SetOutputScalarType(VTK_SIGNED_CHAR);};
    void SetOutputScalarTypeToUnsignedChar() {if (ComputeSigned) signErrorMacro; this->SetOutputScalarType(VTK_UNSIGNED_CHAR);};
    //@}

protected:
    vtkFastImplicitModeller();
    double GetScalarTypeMax(int type);

    int RequestInformation (vtkInformation *,
                                    vtkInformationVector **,
                                    vtkInformationVector *) VTK_OVERRIDE;
    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
    void Cap(vtkDataArray *s);

    int SampleDimensions[3];
    double MaximumDistance;
    double ModelBounds[6];
    int Capping;
    int FlipFaceOrder;
    double CapValue;
    int AdjustBounds;
    double AdjustDistance;
    int OutputScalarType;
    int ScaleToMaximumDistance;
    // flag to limit to one ComputeModelBounds per StartAppend
    int BoundsComputed;
    // the max distance computed during that one call
    double InternalMaxDistance;

    int ComputeSigned;

    int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
private:
    vtkFastImplicitModeller(const vtkFastImplicitModeller&) VTK_DELETE_FUNCTION;
    void operator=(const vtkFastImplicitModeller&) VTK_DELETE_FUNCTION;
};

#endif // VTKFASTIMPLICITMODELLER_H
