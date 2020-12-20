/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastImplicitModeller.cxx
  Author:    L.Cantelli & O.Cuisenaire @ HEIG-VD

=========================================================================*/

#include "vtkFastImplicitModeller.h"
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkImageIterator.h>
#include <vtkVector.h>
#include <vtkCellArray.h>
#include <valarray>

vtkStandardNewMacro(vtkFastImplicitModeller);

// INLINE OPERATORS - TRIVIAL, NO COMMENT
inline vtkVector3d operator*(const vtkVector3d& a, const vtkVector3d& b)
{
    vtkVector3d out;
    out.SetX(a.GetX() * b.GetX());    out.SetY(a.GetY() * b.GetY());    out.SetZ(a.GetZ() * b.GetZ());
    return out;
}

inline vtkVector3d operator+(const vtkVector3d& a, const vtkVector3d& b)
{
    vtkVector3d out;
    out.SetX(a.GetX() + b.GetX());    out.SetY(a.GetY() + b.GetY());    out.SetZ(a.GetZ() + b.GetZ());
    return out;
}

inline vtkVector3d operator*(const vtkVector3d& a, const double& b)
{
    vtkVector3d out;
    out.SetX(a.GetX() * b);    out.SetY(a.GetY() * b);    out.SetZ(a.GetZ() * b);
    return out;
}
inline vtkVector3d operator-(const vtkVector3d& a, const vtkVector3d& b)
{
    vtkVector3d out;
    out.SetX(a.GetX() - b.GetX());    out.SetY(a.GetY() - b.GetY());    out.SetZ(a.GetZ() - b.GetZ());
    return out;
}
// END INLINES

//----------------------------------------------------------------------------
template <class OT>
void SetOutputDistance(double distance, OT *outputValue, double capValue, double scaleFactor)
{
  // for now, just doing "normal" cast... could consider doing round?
  if (scaleFactor)  // need to scale the distance
  {
    *outputValue = static_cast<OT>(distance * scaleFactor);
  }
  else
  {
    if (capValue && distance > capValue) // clamping iff non-float type
    {
      distance = capValue;
    }
    *outputValue = static_cast<OT>(distance);
  }
}

//----------------------------------------------------------------------------
// Convert distance as stored in output (could be scaled and/or non-double
// type) to double distance with correct scaling
template <class OT>
void ConvertToDoubleDistance(const OT &inDistance, double &distance,
                             double &distance2, double scaleFactor)
{
  if (scaleFactor)
  {
    distance = inDistance * scaleFactor;
  }
  else
  {
    distance = inDistance;
  }
  distance2 = distance * distance;
}

