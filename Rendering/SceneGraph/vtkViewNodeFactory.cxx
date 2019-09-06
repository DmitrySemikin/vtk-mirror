/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkViewNodeFactory.h"
#include "vtkObjectFactory.h"
#include "vtkViewNode.h"

#include <map>
#include <string>

//============================================================================
class vtkViewNodeFactory::vtkInternals
{
public:
  std::map<std::string, vtkViewNode *(*)()> Overrides;

  vtkInternals()
  {
  }

  ~vtkInternals()
  {
      this->Overrides.clear();
  }
};

//============================================================================
vtkStandardNewMacro(vtkViewNodeFactory);

//----------------------------------------------------------------------------
vtkViewNodeFactory::vtkViewNodeFactory()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkViewNodeFactory::~vtkViewNodeFactory()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkViewNodeFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkViewNode *vtkViewNodeFactory::CreateNode(vtkObject *who)
{
  if (!who)
  {
    return nullptr;
  }
  const char *forwhom = who->GetClassName();
  vtkViewNode *vn = this->CreateNode(forwhom);
  if (vn)
  {
    vn->SetRenderable(who);
  }
  return vn;
}

//----------------------------------------------------------------------------
vtkViewNode *vtkViewNodeFactory::CreateNode(const char *forwhom)
{
  if (this->Internals->Overrides.find(forwhom) ==
      this->Internals->Overrides.end())
  {
    return nullptr;
  }
  vtkViewNode *(*func)() = this->Internals->Overrides.find(forwhom)->second;
  vtkViewNode *vn = func();
  vn->SetMyFactory(this);
  return vn;
}

//----------------------------------------------------------------------------
void vtkViewNodeFactory::RegisterOverride
  (const char *name, vtkViewNode *(*func)())
{
  this->Internals->Overrides[name] = func;
}
