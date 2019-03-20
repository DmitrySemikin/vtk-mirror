/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastLabeledDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFastLabeledDataMapper.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkFreeTypeTools.h"
#include "vtkImageAppend.h"
#include "vtkImageClip.h"
#include "vtkImageConstantPad.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTextureObject.h"
#include "vtkTimerLog.h"
#include "vtkUnicodeStringArray.h"
#include "vtkWindow.h"
#include "vtkXMLPolyDataWriter.h"

#include <array>

// Colsize is a heuristic to move to the next row in the word texture.
// Keeping the texture roughly square gives better texture sampling
// accuracy (less pixelization) as we use bits from both U and V.
// It matters much more with per word approach than with per char
// where number of elements grows more quickly.
#define COLSIZE 10

// glsl has restrictions on variable arrays indexing so we put a limit on
// the number of vtkTextProperties
#define MAXPROPS 32
#define SMAXPROPS "32"

// a buffer zone around each glyph in the texture atlas to avoid
// bleedthrough
#define PADSZ 5
#define SPADSZ "5"

typedef struct
{
  int propid; // id of TextProperty that created this
  vtkSmartPointer<vtkImageData> Texture;
} WordRecord;

class vtkFastLabeledDataMapper::Internals
{
public:
  Internals()
  {
    this->glyphExtents->SetNumberOfComponents(4);
    this->glyphExtents->SetName("glyphExtents");
    this->coff->SetNumberOfComponents(1);
    this->coff->SetName("coff");
    this->pid->SetNumberOfComponents(1);
    this->pid->SetName("pid");
    this->propid->SetNumberOfComponents(1);
    this->propid->SetName("propid");
    this->framecolors->SetNumberOfComponents(3);
    this->framecolors->SetName("framecolors");
    this->ImageAppender->PreserveExtentsOn();
    this->TextProperties[0] = vtkSmartPointer<vtkTextProperty>::New();
#if 0
    if (getenv("MSAMP")) {
       int ns = atoi(getenv("MSAMP"));
       cerr << "MS " << ns << endl;
       this->glyphsTO->SetSamples(ns);
    }
    if (getenv("MMAP")) {
       cerr << "MMAP ON" << endl;
      this->glyphsTO->SetGenerateMipmap(true);
    }
    if (getenv("MMIN")) {
       cerr << "MIN ON" << endl;
       this->glyphsTO->SetMagnificationFilter(vtkTextureObject::Linear);
    }
    if (getenv("MMAX")) {
      cerr << "MMAX ON" << endl;
      this->glyphsTO->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
    }
#endif
  }
  //----------------------------------------------------------------------------
  ~Internals()
  {
    this->InputPlusArrays->Initialize(); // break refs to dataarrays
  }

  //----------------------------------------------------------------------------
  void FreshIPA()
  {
    this->InputPlusArrays->Initialize();
    this->InputPlusArrays->Allocate();
    this->glyphExtents->SetNumberOfTuples(0);
    this->coff->SetNumberOfTuples(0);
    this->pid->SetNumberOfTuples(0);
    this->propid->SetNumberOfTuples(0);
    this->framecolors->SetNumberOfTuples(0);
  }

  //----------------------------------------------------------------------------
  WordRecord MakeWordTexture(vtkStdString word, vtkTextProperty* prop, int propID)
  {
    auto nchar = vtkSmartPointer<vtkImageData>::New();
    int textdims[2];

    // temporily make settings to get a consistent result
    auto tren = vtkFreeTypeTools::GetInstance();
    bool lasts2p2 = tren->GetScaleToPowerTwo();
    tren->ScaleToPowerTwoOff();
    bool iWasFramed = prop->GetFrame();
    prop->FrameOff();

    // ask freetype for a texture for this word
    tren->RenderString(prop, word, this->DPI, nchar, textdims);

    if (this->Descenders[propID] < 0)
    {
      auto faceMetrics = tren->GetFaceMetrics(prop);
      int descender = -faceMetrics.Descender * prop->GetFontSize() / faceMetrics.UnitsPerEM;
      this->Descenders[propID] = descender;
    }

    // restore what input settings
    prop->SetFrame(iWasFramed);
    tren->SetScaleToPowerTwo(lasts2p2);

    // what have we got?
    int charExt[6];
    nchar->GetExtent(charExt);
    double bg[4];
    prop->GetBackgroundColor(bg);
    bg[0] = bg[0] * 255; // to uchar
    bg[1] = bg[1] * 255;
    bg[2] = bg[2] * 255;
    bg[3] = prop->GetBackgroundOpacity() * 255;

    // freetypetools renderstring pads with BG, which messes up our spacing, so unpad
    // see vtkFreeTypeTools::CalculateBoundingBox
    int clipPix = ((bg[3] > 0) ? 2 : 0);
    this->ImageClipper->SetInputData(nchar);
    this->ImageClipper->SetOutputWholeExtent(
      charExt[0] + clipPix, charExt[1] - clipPix, charExt[2] + clipPix, charExt[3] - clipPix, 0, 0);

    // make a safety zone to prevent bleed through
    this->ImagePadder->SetInputConnection(this->ImageClipper->GetOutputPort());
    this->ImagePadder->SetConstants(4, bg);
    this->ImagePadder->SetOutputWholeExtent(charExt[0] + clipPix - PADSZ,
      charExt[1] - clipPix + PADSZ, charExt[2] + clipPix - PADSZ, charExt[3] - clipPix + PADSZ, 0,
      0);
    this->ImagePadder->Update();
    vtkImageData* outI;
    outI = this->ImagePadder->GetOutput();
    outI->GetExtent(charExt);
    nchar->ShallowCopy(outI);

#if 0
    auto writer = vtkSmartPointer<vtkPNGWriter>::New();
    std::string fname = "/home/demarle/Desktop/foo";
    fname += word;
    fname += "_";
    fname += std::to_string(propID);
    fname += ".png";
    writer->SetFileName(fname.c_str());
    writer->SetInputData(nchar);
    writer->Write();
#endif

    WordRecord wr;
    wr.propid = propID;
    wr.Texture = nchar;

    return wr;
  }