template <typename T>
void MaurerPropagateEuclideanDistance(const T **inputImage, T **outputImage, const int* size, const double* _res, T maxDist)
{

    std::valarray<T> res(3); for(int i=0;i<3;i++) res[i] = T(_res[i]);		// CAST DISTANCE TO USED TYPE (PRECISION)
    std::valarray<T> res2(3); res2 = res*res;								// PRODUCE SQUARED DISTANCE
    T maxDist2 = maxDist*maxDist;                                           // AND SQUARED MAX DISTANCE

/* Scan along z */
// #pragma omp parallel
    {
    T *buffer = new T[size[2]];
    T newd;
// #pragma omp for
    for(int y=0;y<size[1];y++)
        for(int x=0,xy=y*size[0];x<size[0];x++,xy++)					// FOR EACH PIXEL COLUMN (x,y)
        {
            newd = buffer[0] = inputImage[0][xy];						// INITIAL DISTANCE IS THE STORED ONE (ONLY DEFINED ON ADJACENT VOXELS, = MAXDISTANCE FOR OTHER VOXELS)
            for(int z=1;z<size[2];z++)									// SCAN ALONG Z (FORWARDS)
            {
                buffer[z] = inputImage[z][xy];							// MAXDISTANCE OR EFFECTIVE DISTANCE (IF ADJACENT TO MESH CONTOUR)
                newd += res[2];											// ADD Z VOXEL LENGTH
                if(newd < buffer[z])									// THIS ALSO PROPAGATES DISTANCE TO VOXELS WHERE DISTANCE = MAXDISTANCE
                    buffer[z]=newd;
                else 													// IF THE STORED DISTANCE IS LESS THAN THE CURRENT ONE, UPDATE THE CURRENT DISTANCE
                    newd = buffer[z];
            }
            if(newd<maxDist) outputImage[size[2]-1][xy] = T(newd*newd);	// STORE DISTANCE FOR THE LAST VOXEL OF THE LINE
            for(int z=size[2]-2;z>=0;z--) 								// PROPAGATE ALONG Z (BACKWARDS)
            {
                newd += res[2];
                if(newd > buffer[z]) newd = buffer[z];					// IF COMPUTED DISTANCE IS BIGGER THAN OLD ONE, UPDATE COMPUTED DISTANCE
                if(newd<maxDist) outputImage[z][xy] = T(newd*newd);		// STORE UPDATED SQUARED DISTANCE
            }
        }
    delete [] buffer;

    /* Scan along y */

    int a,b,c,npt,k;
    double tst,d,d1,d2;
    int bSize = (size[0]>size[1] ? size[0]:size[1])+2;

    double *g= new double[bSize];
    int *h= new int[bSize];

// #pragma omp for
    for(int z=0;z<size[2];z++)
        for(int x=0;x<size[0];x++)										// FOR EACH PIXEL COLUMN (x,z)
        {
            npt = 0;
            for(int y=0,xy=x;y<size[1];y++,xy+=size[0])					// SCAN ALONG Y (FORWARDS)
            {
                d = outputImage[z][xy];									// GET CURRENT DISTANCE (X,Y,Z)
                if(d<maxDist2) 											// IF DISTANCE IS DEFINED (DIFFERENT THAN THE INIT VALUE)
                {
                    if(npt<2) { 										// FOR THE FIRST TWO VOXELS OF THE COLUMN
                        npt++; g[npt] = d; h[npt] = y;						// INCREMENT VOXEL COUNTER, STORE THE DISTANCE, STORE THE HEIGHT (IN THE VOXEL COLUMN)
                    } else { 											// FOR THE FOLLOWING VOXELS
                        a = h[npt] - h[npt-1];							// SET A = LAST VALUE HEIGHT - FORELAST VALUE HEIGHT
                        b = y - h[npt];									// SET B = HEIGHT - LAST VALUE HEIGHT
                        c = a+b;										// SET C = HEIGHT - FORELAST VALUE HEIGHT
                        tst = c * g[npt] - b * g[npt-1] - a * d - a*b*c*res2[1] ;

                        while( ( npt >=2 ) && ( tst > 0 ) ) {
                            npt--;
                            a = h[npt] - h[npt-1];
                            b = y - h[npt];
                            c = a+b;
                            tst = c * g[npt] - b * g[npt-1] - a * d - a*b*c*res2[1] ;
                        }
                        npt++; g[npt] = d; h[npt] = y;					// INCREMENT VOXEL COUNTER, STORE THE DISTANCE, STORE THE HEIGHT (IN THE VOXEL COLUMN)
                    }
                }
            }

            if(npt>0)													// IF DISTANCE WAS DEFINED FOR AT LEAST ONE VOXEL OF THE COLUMN
            {
                k=1;
                for(int y=0,xy=x;y<size[1];y++,xy+=size[0])
                {
                    d1 = g[k] + res2[1] * (h[k]-y) * (h[k]-y);
                    d2 = g[k+1] + res2[1] * (h[k+1]-y) * (h[k+1]-y);
                    while ( ( k < npt ) && ( d1 > d2 ) )
                    {
                        d1 = d2;
                        k++;
                        d2 = g[k+1] + res2[1] * (h[k+1]-y) * (h[k+1]-y);
                    }
                    if(d1<maxDist2) outputImage[z][xy] = T(d1);
                }
            }
        }

    /* Scan along x */

// #pragma omp for
    for(int z=0;z<size[2];z++)
        for(int y=0;y<size[1];y++)
        {
            npt = 0;
            for(int x=0,xy=y*size[0];x<size[0];x++,xy++)
            {
                d = outputImage[z][xy];
                if(d<maxDist2)
                {
                    if(npt<2) {
                        npt++; g[npt] = d; h[npt] = x;
                    } else {
                        a = h[npt] - h[npt-1];
                        b = x - h[npt];
                        c = a+b;
                        tst = c * g[npt] - b * g[npt-1] - a * d - a*b*c*res2[0] ;

                        while( ( npt >=2 ) && ( tst > 0 ) )
                        {
                            npt--;
                            a = h[npt] - h[npt-1];
                            b = x - h[npt];
                            c = a+b;
                            tst = c * g[npt] - b * g[npt-1] - a * d - a*b*c*res2[0] ;
                        }
                        npt++; g[npt] = d; h[npt] = x;
                    }
                }
            }

            if(npt>1)
            {
                k=1;
                for(int x=0,xy=y*size[0];x<size[0];x++,xy++)
                {
                    d1 = g[k] + res2[0] * (h[k]-x) * (h[k]-x);
                    d2 = g[k+1] + res2[0] * (h[k+1]-x) * (h[k+1]-x);
                    while ( ( k < npt ) && ( d1 > d2 ) )
                    {
                        d1 = d2;
                        k++;
                        d2 = g[k+1] + res2[0] * (h[k+1]-x) * (h[k+1]-x);
                    }
                    if(d1<maxDist2) outputImage[z][xy] = T(d1);
                }
            }
        }

    delete [] g;
    delete [] h;
    }
}

