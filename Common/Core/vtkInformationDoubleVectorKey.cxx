/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkInformationDoubleVectorKey.h"

#include "vtkInformation.h" // For vtkErrorWithObjectMacro

#include <vector>


//----------------------------------------------------------------------------
vtkInformationDoubleVectorKey
::vtkInformationDoubleVectorKey(const char* name, const char* location,
                                 int length):
  vtkInformationKey(name, location), RequiredLength(length)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationDoubleVectorKey::~vtkInformationDoubleVectorKey() = default;

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationDoubleVectorValue: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationDoubleVectorValue, vtkObjectBase);
  std::vector<double> Value;
};

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Append(vtkInformation* info, double value)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  if(v)
  {
    v->Value.push_back(value);
  }
  else
  {
    this->Set(info, &value, 1);
  }
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Set(vtkInformation* info,
                                        const double* value,
                                        int length)
{
  if(value)
  {
    if(this->RequiredLength >= 0 && length != this->RequiredLength)
    {
      vtkErrorWithObjectMacro(
        info,
        "Cannot store double vector of length " << length
        << " with key " << this->Location << "::" << this->Name
        << " which requires a vector of length "
        << this->RequiredLength << ".  Removing the key instead.");
      this->SetAsObjectBase(info, nullptr);
      return;
    }
    vtkInformationDoubleVectorValue* v =
      new vtkInformationDoubleVectorValue;
    v->InitializeObjectBase();
    v->Value.insert(v->Value.begin(), value, value+length);
    this->SetAsObjectBase(info, v);
    v->Delete();
  }
  else
  {
    this->SetAsObjectBase(info, nullptr);
  }
}

//----------------------------------------------------------------------------
double* vtkInformationDoubleVectorKey::Get(vtkInformation* info)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  return (v && !v->Value.empty())?(&v->Value[0]):nullptr;
}

//----------------------------------------------------------------------------
double vtkInformationDoubleVectorKey::Get(vtkInformation* info, int idx)
{
  if (idx >= this->Length(info))
  {
    vtkErrorWithObjectMacro(info,
                            "Information does not contain " << idx
                            << " elements. Cannot return information value.");
    return 0;
  }
  double* values = this->Get(info);
  return values[idx];
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Get(vtkInformation* info,
                                     double* value)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  if(v && value)
  {
    for(std::vector<double>::size_type i = 0;
        i < v->Value.size(); ++i)
    {
      value[i] = v->Value[i];
    }
  }
}

//----------------------------------------------------------------------------
int vtkInformationDoubleVectorKey::Length(vtkInformation* info)
{
  vtkInformationDoubleVectorValue* v =
    static_cast<vtkInformationDoubleVectorValue *>(
      this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Value.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::ShallowCopy(vtkInformation* from,
                                          vtkInformation* to)
{
  this->Set(to, this->Get(from), this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationDoubleVectorKey::Print(ostream& os, vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    double* value = this->Get(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
    {
      os << sep << value[i];
      sep = " ";
    }
  }
}