  //----------------------------------------------------------------------------
  void AppendToWordTexture(vtkImageData* in, int propIdx, int& sx, int& sy, int& ex, int& ey)
  {
    // in practice this just gets ready to append the word, we defer the actual
    // texture construction to MakeItSo so we can do the whole thing at once
    // very quickly

    // Pad the calculations so that the glyphs do not overlap:
    constexpr int PAD = 1;

    // where this new word goes
    int x0;
    int x1;
    int y0;
    int y1;
    int wordsdims[3];
    in->GetDimensions(wordsdims);

    bool nextcolumn = false;
    if (jpos == COLSIZE)
    {
      // We are trying to keep the font texture roughly square
      // because that cuts down on texture sampling precision issues
      // i.e. spread out the precision over two dimensions instead of one
      nextcolumn = true;
    }
    if (nextcolumn)
    {
      // this word's spot in the texture
      x0 = ex;
      x1 = ex + wordsdims[0] - 1;
      y0 = 0;
      y1 = 0 + wordsdims[1] - 1;
      // next spot in texture
      sx = ex;
      ex = std::max(ex, x1 + PAD);
      sy = 0;
      ey = y1 + PAD;
    }
    else
    {
      // this word's spot in the texture
      x0 = sx;
      x1 = sx + wordsdims[0] - 1;
      y0 = ey;
      y1 = ey + wordsdims[1] - 1;
      // next spot in texture
      sx = sx;
      ex = std::max(ex, x1 + PAD);
      sy = ey;
      ey = y1 + PAD;
    }

    in->SetExtent(x0, x1, y0, y1, 0, 0);
    this->MaxGlyphHeights[propIdx] =
      std::max(this->MaxGlyphHeights[propIdx], y1 - y0 + 1 - (2 * PADSZ));

    if (jpos == COLSIZE)
    {
      ipos++;
      jpos = 0;
    }
    else
    {
      jpos++;
    }
  }

  //----------------------------------------------------------------------------
  void MakeItSo()
  {
    // aggregate actions of appendwordtotexture
    this->ImageAppender->RemoveAllInputs();
    std::map<std::pair<vtkStdString, int>, WordRecord>::iterator it = this->AllStrings.begin();
    int cnt = 0;
    while (it != this->AllStrings.end())
    {
      cnt++;
      this->ImageAppender->AddInputData(it->second.Texture);
      if (!(cnt % 10000))
      {
        cnt = 0;
        this->ImageAppender->Update();
        auto tempID = vtkSmartPointer<vtkImageData>::New();
        tempID->DeepCopy(this->ImageAppender->GetOutput());
        this->ImageAppender->RemoveAllInputs();
        this->ImageAppender->AddInputData(tempID);
      }
      it++;
    }
    this->ImageAppender->Update();
    this->WordsTexture = this->ImageAppender->GetOutput();

#if 0
    // debugging: Add a checkerboard pattern to the texture:
    int *dims = this->WordsTexture->GetDimensions();
    for (int i = 0; i < dims[0]; ++i)
    {
      for (int j = 0; j < dims[1]; ++j)
      {
        if ((i + j) % 2)
        {
          void *dptr = this->WordsTexture->GetScalarPointer(i, j, 0);
          auto ptr = reinterpret_cast<unsigned char *>(dptr);
          ptr[0] = 255;
          ptr[1] = 0;
          ptr[2] = 0;
          ptr[3] = 255;
        }
      }
    }
#endif
  }

  void UploadTexture()
  {
    int* dims = this->WordsTexture->GetDimensions();
    this->glyphsTO->Create2DFromRaw(static_cast<unsigned int>(dims[0]),
      static_cast<unsigned int>(dims[1]), 4, VTK_UNSIGNED_CHAR,
      this->WordsTexture->GetScalarPointer());
  }

  void UpdateTextPropertyAttributeArrays()
  {
    std::array<double, 4> bg{ 0. };
    std::array<double, 4> frame{ 0. };
    int frameWidth = 0;

    for (std::size_t i = 0; i < MAXPROPS; ++i)
    {
      vtkTextProperty* prop = this->TextProperties[i].Get();
      if (prop)
      {
        prop->GetBackgroundColor(bg.data());
        bg[3] = prop->GetBackgroundOpacity();
        prop->GetFrameColor(frame.data());
        frame[3] = 1.;
        frameWidth = prop->GetFrame() ? prop->GetFrameWidth() : 0;
      }
      this->BackgroundColors[i][0] = static_cast<float>(bg[0]);
      this->BackgroundColors[i][1] = static_cast<float>(bg[1]);
      this->BackgroundColors[i][2] = static_cast<float>(bg[2]);
      this->BackgroundColors[i][3] = static_cast<float>(bg[3]);
      this->FrameWidths[i] = frameWidth;
    }
  }

  vtkNew<vtkImageAppend> ImageAppender;
  vtkNew<vtkImageConstantPad> ImagePadder;
  vtkNew<vtkImageClip> ImageClipper;
  vtkNew<vtkPolyData> InputPlusArrays;
  vtkNew<vtkIntArray> glyphExtents;
  vtkNew<vtkFloatArray> coff;
  vtkNew<vtkIdTypeArray> pid;
  vtkNew<vtkFloatArray> propid;
  vtkNew<vtkFloatArray> framecolors;
  int ipos = 0;
  int jpos = 0;
  std::map<std::pair<vtkStdString, int>, WordRecord> AllStrings;
  vtkSmartPointer<vtkImageData> WordsTexture;
  vtkNew<vtkTextureObject> glyphsTO;
  int DPI = 72;

  std::array<vtkSmartPointer<vtkTextProperty>, MAXPROPS> TextProperties;

  // Must use C arrays to mesh with vtkShaderProgram's API:
  float BackgroundColors[MAXPROPS][4];
  int FrameWidths[MAXPROPS];
  int MaxGlyphHeights[MAXPROPS]; // max glyph height per text property
  int Descenders[MAXPROPS];
};

//----------------------------------------------------------------------
vtkStandardNewMacro(vtkFastLabeledDataMapper);

//----------------------------------------------------------------------

template <typename T>
void vtkFastLabeledDataMapper_PrintComponent(
  char* output, size_t outputSize, const char* format, int index, const T* array)
{
  snprintf(output, outputSize, format, array[index]);
}

//----------------------------------------------------------------------------
vtkFastLabeledDataMapper::vtkFastLabeledDataMapper()
{
  this->Implementation = new Internals;

  this->Input = nullptr;
  this->LabelMode = VTK_LABEL_IDS;

  this->LabelFormat = nullptr;

  this->LabeledComponent = (-1);
  this->FieldDataArray = 0;
  this->FieldDataName = nullptr;

  this->NumberOfLabelsAllocated = 0;
  this->AllocateLabels(50);

  this->ComponentSeparator = ' ';

  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "type");

  auto prop = vtkSmartPointer<vtkTextProperty>::New();
  prop->SetFontFamilyAsString("Arial");
  prop->SetFontSize(24);
  prop->SetColor(1.0, 1.0, 1.0);
  prop->SetBackgroundColor(1.0, 0.0, 0.0);
  this->SetLabelTextProperty(prop);

  this->MakeupShaders();
  this->SetPointIdArrayName("pid");

  this->FrameColorsName = nullptr;

  this->TextAnchor = vtkFastLabeledDataMapper::Center;
}