template <typename T>
void MaurerPropagateEuclideanDistance(const T *inputImage, T *outputImage, const int* size, const double* res, T maxDist)
{
    const T** in = new const T*[size[2]];
    T** out = new T*[size[2]];

    // GENERATE POINTERS TO IN/OUT SLICES
    for(int z=0;z<size[2];z++)
    {
        in[z] = inputImage+z*size[0]*size[1];
        out[z] = outputImage+z*size[0]*size[1];
    }

    // PROPAGATE DISTANCES
    MaurerPropagateEuclideanDistance<T>(in,out,size,res,maxDist);

    delete[] in;
    delete[] out;
}

//----------------------------------------------------------------------------
// Templated filter function
template <class OT>
void vtkFastImplicitModellerExecute(vtkFastImplicitModeller *self,
                                    vtkPolyData *mesh, vtkImageData *outData,
                                    double distanceLimit, OT *){
    vtkIdType npts, *pts;
    mesh->GetPolys()->InitTraversal();
    vtkPoints* cells = mesh->GetPoints();
    std::vector <vtkVector3d> pointTriplets;
    int *vsSize = self->GetSampleDimensions();
    vtkVector3d vsVoxelDimensions (outData->GetSpacing());

    int vsVoxelCount = vsSize[0] * vsSize[1] * vsSize[2];

    // set the origin of the voxelspace
    vtkVector3d vsOrigin (outData->GetOrigin());
    // set the end of the voxelspace
    vtkVector3d vsEnd (outData->GetBounds()[1], outData->GetBounds()[3], outData->GetBounds()[5]);

    double maxDistance = (vsSize [0] * vsVoxelDimensions.GetX() + vsSize [1] * vsVoxelDimensions.GetY() + vsSize [2] * vsVoxelDimensions.GetZ() );
    maxDistance *= maxDistance;

    // so we know how to scale if desired
    double scaleFactor = 0; // 0 used to indicate not scaling
    double toDoubleScaleFactor = 0; // 0 used to indicate not scaling
    double capValue = 0; // 0 used to indicate not clamping (float or double)
    if (self->GetOutputScalarType() != VTK_FLOAT &&
      self->GetOutputScalarType() != VTK_DOUBLE)
    {
      capValue = self->GetCapValue();
      if (self->GetScaleToMaximumDistance())
      {
        scaleFactor = capValue / distanceLimit;
        toDoubleScaleFactor = distanceLimit / capValue;
      }
    }

    // allocate an array to store the sign of voxels
    char *signs = new char[vsVoxelCount];
    if (!signs)
        vtkErrorWithObjectMacro(self, "Signs map allocation failed.");
    memset( signs, 0, vsVoxelCount * sizeof(char));
#ifdef WIN32
    double *outBuffer = (double*) _aligned_malloc(sizeof(double)*vsVoxelCount, 16);
#else
    double *outBuffer = (double*) aligned_alloc(16, sizeof(double)*vsVoxelCount);
#endif
    if (!outBuffer)
        vtkErrorWithObjectMacro(self, "Distance map allocation failed.");

// #pragma omp parallel for
    for( int elemCnt = 0; elemCnt < vsVoxelCount; ++elemCnt )
            outBuffer[ elemCnt ] = maxDistance;

    // build triangle list as triplets of points
    // get the vertices IDs for the next triangle
    while(mesh->GetPolys()->GetNextCell(npts,pts)) {
        double p0[3], p1[3], p2[3];
        // retrieve the triangle vertices
        cells->GetPoint(pts[0], p0);
        cells->GetPoint(pts[1], p1);
        cells->GetPoint(pts[2], p2);

        // fill the raw array of points representing triangles
        pointTriplets.push_back(vtkVector3d(p0));
        pointTriplets.push_back(vtkVector3d(p1));
        pointTriplets.push_back(vtkVector3d(p2));
    }

    // use the shortest side of a voxel as step
    double minStep = __min(__min(vsVoxelDimensions.GetX(),
                                 vsVoxelDimensions.GetY()),
                           vsVoxelDimensions.GetZ());

    // compute inverse resolution; aka the number of cells per space unit
    vtkVector3d inv_res = vtkVector3d(1.0 / vsVoxelDimensions.GetX(),
                                      1.0 / vsVoxelDimensions.GetY(),
                                      1.0 / vsVoxelDimensions.GetZ());

    // loop over triangles
// #pragma omp parallel for
    for (int t = 0; t < pointTriplets.size() / 3; t ++) {
        // store the current triangle vertices
        vtkVector3d Vertices[3];

        Vertices[0] = pointTriplets[t*3];
        Vertices[1] = pointTriplets[t*3+1];
        Vertices[2] = pointTriplets[t*3+2];

        // triangle A edge and scale factor
        vtkVector3d A = (Vertices[1] - Vertices[0]);
        double A_scale = minStep / sqrt(A.Dot(A));
        // resize A to minStep length (to run along it)
        A = A * A_scale;
        // compute number of steps along A
        double A_steps = 1.0 / A_scale + 1.0;

        // triangle's B edge and scale factor
        vtkVector3d B = (Vertices[2] - Vertices[0]);
        double B_scale = minStep / sqrt(B.Dot(B));
        // resize B to minStep length (to run along it)
        B = B * B_scale;
        // compute number of steps along B
        double B_steps = 1.0 / B_scale + 1.0;

        // triangle origin coordinates to voxel indexes
        vtkVector3d v1 = (Vertices[0] - vsOrigin) * inv_res;
        // convert A edge into voxel indexes shift
        A = A * inv_res;
        // convert B edge into voxel indexes shift
        B = B * inv_res;

        // run along A edge
        for (int curr_step_A = 0; curr_step_A <= A_steps; curr_step_A++) {
            vtkVector3d v2 = v1;
            // adjust number of steps along B at this position along A
            double adj_B_steps = (A_steps - curr_step_A) * (B_steps / A_steps);
            for (int curr_step_B = 0; curr_step_B <= adj_B_steps; curr_step_B++) {

                // compute adjacent voxels indexes
                int sx = __max(0,int(v2.GetX()-0.01));
                int ex = __min(int(v2.GetX()+2.01), vsSize[0]);
                int sy = __max(0,int(v2.GetY()-0.01));
                int ey = __min(int(v2.GetY()+2.01), vsSize[1]);
                int sz = __max(0,int(v2.GetZ()-0.01));
                int ez = __min(int(v2.GetZ()+2.01), vsSize[2]);

                // loop through all adjacent voxels on Z
                for (int z = sz; z < ez; z++){
                    for (int y = sy; y < ey; y++){
                        for (int x = sx; x < ex; x++){
                            // compute voxel index
                            int voxel_index = x + vsSize[0] * (y + vsSize[1] * z);
                            // flag voxel as adjacent
                            signs[voxel_index] = 2;
                        }
                    }
                }
                // progress along B
                v2 = v2 + B;
            }
            // progress along A
            v1 = v1 + A;
        }
    }

    // loop over triangles
// #pragma omp parallel for
    for (int t = 0; t < pointTriplets.size() / 3; t ++){
        // init triangle bounds (min = biggest)
        vtkVector3d minV = vsEnd;
        // init triangle bounds (max = smallest)
        vtkVector3d maxV = vsOrigin;

        // store the current triangle vertices
        vtkVector3d Vertices[3];
        Vertices[0] = pointTriplets[t*3];
        Vertices[1] = pointTriplets[t*3+1];
        Vertices[2] = pointTriplets[t*3+2];

        for (int v = 0; v < 3; v++){
            // update min (X, Y, Z) for this triangle
            minV.SetX(__min(minV.GetX(),Vertices[v].GetX()));
            minV.SetY(__min(minV.GetY(),Vertices[v].GetY()));
            minV.SetZ(__min(minV.GetZ(),Vertices[v].GetZ()));
            // update max (X, Y, Z) for this triangle
            maxV.SetX(__max(maxV.GetX(),Vertices[v].GetX()));
            maxV.SetY(__max(maxV.GetY(),Vertices[v].GetY()));
            maxV.SetZ(__max(maxV.GetZ(),Vertices[v].GetZ()));
        }
        // triangle bounds to voxelspace index (floating)
        minV = (minV - vsOrigin) * inv_res;
        maxV = (maxV - vsOrigin) * inv_res;

        // clip triangle's bounding box to fit voxelspace
        int minBox[3] = {__max(0, (int)(minV.GetX())),
                         __max(0, (int)(minV.GetY())),
                         __max(0, (int)(minV.GetZ()))};
        int maxBox[3] = {__min((int)vsSize[0], (int)(maxV.GetX() + 2)),
                         __min((int)vsSize[1], (int)(maxV.GetY() + 2)),
                         __min((int)vsSize[2], (int)(maxV.GetZ() + 2))};

        // compute triangle edges
        vtkVector3d A = Vertices[1] - Vertices[0];
        vtkVector3d B = Vertices[2] - Vertices[0];
        vtkVector3d C = Vertices[2] - Vertices[1];

        // inverse squared length
        double dotAAinv = 1.0 / A.Dot(A);
        double dotBBinv = 1.0 / B.Dot(B);
        double dotCCinv = 1.0 / C.Dot(C);

        // normal to triangle
        vtkVector3d normal = A.Cross(B);
        // inverse square length of normal
        double dotNNinv =  1.0 / normal.Dot(normal);

        double dotAB = A.Dot(B);

        vtkVector3d T = A * dotAB * dotAAinv;
        // normal to A edge
        vtkVector3d AP = (B - T);
        double APBinv = 1.0 / AP.Dot(B);

        T = B * dotAB * dotBBinv;
        // normal to B edge
        vtkVector3d BP = (A - T);
        double BPAinv = 1.0 / BP.Dot(A);

        for(int z = minBox[2]; z <= maxBox[2]; z++) {
            for(int y = minBox[1]; y <= maxBox[1]; y++) {
                // compute current point P
                vtkVector3d P = vtkVector3d (minBox[0] * vsVoxelDimensions.GetX() + vsOrigin.GetX() - Vertices[0].GetX(),
                                             y * vsVoxelDimensions.GetY() + vsOrigin.GetY() - Vertices[0].GetY(),
                                             z * vsVoxelDimensions.GetZ() + vsOrigin.GetZ() - Vertices[0].GetZ());
                // compute voxelspace index bounds
                int yzIndexShift = vsSize[0] * (y + vsSize[1] * z);
                int xyzMax = maxBox[0] + yzIndexShift;

                // for each voxel traversed by the current triangle
                for(int xyz = minBox[0] + yzIndexShift; xyz<=xyzMax; xyz++) {
                    if (signs[xyz] != 0){
                        double dotPN = P.Dot(normal);
                        vtkVector3d PN = normal * dotPN * dotNNinv;
                        double distToPlane = PN.Dot(PN);

                        if(distToPlane < outBuffer[xyz]) {
                            double newDist = maxDistance;
                            bool isInTriangle = true;

                            // compute point P projected on triangle plane
                            vtkVector3d PT = P - PN;

                            // compute P coordinates on the triangle
                            double alpha = PT.Dot(BP) * BPAinv;
                            double beta  = PT.Dot(AP) * APBinv;

                            // check if the point is outside the triangle
                            // => check outside in the A edge direction
                            if(beta < 0) {
                                double f = PT.Dot(A) * dotAAinv;
                                if(f > 1) f = 1;
                                else if(f < 0) f = 0;
                                T = PT - (A * f);
                                newDist = T.Dot(T);
                                isInTriangle = false;
                            }
                            // => check outside in the B edge direction
                            if(alpha < 0) {
                                double f = PT.Dot(B) * dotBBinv;
                                if(f > 1) f = 1;
                                else if(f < 0) f = 0;
                                T = PT - (B * f);
                                double dist = T.Dot(T);
                                newDist = __min(dist, newDist);
                                isInTriangle = false;
                            }

                            // => check outside in the C edge direction
                            if(alpha + beta > 1) {
                                PT = PT + Vertices[0] - Vertices[1];
                                double f = PT.Dot(C) * dotCCinv;
                                if(f > 1) f = 1;
                                else if(f < 0) f = 0;
                                T = PT - (C * f);
                                double dist = T.Dot(T);
                                newDist = __min(dist, newDist);
                                isInTriangle = false;
                            }

                            // if point it's not outside, then it's inside the triangle and distance is 0
                            if( isInTriangle ) {
                                newDist = 0;
                            }

                            // add distance to triangle plane
                            newDist += distToPlane;

                            // compute sign against the nearest mesh triangle
                            if(newDist <= outBuffer[xyz]) {
                                outBuffer[xyz] = newDist;
                                // set sign = 1 if voxel is outside the mesh (i.e. positive "normal dot")
                                if(dotPN >= 0) signs[xyz] = 1;
                                // set sign = -1 if voxel is inside the mesh (i.e. negative "normal dot")
                                else signs[xyz] = -1;
                            }
                        }
                    }
                    P.SetX(P.GetX() + vsVoxelDimensions.GetX());
                }
            }
        }
    }

    // propagate signs
// #pragma omp parallel for
    for (int z = 0; z < vsSize[2]; z++){
        for (int y = 0; y < vsSize[1]; y++){
            int idx = vsSize[0] * (y + vsSize[1] * z);
            // if sign is not set, set it to positive (outside)
            if ((signs[idx]==0) || (signs[idx]==2)){
                signs[idx] = 1;
            }
            idx++;
            for(int x = 1;x < vsSize[0]; x++, idx++){
                // if sign is not set, copy it from neighbour (this will detect borders)
                if((signs[idx]==0) || (signs[idx]==2))
                    signs[idx] = signs[idx-1];
            }
        }
    }

    // propagate distances -- Maurer euclidean distance transform
// #pragma omp parallel for
    for(int xyz=0;xyz<vsVoxelCount;xyz++){
        if(outBuffer[xyz]!=maxDistance)
            outBuffer[ xyz ] = sqrt(outBuffer[xyz]);
    }

    MaurerPropagateEuclideanDistance<double>(outBuffer, outBuffer, vsSize, vsVoxelDimensions.GetData(), maxDistance);

    // fill output
    OT* voxelspace = static_cast<OT *> (outData->GetScalarPointer());
    if (self->GetComputeSigned()){
// #pragma omp parallel for
        for( int elemCnt = 0; elemCnt < vsVoxelCount; ++elemCnt ){
             SetOutputDistance<OT> (sqrt(outBuffer[elemCnt]) * double(signs[elemCnt]), &voxelspace [elemCnt], capValue, scaleFactor);
        }
    }
    else {
// #pragma omp parallel for
        for( int elemCnt = 0; elemCnt < vsVoxelCount; ++elemCnt ){
             SetOutputDistance<OT> (sqrt(outBuffer[elemCnt]) * double(signs[elemCnt] == -1), &voxelspace [elemCnt], capValue, scaleFactor);
        }
    }

#ifdef WIN32
    _aligned_free(outBuffer);
#else
    free(outBuffer);
#endif
    delete signs;
}

