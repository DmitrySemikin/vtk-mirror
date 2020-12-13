## Fix SIGSEGV with vtkFieldData::DeepCopy ( #17980)

- vtkFieldData::DeepCopy now initialize fields before starting actual copy.  This fixes SIGSEGV.  Flags are also copied now.
- vtkFieldData::CopyFlags now copies flag status as well as AllOn/Off status.  This affects ShallowCopy.