//----------------------------------------------------------------------------
vtkFastLabeledDataMapper::~vtkFastLabeledDataMapper()
{
  this->SetFieldDataName(nullptr);
  delete this->Implementation;
  this->SetFrameColorsName(nullptr);
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input: (" << this->Input << ")\n";
  }
  else
  {
    os << indent << "Input: (none)\n";
  }

  os << indent << "Label Mode: ";
  if (this->LabelMode == VTK_LABEL_IDS)
  {
    os << "Label Ids\n";
  }
  else if (this->LabelMode == VTK_LABEL_SCALARS)
  {
    os << "Label Scalars\n";
  }
  else if (this->LabelMode == VTK_LABEL_VECTORS)
  {
    os << "Label Vectors\n";
  }
  else if (this->LabelMode == VTK_LABEL_NORMALS)
  {
    os << "Label Normals\n";
  }
  else if (this->LabelMode == VTK_LABEL_TCOORDS)
  {
    os << "Label TCoords\n";
  }
  else if (this->LabelMode == VTK_LABEL_TENSORS)
  {
    os << "Label Tensors\n";
  }
  else
  {
    os << "Label Field Data\n";
  }

  os << indent << "Label Format: " << (this->LabelFormat ? this->LabelFormat : "Null") << "\n";

  os << indent << "Labeled Component: ";
  if (this->LabeledComponent < 0)
  {
    os << "(All Components)\n";
  }
  else
  {
    os << this->LabeledComponent << "\n";
  }

  os << indent << "Field Data Array: " << this->FieldDataArray << "\n";
  os << indent << "Field Data Name: " << (this->FieldDataName ? this->FieldDataName : "Null")
     << "\n";
}