//----------------------------------------------------------------------------
// Construct with sample dimensions=(50,50,50), and so that model bounds are
// automatically computed from the input. Capping is turned on with CapValue
// equal to a large positive number.
vtkFastImplicitModeller::vtkFastImplicitModeller()
{
  this->MaximumDistance = 0.1;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;
  this->BoundsComputed = 0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 1;
  this->FlipFaceOrder = 0;
  this->ComputeSigned = 1;
  this->OutputScalarType = VTK_FLOAT;
  this->CapValue = this->GetScalarTypeMax( this->OutputScalarType );
  this->ScaleToMaximumDistance = 0.1; // only used for non-float output type

  this->AdjustBounds = 1;
  this->AdjustDistance = 0.0125;

}

//----------------------------------------------------------------------------
void vtkFastImplicitModeller::SetOutputScalarType(int type)
{
  double scalarMax;

  vtkDebugMacro(<< this->GetClassName() << " (" << this <<
    "): setting OutputScalarType to " << type);

  scalarMax = this->GetScalarTypeMax(type);
  if (scalarMax) // legal type
  {
    int modified = 0;
    if (this->CapValue != scalarMax)
    {
      this->CapValue = scalarMax;
      modified = 1;
    }
    if (this->OutputScalarType != type)
    {
      this->OutputScalarType = type;
      modified = 1;
    }
    if (modified)
    {
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkFastImplicitModeller::SetCapValue(double value)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this <<
    "): setting CapValue to " << value);
  // clamp to between 0 and max for scalar type
  double max = this->GetScalarTypeMax(this->OutputScalarType);
  if (this->CapValue != (value < 0 ? 0 : (value > max ? max : value)))
  {
    this->CapValue = (value < 0 ? 0 : (value > max ? max : value));
    this->Modified();
  }
}

//----------------------------------------------------------------------------
double vtkFastImplicitModeller::GetScalarTypeMax(int type)
{
  switch (type)
  {
    case VTK_UNSIGNED_CHAR:  return (double)VTK_UNSIGNED_CHAR_MAX;
    case VTK_SIGNED_CHAR:    return (double)VTK_SIGNED_CHAR_MAX;
    case VTK_UNSIGNED_SHORT: return (double)VTK_UNSIGNED_SHORT_MAX;
    case VTK_SHORT:          return (double)VTK_SHORT_MAX;
    case VTK_UNSIGNED_INT:   return (double)VTK_UNSIGNED_INT_MAX;
    case VTK_INT:            return (double)VTK_INT_MAX;
    case VTK_UNSIGNED_LONG:  return (double)VTK_UNSIGNED_LONG_MAX;
    case VTK_LONG:           return (double)VTK_LONG_MAX;
    case VTK_FLOAT:          return (double)VTK_FLOAT_MAX;
    case VTK_DOUBLE:         return (double)VTK_DOUBLE_MAX;
    default: return 0;
  }
}

//----------------------------------------------------------------------------
int vtkFastImplicitModeller::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, 1);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, this->SampleDimensions[0]-1,
               0, this->SampleDimensions[1]-1,
               0, this->SampleDimensions[2]-1);

  for (i=0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2*i];
    if ( this->SampleDimensions[i] <= 1 )
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkFastImplicitModeller::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{
    // get the input
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkPolyData *input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));

    vtkDebugMacro(<< "Executing implicit model");

    vtkIdType numPts;
    vtkIdType i;
    double maxDistance;

    vtkInformation* outInfo = this->GetOutputInformation(0);
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      vtkStreamingDemandDrivenPipeline::GetWholeExtent(outInfo),
      6);

    vtkDebugMacro(<< "Initializing data");
    this->AllocateOutputData(this->GetOutput(), this->GetOutputInformation(0));
    this->UpdateProgress(0.0);

    numPts = this->SampleDimensions[0] * this->SampleDimensions[1]
               * this->SampleDimensions[2];

    // initialize output to CapValue at each location
    maxDistance = this->CapValue;
    vtkDataArray *newScalars = this->GetOutput()->GetPointData()->GetScalars();
    for (i=0; i<numPts; i++){
      newScalars->SetComponent(i, 0, maxDistance);
    }

    vtkDebugMacro(<< "Appending data");

    vtkImageData *output = this->GetOutput();

    if ( !this->BoundsComputed ){
      this->ComputeModelBounds(input);
    }

    if (!output->GetPointData()->GetScalars()){
        vtkErrorMacro("Sanity check failed.");
    }

    switch (this->OutputScalarType){
        vtkTemplateMacro(
          vtkFastImplicitModellerExecute( this,
                                          input,
                                          output,
                                          this->InternalMaxDistance,
                                          static_cast<VTK_TT *>(0)));
    }
    vtkDebugMacro(<< "End append");

    if (!(newScalars = this->GetOutput()->GetPointData()->GetScalars())){
        vtkErrorMacro("Sanity check failed.");
    }

    if ( this->Capping ){
        this->Cap(newScalars);
    }
    this->UpdateProgress(1.0);
    /* END EndAppend(); */

    return 1;
}

