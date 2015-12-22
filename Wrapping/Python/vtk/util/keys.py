"""
Utility module to make it easier to create new keys.
"""
from vtk import vtkInformationDataObjectKey as DataaObjectKey
from vtk import vtkInformationDoubleKey as DoubleKey
from vtk import vtkInformationDoubleVectorKey as DoubleVectorKey
from vtk import vtkInformationIdTypeKey as IdTypeKey
from vtk import vtkInformationInformationKey as InformationKey
from vtk import vtkInformationInformationVectorKey as InformationVectorKey
from vtk import vtkInformationIntegerKey as IntegerKey
from vtk import vtkInformationIntegerVectorKey as IntegerVectorKey
from vtk import vtkInformationKeyVectorKey as KeyVectorKey
from vtk import vtkInformationObjectBaseKey as ObjectBaseKey
from vtk import vtkInformationObjectBaseVectorKey as ObjectBaseVectorKey
from vtk import vtkInformationRequestKey as RequestKey
from vtk import vtkInformationStringKey as StringKey
from vtk import vtkInformationStringVectorKey as StringVectorKey
from vtk import vtkInformationUnsignedLongKey as UnsignedLongKey
from vtk import vtkInformationVariantKey as VariantKey
from vtk import vtkInformationVariantVectorKey as VariantVectorKey
from vtk import vtkInformationDataObjectMetaDataKey as DataObjectMetaDataKey
from vtk import vtkInformationExecutivePortKey as ExecutivePortKey
from vtk import vtkInformationExecutivePortVectorKey as ExecutivePortVectorKey
from vtk import vtkInformationIntegerRequestKey as IntegerRequestKey

def MakeKey(key_type, name, location, *args):
    """Given a key type, make a new key of given name
    and location."""
    return key_type.MakeKey(name, location, *args)
