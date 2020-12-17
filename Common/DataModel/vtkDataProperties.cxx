#include "vtkDataProperties.h"

#include <limits>
#include <string>

namespace
{
const char* vtkDataPropertiesStrings[] = { "Visibility", "Pickability", "Color", "Opacity",
  "Material" };
}

namespace vtkDataProperties
{

const char* GetStringFromDataProperties(int prop)
{
  const auto sizeOfPropStr = sizeof(vtkDataPropertiesStrings) / sizeof(const char*);

  if (prop >= sizeOfPropStr)
  {
    return {};
  }

  return vtkDataPropertiesStrings[prop];
}

int GetPropertiesFromString(const char* prop)
{
  const auto sizeOfPropStr = sizeof(vtkDataPropertiesStrings) / sizeof(const char*);
  for (int i = 0; i < sizeOfPropStr; i++)
  {
    if (std::string(prop) == std::string(vtkDataPropertiesStrings[i]))
    {
      return i;
    }
  }

  return std::numeric_limits<int>::max();
}

}