//----------------------------------------------------------------------------
// Compute ModelBounds from input geometry.
double vtkFastImplicitModeller::ComputeModelBounds(vtkPolyData *input)
{
  const double * bounds;
  double maxDist;
  int i;
  vtkImageData *output=this->GetOutput();
  double tempd[3];

  // compute model bounds if not set previously
  if ( this->ModelBounds[0] >= this->ModelBounds[1] ||
       this->ModelBounds[2] >= this->ModelBounds[3] ||
       this->ModelBounds[4] >= this->ModelBounds[5] )
  {
    if (input != nullptr)
    {
      bounds = input->GetBounds();
    }
    else
    {
      vtkDataSet *dsInput = vtkDataSet::SafeDownCast(this->GetInput());
      if (dsInput != nullptr)
      {
        bounds = dsInput->GetBounds();
      }
      else
      {
        vtkErrorMacro(
          << "An input must be specified to Compute the model bounds.");
        return VTK_FLOAT_MAX;
      }
    }
  }
  else
  {
    bounds = this->ModelBounds;
  }

  for (maxDist=0.0, i=0; i<3; i++)
  {
    if ( (bounds[2*i+1] - bounds[2*i]) > maxDist )
    {
      maxDist = bounds[2*i+1] - bounds[2*i];
    }
  }

  // adjust bounds so model fits strictly inside (only if not set previously)
  if ( this->AdjustBounds )
  {
    for (i=0; i<3; i++)
    {
      this->ModelBounds[2*i] = bounds[2*i] - maxDist*this->AdjustDistance;
      this->ModelBounds[2*i+1] = bounds[2*i+1] + maxDist*this->AdjustDistance;
    }
  }
  else  // to handle problem case where bounds not specified and AdjustBounds
    //  not on; will be setting ModelBounds to self if previosusly set
  {
    for (i=0; i<3; i++)
    {
      this->ModelBounds[2*i] = bounds[2*i];
      this->ModelBounds[2*i+1] = bounds[2*i+1];
    }
  }

  maxDist *= this->MaximumDistance;

  // Set volume origin and data spacing
  output->SetOrigin(this->ModelBounds[0],
                    this->ModelBounds[2],
                    this->ModelBounds[4]);

  for (i=0; i<3; i++)
  {
    tempd[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
  }
  output->SetSpacing(tempd);

  vtkInformation *outInfo = this->GetExecutive()->GetOutputInformation(0);
  outInfo->Set(vtkDataObject::ORIGIN(),this->ModelBounds[0],
               this->ModelBounds[2], this->ModelBounds[4]);
  outInfo->Set(vtkDataObject::SPACING(),tempd,3);

  this->BoundsComputed = 1;
  this->InternalMaxDistance = maxDist;

  return maxDist;
}

//----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void vtkFastImplicitModeller::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void vtkFastImplicitModeller::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] ||
       dim[1] != this->SampleDimensions[1] ||
       dim[2] != this->SampleDimensions[2] )
  {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
    {
      vtkErrorMacro (<< "Bad Sample Dimensions, retaining previous values");
      return;
    }

    for (dataDim=0, i=0; i<3 ; i++)
    {
      if (dim[i] > 1)
      {
        dataDim++;
      }
    }

    if ( dataDim  < 3 )
    {
      vtkErrorMacro(<<"Sample dimensions must define a volume!");
      return;
    }

    for ( i=0; i<3; i++)
    {
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkFastImplicitModeller::Cap(vtkDataArray *s)
{
  int i,j,k;
  int idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  for (j=0; j<this->SampleDimensions[1]; j++)
  {
    for (i=0; i<this->SampleDimensions[0]; i++)
    {
      s->SetComponent(i+j*this->SampleDimensions[0],0, this->CapValue);
    }
  }
  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
  {
    for (i=0; i<this->SampleDimensions[0]; i++)
    {
      s->SetComponent(idx+i+j*this->SampleDimensions[0], 0, this->CapValue);
    }
  }
  // j-k planes
  for (k=0; k<this->SampleDimensions[2]; k++)
  {
    for (j=0; j<this->SampleDimensions[1]; j++)
    {
      s->SetComponent(j*this->SampleDimensions[0]+k*d01,0,this->CapValue);
    }
  }
  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
  {
    for (j=0; j<this->SampleDimensions[1]; j++)
    {
      s->SetComponent(i+j*this->SampleDimensions[0]+k*d01,0, this->CapValue);
    }
  }
  // i-k planes
  for (k=0; k<this->SampleDimensions[2]; k++)
  {
    for (i=0; i<this->SampleDimensions[0]; i++)
    {
      s->SetComponent(i+k*d01,0, this->CapValue);
    }
  }
  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
  {
    for (i=0; i<this->SampleDimensions[0]; i++)
    {
      s->SetComponent(idx+i+k*d01,0, this->CapValue);
    }
  }
}

//----------------------------------------------------------------------------
int vtkFastImplicitModeller::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkFastImplicitModeller::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", "
     << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", "
     << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", "
     << this->ModelBounds[5] << ")\n";

  os << indent << "ScaleToMaximumDistance: " << (this->ScaleToMaximumDistance ? "On\n" : "Off\n");
  os << indent << "AdjustBounds: " << (this->AdjustBounds ? "On\n" : "Off\n");
  os << indent << "Adjust Distance: " << this->AdjustDistance << "\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";
}