//----------------------------------------------------------------------------
int vtkFastLabeledDataMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::SetMapperShaderParameters(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor)
{
  vtkShaderProgram* program = cellBO.Program;
  program->SetUniform2i("atlasDims", this->Implementation->WordsTexture->GetDimensions());
  program->SetUniform2i("vpDims", ren->GetSize());
  program->SetUniform2i("winDims", ren->GetRenderWindow()->GetSize());
  program->SetUniformi("atlasTex", this->Implementation->glyphsTO->GetTextureUnit());
  int anchorCenter[2] = { -1, -1 };
  switch (this->TextAnchor)
  {
    case LowerLeft:
    default:
      break;
    case LowerEdge:
      anchorCenter[0] = 0;
      break;
    case LowerRight:
      anchorCenter[0] = 1;
      break;
    case LeftEdge:
      anchorCenter[1] = 0;
      break;
    case Center:
      anchorCenter[0] = 0;
      anchorCenter[1] = 0;
      break;
    case RightEdge:
      anchorCenter[0] = 1;
      anchorCenter[1] = 0;
      break;
    case UpperLeft:
      anchorCenter[1] = 1;
      break;
    case UpperEdge:
      anchorCenter[0] = 0;
      anchorCenter[1] = 1;
      break;
    case UpperRight:
      anchorCenter[0] = 1;
      anchorCenter[1] = 1;
      break;
  }
  program->SetUniform2i("anchorCenter", anchorCenter);

  double vp[4];
  ren->GetViewport(vp);
  float vpf[4] = { static_cast<float>(vp[0]), static_cast<float>(vp[1]), static_cast<float>(vp[2]),
    static_cast<float>(vp[3]) };
  program->SetUniform4f("vp", vpf);

  double tileVP[4];
  ren->GetRenderWindow()->GetTileViewport(tileVP);
  float nvpf[4] = { static_cast<float>(std::max(vp[0], tileVP[0])),
    static_cast<float>(std::max(vp[1], tileVP[1])), static_cast<float>(std::min(vp[2], tileVP[2])),
    static_cast<float>(std::min(vp[3], tileVP[3])) };
  program->SetUniform4f("nvp", nvpf);

  program->SetUniform4fv("BackgroundColors", MAXPROPS, this->Implementation->BackgroundColors);
  program->SetUniform1iv("FrameWidths", MAXPROPS, this->Implementation->FrameWidths);
  program->SetUniform1iv("MaxGlyphHeights", MAXPROPS, this->Implementation->MaxGlyphHeights);
  program->SetUniform1iv("Descenders", MAXPROPS, this->Implementation->Descenders);

  this->Superclass::SetMapperShaderParameters(cellBO, ren, actor);
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::SetInputData(vtkDataSet* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkFastLabeledDataMapper::GetInput()
{
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::SetLabelTextProperty(vtkTextProperty* prop, int type)
{
  if (type >= MAXPROPS)
  {
    vtkErrorMacro(
      "Maximum number of text properties exceeded (" << type << " >= " << MAXPROPS << ").");
    return;
  }

  this->Implementation->TextProperties[type] = prop;
  this->Implementation->AllStrings.clear();
  std::fill(this->Implementation->Descenders, this->Implementation->Descenders + MAXPROPS, -1);
  this->Implementation->WordsTexture = vtkSmartPointer<vtkImageData>::New();
  this->MapDataArrayToVertexAttribute(
    "glyphExtentsVS", "glyphExtents", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->MapDataArrayToVertexAttribute("coff", "coff", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->MapDataArrayToVertexAttribute("propid", "propid", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->MapDataArrayToVertexAttribute(
    "framecolors", "framecolors", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkFastLabeledDataMapper::GetLabelTextProperty(int type)
{
  if (type >= MAXPROPS)
  {
    vtkErrorMacro(
      "Maximum number of text properties exceeded (" << type << " >= " << MAXPROPS << ").");
    return nullptr;
  }

  return this->Implementation->TextProperties[type];
}

//----------------------------------------------------------------------
void vtkFastLabeledDataMapper::SetFieldDataArray(int arrayIndex)
{
  delete[] this->FieldDataName;
  this->FieldDataName = nullptr;

  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting FieldDataArray to "
                << arrayIndex);

  if (this->FieldDataArray !=
    (arrayIndex < 0 ? 0 : (arrayIndex > VTK_INT_MAX ? VTK_INT_MAX : arrayIndex)))
  {
    this->FieldDataArray =
      (arrayIndex < 0 ? 0 : (arrayIndex > VTK_INT_MAX ? VTK_INT_MAX : arrayIndex));
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkFastLabeledDataMapper::SetFieldDataName(const char* arrayName)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting "
                << "FieldDataName"
                << " to " << (arrayName ? arrayName : "(null)"));

  if (this->FieldDataName == nullptr && arrayName == nullptr)
  {
    return;
  }
  if (this->FieldDataName && arrayName && (!strcmp(this->FieldDataName, arrayName)))
  {
    return;
  }
  delete[] this->FieldDataName;
  if (arrayName)
  {
    this->FieldDataName = new char[strlen(arrayName) + 1];
    strcpy(this->FieldDataName, arrayName);
  }
  else
  {
    this->FieldDataName = nullptr;
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::MakeupShaders()
{
  this->AddShaderReplacement(vtkShader::Vertex,
    "//VTK::Normal::Dec",  // replace the normal block
    true,                  // before the standard replacements
    "//VTK::Normal::Dec\n" // we still want the default
    "\n"
    "//fast text declaration\n"
    "in vec4 glyphExtentsVS;\n" // texture coordinates of glyph
    "in float coff;\n"          // offset of character's start within word texture
    "in float propid;\n"        // index of textproperty info
    "in vec3 framecolors;\n"

    // passing info to next shader
    "out ivec4 glyphExtentsGS;\n"
    "out float COFF;\n"
    "out int PROPID;\n"
    "out vec3 framecolorsGS;\n",
    false // only do it once
  );

  this->AddShaderReplacement(vtkShader::Vertex,
    "//VTK::Normal::Impl",  // replace the normal block
    true,                   // before the standard replacements
    "//VTK::Normal::Impl\n" // we still want the default
    "\n"
    "  //fast text implementation\n"
    // Pass in the glyph extent as float then cast to int because
    // vtkOpenGLPolyDataMapper::MapDataArrayToVertexAttibute assumes all glsl
    // attributes are floats...
    // Add 0.5 for rounding during implicit 'floor' during float -> int conv.
    "  glyphExtentsGS = ivec4(glyphExtentsVS + vec4(0.5f));\n"
    "  COFF = coff;\n"
    "  PROPID = int(propid);\n"
    "  framecolorsGS = framecolors;\n",
    false // only do it once
  );

  std::string geomp_str(
    "//VTK::System::Dec\n"
    "layout(points) in;\n"
    "//VTK::Output::Dec\n"
    "//VTK::Picking::Dec\n"
    "layout(triangle_strip, max_vertices = 18) out;\n"
    "in ivec4[] glyphExtentsGS;\n" // glyph {xmin, xmax, ymin, ymax} idxs in atlas
    "in float[] COFF;\n"
    "in int[] PROPID;\n"
    "in vec3[] framecolorsGS;\n"
    "flat out int FPROPID;\n"
    "flat out vec3 framecolorsFS;\n"
    "out vec2 UV2;\n"
    "flat out int layer;\n" // 0: glyph 1: bg 2: frame
    "uniform int FrameWidths[" SMAXPROPS "];\n"
    "uniform int MaxGlyphHeights[" SMAXPROPS "];\n"
    "uniform int Descenders[" SMAXPROPS "];\n"
    "uniform vec4 vp;\n"            // local viewport (xmin, ymin, xmax, ymax) range (0.0, 1.0)
    "uniform vec4 nvp;\n"           // normalized viewport (accounts for tile vps)
    "uniform ivec2 vpDims;\n"       // viewport dimensions
    "uniform ivec2 winDims;\n"      // window dimensions
    "uniform ivec2 atlasDims;\n"    // glyph atlas dimensions
    "uniform ivec2 anchorCenter;\n" // centering options, x and y

    // Viewport offset in pixels.
    "vec2 vpOff = vec2(vp[0] * winDims[0] + 0.5,\n"
    "                  vp[1] * winDims[1] + 0.5);\n"
    "\n"

    // Convert between relevant coordinate systems:
    "vec4 ClipCoordToDisplayCoord(vec4 clipCoord)\n"
    "{\n"
    "  vec4 dispCoord = clipCoord;\n"
    "  dispCoord.xyz /= dispCoord.w;\n"
    "  dispCoord.x = nvp[0] + ((dispCoord.x + 1.) / 2.) * (nvp[2] - nvp[0]);\n"
    "  dispCoord.y = nvp[1] + ((dispCoord.y + 1.) / 2.) * (nvp[3] - nvp[1]);\n"
    "  dispCoord.x = (dispCoord.x - vp[0]) / (vp[2] - vp[0]);\n"
    "  dispCoord.y = (dispCoord.y - vp[1]) / (vp[3] - vp[1]);\n"
    "  dispCoord.x *= vpDims.x - 1.;\n"
    "  dispCoord.y *= vpDims.y - 1.;\n"
    "  dispCoord.xy += vpOff.xy;\n"
    "  return dispCoord;\n"
    "}\n"
    "\n"
    "vec4 DisplayCoordToClipCoord(vec4 dispCoord)\n"
    "{\n"
    "  vec4 clipCoord = dispCoord;\n"
    "  clipCoord.xy -= vpOff.xy;\n"
    "  clipCoord.x /= vpDims.x - 1;\n"
    "  clipCoord.y /= vpDims.y - 1;\n"
    "  clipCoord.x = clipCoord.x * (vp[2] - vp[0]) + vp[0];\n"
    "  clipCoord.y = clipCoord.y * (vp[3] - vp[1]) + vp[1];\n"
    "  clipCoord.x = 2. * (clipCoord.x - nvp[0]) / (nvp[2] - nvp[0]) - 1.;\n"
    "  clipCoord.y = 2. * (clipCoord.y - nvp[1]) / (nvp[3] - nvp[1]) - 1.;\n"
    "  clipCoord.xyz *= clipCoord.w;\n"
    "  return clipCoord;\n"
    "}\n"
    "\n"

    "void main()\n"
    "{\n"
    "  int i = 0;\n" // needed by picking::Impl
    "  //VTK::Picking::Impl\n"

    "  FPROPID = PROPID[0];\n"
    "  framecolorsFS = framecolorsGS[0];\n"
    "  int frameWidth = FrameWidths[FPROPID];\n"
    "  int descender = Descenders[FPROPID];\n" // See HACK comment below

    // glyph info
    "  ivec4 glyphExt = glyphExtentsGS[0];\n"
    "  ivec2 glyphGeom = ivec2(glyphExt[1] - glyphExt[0] + 1 -2*" SPADSZ ",\n"
    "                          glyphExt[3] - glyphExt[2] + 1 -2*" SPADSZ ");\n"

    // tcoords
    "  vec2 tcMin = vec2((glyphExt[0] +" SPADSZ ") / float(atlasDims.x),\n"
    "                    (glyphExt[2] +" SPADSZ ") / float(atlasDims.y));\n"
    "  vec2 tcMax = vec2((glyphExt[1] + 1 -" SPADSZ ") / float(atlasDims.x),\n"
    "                    (glyphExt[3] + 1 -" SPADSZ ") / float(atlasDims.y));\n"

    // Compute the anchor point's screen coordinates
    "  vec4 anchor = ClipCoordToDisplayCoord(gl_in[0].gl_Position);\n"
    // Adjust for the character offset:
    "  anchor.x += round(COFF[0]);\n"

    // Adjust for the frame and bg padding, so that the anchor is on the frame edge.
    "  int acenterX = 0;\n"
    "  int acenterY = 0;\n"
    "  if (anchorCenter[0]<0)\n"
    "    {acenterX = frameWidth + 1 + descender; }\n"
    "  if (anchorCenter[0]==0)\n"
    "    {acenterX = 0; }\n"
    "  if (anchorCenter[0]>0)\n"
    "    {acenterX = -(frameWidth + 1 + descender); }\n"
    "  if (anchorCenter[1]<0)\n"
    "    {acenterY = frameWidth; }\n"
    "  if (anchorCenter[1]==0)\n"
    "    {acenterY = -(descender+glyphGeom.y)/2; }\n"
    "  if (anchorCenter[1]>0)\n"
    "    {acenterY = -(frameWidth+descender+glyphGeom.y); }\n"
    "  anchor.xy += vec2(acenterX, acenterY);\n"

    // Anchor to an exact pixel:
    "  anchor.xy = floor(anchor.xy);\n"

    "  vec4 blDisp = anchor;\n"
    "  vec4 trDisp = vec4(anchor.xy + glyphGeom.xy, anchor.zw);\n"
    "  vec4 brDisp = vec4(trDisp.x, anchor.yzw);\n"
    "  vec4 tlDisp = vec4(anchor.x, trDisp.y, anchor.zw);\n"

    "  vec4 bl = DisplayCoordToClipCoord(blDisp);\n"
    "  vec4 tr = DisplayCoordToClipCoord(trDisp);\n"
    "  vec4 br = DisplayCoordToClipCoord(brDisp);\n"
    "  vec4 tl = DisplayCoordToClipCoord(tlDisp);\n"

    // first the glyph
    "  layer = 0;\n"

    "  //first triangle\n"
    "  UV2 = tcMin;\n"
    "  gl_Position = bl;\n"
    "  EmitVertex();\n"
    "  UV2 = vec2(tcMax.x, tcMin.y);\n"
    "  gl_Position = br;\n"
    "  EmitVertex();\n"
    "  UV2 = tcMax;\n"
    "  gl_Position = tr;\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"

    "  //second triangle\n"
    "  UV2 = tcMin;\n"
    "  gl_Position = bl;\n"
    "  EmitVertex();\n"
    "  UV2 = tcMax;\n"
    "  gl_Position = tr;\n"
    "  EmitVertex();\n"
    "  UV2 = vec2(tcMin.x, tcMax.y);\n"
    "  gl_Position = tl;\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"

    // Draw the background:

    "  layer = 1;\n"

    // Overdraw background to cover any gaps between glyphs. Use the maximum height
    // of all glyphs for the current tprop to keep the upper border consistent.
    "  int bgHeight = MaxGlyphHeights[FPROPID];\n"
    "  blDisp.x -= 1;\n"
    //"  brDisp.x += 1;\n"
    "  tlDisp.x -= 1;\n"
    "  tlDisp.y = blDisp.y + bgHeight;\n"
    //"  trDisp.x += 1;\n"
    "  trDisp.y = brDisp.y + bgHeight;\n"

    // HACK:
    // To make strings without descenders more aesthetically pleasing, pad the
    // top and sides with the descender. This will take up a lot of extra screen
    // real estate and make strings with descenders look weird, but for now, this is
    // what the project calls for. This should be replaced with a proper
    // implementation of tightly-bound glyphs at some point so that all rendered
    // strings, with or without descenders, will have balanced margins.
    "  blDisp.x -= descender;\n"
    "  brDisp.x += descender;\n"
    "  tlDisp.x -= descender;\n"
    "  tlDisp.y += descender;\n"
    "  trDisp.xy += ivec2(descender);\n"

    "  bl = DisplayCoordToClipCoord(blDisp);\n"
    "  br = DisplayCoordToClipCoord(brDisp);\n"
    "  tl = DisplayCoordToClipCoord(tlDisp);\n"
    "  tr = DisplayCoordToClipCoord(trDisp);\n"

    "  gl_Position = tl;\n"
    "  EmitVertex();\n"
    "  gl_Position = tr;\n"
    "  EmitVertex();\n"
    "  gl_Position = bl;\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"

    "  gl_Position = tr;\n"
    "  EmitVertex();\n"
    "  gl_Position = bl;\n"
    "  EmitVertex();\n"
    "  gl_Position = br;\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"

    // Draw the frame (if needed):

    "  if (frameWidth > 0)\n"
    "  {\n"
    "    layer = 2;\n"
    "    blDisp.xy -= vec2(frameWidth);\n"
    "    trDisp.xy += vec2(frameWidth);\n"
    "    brDisp.x = trDisp.x;\n"
    "    brDisp.y = blDisp.y;\n"
    "    tlDisp.x = blDisp.x;\n"
    "    tlDisp.y = trDisp.y;\n"

    "    bl = DisplayCoordToClipCoord(blDisp);\n"
    "    tr = DisplayCoordToClipCoord(trDisp);\n"
    "    br = DisplayCoordToClipCoord(brDisp);\n"
    "    tl = DisplayCoordToClipCoord(tlDisp);\n"

    "    gl_Position = tl;\n"
    "    EmitVertex();\n"
    "    gl_Position = tr;\n"
    "    EmitVertex();\n"
    "    gl_Position = bl;\n"
    "    EmitVertex();\n"
    "    EndPrimitive();\n"

    "    gl_Position = tr;\n"
    "    EmitVertex();\n"
    "    gl_Position = bl;\n"
    "    EmitVertex();\n"
    "    gl_Position = br;\n"
    "    EmitVertex();\n"
    "    EndPrimitive();\n"
    "  }\n"
    "}\n");
  this->SetGeometryShaderCode(geomp_str.c_str());

  this->AddShaderReplacement(vtkShader::Fragment, "//VTK::TCoord::Dec", true,
    "in vec2 UV2;\n"
    "flat in int FPROPID;\n"
    "flat in int layer;\n",
    false);
  this->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::Color::Impl", true, "//NO COLOR IMPL", false);
  this->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::Normal::Impl", true, "//NO NORMAL IMPL", false);

  this->AddShaderReplacement(vtkShader::Fragment, "//VTK::Coincident::Dec", true,
    "float cscale = length(vec2(dFdx(gl_FragCoord.z),dFdy(gl_FragCoord.z)));\n", false);

  this->AddShaderReplacement(vtkShader::Fragment, "//VTK::Depth::Impl", true,
    "if (layer == 0) {\n"
    "  gl_FragDepth = gl_FragCoord.z;\n"
    "}\n"
    "if (layer == 1) {\n"
    "  gl_FragDepth = gl_FragCoord.z + 2*cscale + 0.000016*2.0;\n"
    "}\n"
    "if (layer == 2) {\n"
    "  gl_FragDepth = gl_FragCoord.z + 2*cscale + 0.000016*4.0;\n"
    "}\n",
    false);

  this->AddShaderReplacement(vtkShader::Fragment, "//VTK::Light::Dec", true,
    "uniform vec4 BackgroundColors[" SMAXPROPS "];\n"
    "uniform sampler2D atlasTex;\n"
    "flat in vec3 framecolorsFS;\n"
    "//VTK::Light::Dec",
    false);
  this->AddShaderReplacement(vtkShader::Fragment, "//VTK::Light::Impl", true,
    "if (layer == 0) {\n"
    " gl_FragData[0] = texture(atlasTex, UV2);\n"
    "}\n"
    "if (layer == 1) {\n"
    " gl_FragData[0] = BackgroundColors[FPROPID];\n"
    "}\n"
    "if (layer == 2) {\n"
    " gl_FragData[0] = vec4(framecolorsFS,1);\n"
    "}\n",
    false);
  this->AddShaderReplacement(
    vtkShader::Fragment, "//VTK::TCoord::Impl", true, "//NO TCOORD IMPL", false);
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::AllocateLabels(int numLabels)
{
  if (numLabels > this->NumberOfLabelsAllocated)
  {
    this->NumberOfLabelsAllocated = numLabels;
  }
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::MakeShaderArrays(int numCurLabels,
  const std::vector<std::string>& stringlist, vtkIntArray* typeArr, vtkFloatArray* fcolArr)
{
  // make up arrays from to help place each character in each word
  int pntcnt = 0;
  for (int i = 0; i < numCurLabels; i++) // each word
  {
    auto wordString = stringlist[i];
    int wordsPropId = 0;
    if (typeArr)
    {
      // todo assumes types are 0,1,2,... this isn't necessarily true
      wordsPropId = typeArr->GetValue(i);
    }
    double fcolors[3] = { 0.0, 0.0, 0.0 };
    if (fcolArr)
    {
      // frame colors from point aligned array
      fcolArr->GetTuple(i, fcolors);
    }
    else
    {
      // frame colors from TextProperty
      this->Implementation->TextProperties[wordsPropId]->GetFrameColor(fcolors);
    }
    vtkTextProperty* prop = this->GetLabelTextProperty(wordsPropId);
    if (!prop)
    {
      vtkErrorMacro("No text property available for type array entry '" << wordsPropId << "'.");
      continue;
    }

    double coffset = 0.0;
    vtkIdType startpt = pntcnt;
    for (unsigned int cidx = 0; cidx < wordString.size(); cidx++) // each char
    {
      std::string c = wordString.substr(cidx, 1);
      WordRecord wrec =
        this->Implementation->AllStrings.find(std::make_pair(c, wordsPropId))->second;
      vtkImageData* wr = wrec.Texture;
      int wordsextents[6];
      wr->GetExtent(wordsextents);
      vtkIdType ptlist = pntcnt;
      pntcnt++;
      this->Implementation->InputPlusArrays->InsertNextCell(VTK_VERTEX, 1, &ptlist);
      this->Implementation->pid->InsertNextValue(i);
      this->Implementation->glyphExtents->InsertNextTypedTuple(wordsextents);
      this->Implementation->coff->InsertNextValue(coffset);
      float width = wordsextents[1] - wordsextents[0] + 1 - (2 * PADSZ);
      coffset += width;
      this->Implementation->propid->InsertNextValue(wrec.propid);
      this->Implementation->framecolors->InsertNextTuple3(fcolors[0], fcolors[1], fcolors[2]);
    }
    // align glyphs
    if (this->TextAnchor == LowerLeft || this->TextAnchor == UpperLeft ||
      this->TextAnchor == LeftEdge)
    {
      coffset = 0.0;
    }
    else if (this->TextAnchor == LowerEdge || this->TextAnchor == UpperEdge ||
      this->TextAnchor == Center)
    {
      coffset = coffset / 2;
    }
    /*
    else if (this->TextAnchor == LowerRight ||
        this->TextAnchor == UpperRight ||
        this->TextAnchor == RightEdge)
    {
      coffset = coffset;
    }
    */
    for (unsigned int cidx = 0; cidx < wordString.size(); cidx++) // each char
    {
      double pos = this->Implementation->coff->GetValue(startpt + cidx);
      pos = pos - coffset;
      this->Implementation->coff->SetValue(startpt + cidx, pos);
    }
  }
  this->Implementation->InputPlusArrays->GetPointData()->AddArray(
    this->Implementation->glyphExtents);
  this->Implementation->InputPlusArrays->GetPointData()->AddArray(this->Implementation->coff);
  this->Implementation->InputPlusArrays->GetPointData()->AddArray(this->Implementation->pid);
  this->Implementation->InputPlusArrays->GetPointData()->AddArray(this->Implementation->propid);
  this->Implementation->InputPlusArrays->GetPointData()->AddArray(
    this->Implementation->framecolors);
  this->Implementation->glyphExtents->Modified();
  this->Implementation->coff->Modified();
  this->Implementation->pid->Modified();
  this->Implementation->propid->Modified();
  this->Implementation->framecolors->Modified();
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::BuildLabelsInternal(vtkDataSet* input)
{
  int i, j, numComp = 0, pointIdLabels = 0, activeComp = 0;
  vtkAbstractArray* abstractData = nullptr;
  vtkDataArray* numericData = nullptr;
  vtkStringArray* stringData = nullptr;
  vtkUnicodeStringArray* uStringData = nullptr;
  if (input->GetNumberOfPoints() == 0)
  {
    return;
  }

  vtkNew<vtkTimerLog> ttotal;
  ttotal->StartTimer();
  vtkNew<vtkTimerLog> tfreetype;
  double tfreetypet = 0.0;
  vtkNew<vtkTimerLog> tindex;
  vtkNew<vtkTimerLog> tappend;
  double tappendt = 0.0;

  vtkPointData* pd = input->GetPointData();
  // figure out what to label, and if we can label it
  pointIdLabels = 0;
  switch (this->LabelMode)
  {
    case VTK_LABEL_IDS:
    {
      pointIdLabels = 1;
    };
    break;
    case VTK_LABEL_SCALARS:
      if (pd->GetScalars())
      {
        numericData = pd->GetScalars();
      }
      break;
    case VTK_LABEL_VECTORS:
      if (pd->GetVectors())
      {
        numericData = pd->GetVectors();
      }
      break;
    case VTK_LABEL_NORMALS:
      if (pd->GetNormals())
      {
        numericData = pd->GetNormals();
      }
      break;
    case VTK_LABEL_TCOORDS:
      if (pd->GetTCoords())
      {
        numericData = pd->GetTCoords();
      }
      break;
    case VTK_LABEL_TENSORS:
      if (pd->GetTensors())
      {
        numericData = pd->GetTensors();
      }
      break;
    case VTK_LABEL_FIELD_DATA:
    {
      int arrayNum;
      if (this->FieldDataName != nullptr)
      {
        vtkDebugMacro(<< "Labeling field data array " << this->FieldDataName);
        abstractData = pd->GetAbstractArray(this->FieldDataName, arrayNum);
      }
      else
      {
        arrayNum = (this->FieldDataArray < pd->GetNumberOfArrays() ? this->FieldDataArray
                                                                   : pd->GetNumberOfArrays() - 1);
        abstractData = pd->GetAbstractArray(arrayNum);
      }
      numericData = vtkArrayDownCast<vtkDataArray>(abstractData);
      stringData = vtkArrayDownCast<vtkStringArray>(abstractData);
      uStringData = vtkArrayDownCast<vtkUnicodeStringArray>(abstractData);
    };
    break;
  }

  // determine number of components and check input
  if (pointIdLabels)
  {
    numComp = 1;
  }
  else if (numericData)
  {
    numComp = numericData->GetNumberOfComponents();
    activeComp = 0;
    if (this->LabeledComponent >= 0)
    {
      activeComp = (this->LabeledComponent < numComp ? this->LabeledComponent : numComp - 1);
      numComp = 1;
    }
  }
  else
  {
    if (stringData)
    {
      numComp = stringData->GetNumberOfComponents();
    }
    else if (uStringData)
    {
      numComp = uStringData->GetNumberOfComponents();
    }
    else
    {
      if (this->FieldDataName)
      {
        vtkWarningMacro(<< "Could not find label array (" << this->FieldDataName << ") "
                        << "in input.");
      }
      else
      {
        vtkWarningMacro(<< "Could not find label array ("
                        << "index " << this->FieldDataArray << ") "
                        << "in input.");
      }

      return;
    }
  }

  vtkStdString FormatString;
  if (this->LabelFormat)
  {
    // The user has specified a format string.
    vtkDebugMacro(<< "Using user-specified format string " << this->LabelFormat);
    FormatString = this->LabelFormat;
  }
  else
  {
    // Try to come up with some sane default.
    if (pointIdLabels)
    {
      FormatString = "%d";
    }
    else if (numericData)
    {
      switch (numericData->GetDataType())
      {
        case VTK_VOID:
          FormatString = "0x%x";
          break;

          // don't use vtkTypeTraits::ParseFormat for character types as parse formats
          // aren't the same as print formats for these types.
        case VTK_BIT:
        case VTK_SIGNED_CHAR:
        case VTK_UNSIGNED_CHAR:
        case VTK_SHORT:
        case VTK_UNSIGNED_SHORT:
        case VTK_INT:
        case VTK_UNSIGNED_INT:
          FormatString = "%d";
          break;

        case VTK_CHAR:
          FormatString = "%c";
          break;

        case VTK_LONG:
          FormatString = vtkTypeTraits<long>::ParseFormat();
          break;
        case VTK_UNSIGNED_LONG:
          FormatString = vtkTypeTraits<unsigned long>::ParseFormat();
          break;

        case VTK_ID_TYPE:
          FormatString = vtkTypeTraits<vtkIdType>::ParseFormat();
          break;

        case VTK_LONG_LONG:
          FormatString = vtkTypeTraits<long long>::ParseFormat();
          break;
        case VTK_UNSIGNED_LONG_LONG:
          FormatString = vtkTypeTraits<unsigned long long>::ParseFormat();
          break;

        case VTK_FLOAT:
          FormatString = vtkTypeTraits<float>::ParseFormat();
          break;

        case VTK_DOUBLE:
          FormatString = vtkTypeTraits<double>::ParseFormat();
          break;

        default:
          FormatString = "BUG - UNKNOWN DATA FORMAT";
          break;
      }
    }
    else if (stringData)
    {
      FormatString = "";
    }
    else if (uStringData)
    {
      vtkWarningMacro(
        "Unicode string arrays are not adequately supported by the vtkFastLabeledDataMapper.  "
        "Unicode strings will be converted to vtkStdStrings for rendering.");
      FormatString = "unicode";
    }
    else
    {
      FormatString = "BUG - COULDN'T DETECT DATA TYPE";
    }

    vtkDebugMacro(<< "Using default format string " << FormatString.c_str());
  }

  int numCurLabels = input->GetNumberOfPoints();
  int numCurChars = 0;
  // We are assured that
  // this->NumberOfLabelsAllocated >= (this->NumberOfLabels + numCurLabels)
  if (this->NumberOfLabelsAllocated < (this->NumberOfLabels + numCurLabels))
  {
    vtkErrorMacro("Number of labels must be allocated before this method is called.");
    return;
  }

  vtkIntArray* typeArr =
    vtkArrayDownCast<vtkIntArray>(this->GetInputAbstractArrayToProcess(0, input));
  vtkFloatArray* fcolArr = nullptr;
  if (this->FrameColorsName)
  {
    fcolArr =
      vtkArrayDownCast<vtkFloatArray>(input->GetPointData()->GetArray(this->FrameColorsName));
  }

  // ----------------------------------------
  // Now we actually construct the label strings
  //
  vtkPolyData* asPD = vtkPolyData::SafeDownCast(input);
  const char* LiveFormatString = FormatString.c_str();
  char TempString[1024];

  int rebuildcnt = 0;
  std::vector<std::string> stringlist;
  // stringlist.reserve(numCurLabels*2);
  for (i = 0; i < numCurLabels; i++)
  {
    vtkStdString ResultString;
    if (pointIdLabels)
    {
      snprintf(TempString, sizeof(TempString), LiveFormatString, i);
      ResultString = TempString;
    }
    else
    {
      if (numericData)
      {
        void* rawData = numericData->GetVoidPointer(i * numComp);

        if (numComp == 1)
        {
          switch (numericData->GetDataType())
          {
            vtkTemplateMacro(vtkFastLabeledDataMapper_PrintComponent(TempString, sizeof(TempString),
              LiveFormatString, activeComp, static_cast<VTK_TT*>(rawData)));
          }
          ResultString = TempString;
        }
        else // numComp != 1
        {
          ResultString = "(";

          // Print each component in turn and add it to the string.
          for (j = 0; j < numComp; ++j)
          {
            switch (numericData->GetDataType())
            {
              vtkTemplateMacro(vtkFastLabeledDataMapper_PrintComponent(TempString,
                sizeof(TempString), LiveFormatString, j, static_cast<VTK_TT*>(rawData)));
            }
            ResultString += TempString;

            if (j < (numComp - 1))
            {
              ResultString += this->GetComponentSeparator();
            }
            else
            {
              ResultString += ')';
            }
          }
        }
      }
      else // rendering string data
      {
        // If the user hasn't given us a custom format string then
        // we'll sidestep a lot of snprintf nonsense.
        if (this->LabelFormat == nullptr)
        {
          if (uStringData)
          {
            ResultString = uStringData->GetValue(i).utf8_str();
          }
          else
          {
            ResultString = stringData->GetValue(i);
          }
        }
        else // the user specified a label format
        {
          snprintf(TempString, 1023, LiveFormatString, stringData->GetValue(i).c_str());
          ResultString = TempString;
        } // done printing strings with label format
      }   // done printing strings
    }     // done creating string

    // cerr << "MAKING TEXTURES FOR " << ResultString << endl;
    for (unsigned int cidx = 0; cidx < ResultString.size(); cidx++)
    {
      numCurChars++;
      std::string c = ResultString.substr(cidx, 1);
      auto hasTexture = this->Implementation->AllStrings.find(std::make_pair(c, 0));
      if (hasTexture == this->Implementation->AllStrings.end())
      {
        rebuildcnt++;
        for (int tid = 0; tid < MAXPROPS; ++tid)
        {
          vtkTextProperty* prop = this->Implementation->TextProperties[tid];
          if (prop)
          {
            // cerr << "make a texture for " << c << " " << tid << endl;
            tfreetype->StartTimer();
            WordRecord wr = this->Implementation->MakeWordTexture(c, prop, tid);
            this->Implementation->AllStrings[std::make_pair(c, tid)] = wr;
            tfreetype->StopTimer();
            tfreetypet += tfreetype->GetElapsedTime();
          }
        }
      }
      else
      {
        // cerr << "reuse " << ResultString << endl;
      }
    }
    stringlist.push_back(ResultString);
  }

  if ((this->Implementation->InputPlusArrays->GetNumberOfCells() != numCurChars) && !rebuildcnt)
  {
    // the data has changed, but we don't need any new characters
    // rebuild the structure to get the data right
    // cerr << "RESIZE "  << numCurLabels << " " << numCurChars << endl;

    this->Implementation->FreshIPA();
    this->MakeShaderArrays(numCurLabels, stringlist, typeArr, fcolArr);
  }
  if (rebuildcnt)
  {
    // we need at least one new characters
    // rebuild the texture and then
    // rebuild the structure to get the data right
    // cerr << "REBUILD "  << numCurLabels << " " << numCurChars << endl;
    std::fill(
      this->Implementation->MaxGlyphHeights, this->Implementation->MaxGlyphHeights + MAXPROPS, 0);
    this->Implementation->ipos = 0;
    this->Implementation->jpos = 0;
    int sx = 0;
    int sy = 0;
    int ex = 0;
    int ey = 0;
    this->Implementation->FreshIPA();
    // add all of the characters to the grouped texture
    for (auto writ = this->Implementation->AllStrings.begin();
         writ != this->Implementation->AllStrings.end(); writ++)
    {
      int propIdx = writ->first.second;
      WordRecord wr = writ->second;
      this->Implementation->AppendToWordTexture(wr.Texture, propIdx, sx, sy, ex, ey);
    }
    this->MakeShaderArrays(numCurLabels, stringlist, typeArr, fcolArr);
    tappend->StartTimer();
    this->Implementation->MakeItSo();
    tappend->StopTimer();
    tappendt += tappend->GetElapsedTime();
#if 0
    auto dwriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    dwriter->SetFileName("/home/demarle/Desktop/foo.vtp");
    dwriter->SetInputData(this->Implementation->InputPlusArrays);
    dwriter->SetDataModeToAscii();
    dwriter->Write();

    auto writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName("/home/demarle/Desktop/foo.png");
    writer->SetInputData(this->Implementation->WordsTexture);
    writer->Write();
#endif
  }

  const vtkMTimeType textureMTime = this->Implementation->glyphsTO->GetMTime();
  const vtkMTimeType imageMTime = this->Implementation->WordsTexture->GetMTime();

  if (rebuildcnt || this->Implementation->glyphsTO->GetHandle() == 0 || textureMTime < imageMTime)
  {
    this->Implementation->UploadTexture();
  }

  if (asPD->GetMTime() > this->BuildTime || rebuildcnt)
  {
    auto pts = vtkSmartPointer<vtkPoints>::New();
    this->Implementation->InputPlusArrays->SetPoints(pts);
    for (i = 0; i < numCurLabels; i++)
    {
      auto wordString = stringlist[i];
      for (unsigned int cidx = 0; cidx < wordString.size(); cidx++)
      {
        pts->InsertNextPoint(asPD->GetPoints()->GetPoint(i));
      }
    }
  }

  this->NumberOfLabels += numCurLabels;

  ttotal->StopTimer();

  // cerr << numCurLabels << " " << ttotal->GetElapsedTime() << " " << tfreetypet << " " << tappendt
  // << endl;
}

//----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::BuildLabels()
{
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(inputDO);
  if (ds)
  {
    this->AllocateLabels(ds->GetNumberOfPoints());
    this->NumberOfLabels = 0;
    this->BuildLabelsInternal(ds);
    this->Implementation->UpdateTextPropertyAttributeArrays();
  }
  else
  {
    vtkErrorMacro("Unsupported data type: " << inputDO->GetClassName());
  }
  // this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::RenderPiece(vtkRenderer* ren, vtkActor* actor)
{
  // Updates the input pipeline if needed.
  this->Update();

  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  if (!inputDO)
  {
    this->NumberOfLabels = 0;
    vtkErrorMacro(<< "Need input data to render labels (2)");
    return;
  }
  vtkAlgorithmOutput* savedreference = this->GetInputConnection(0, 0);

  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || inputDO->GetMTime() > this->BuildTime ||
    this->Implementation->DPI != ren->GetRenderWindow()->GetDPI())
  {
    this->Implementation->glyphsTO->SetContext(
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow()));
    this->Implementation->DPI = ren->GetRenderWindow()->GetDPI();
    this->BuildLabels();
  }
  this->SetInputData(this->Implementation->InputPlusArrays);
  this->Superclass::RenderPiece(ren, actor);
  // weird that data input path doesn't work
  if (true) // savedreference && savedreference->GetProducer())
  {
    this->SetInputConnection(savedreference);
  }
  else
  {
    this->SetInputData(vtkPolyData::SafeDownCast(inputDO));
  }
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------
vtkMTimeType vtkFastLabeledDataMapper::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();

  for (vtkTextProperty* tprop : this->Implementation->TextProperties)
  {
    if (tprop)
    {
      mtime = std::max(mtime, tprop->GetMTime());
    }
  }

  return mtime;
}

//-----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::RenderPieceStart(vtkRenderer* ren, vtkActor* actor)
{
  this->Implementation->glyphsTO->Activate();
  this->Superclass::RenderPieceStart(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::RenderPieceFinish(vtkRenderer* ren, vtkActor* actor)
{
  this->Implementation->glyphsTO->Deactivate();
  this->Superclass::RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkFastLabeledDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Implementation->glyphsTO->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}
