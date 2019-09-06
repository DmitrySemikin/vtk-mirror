/*===========================================================================*/
/* Distributed under OSI-approved BSD 3-Clause License.                      */
/* For copyright, see the following accompanying files or https://vtk.org:   */
/* - VTK-Copyright.txt                                                       */
/*===========================================================================*/
#include "vtkStructuredGridLIC2DTestDriver.h"
#include "vtkTestUtilities.h"
#include "vtksys/SystemTools.hxx"

int TestStructuredGridLIC2DXSlice(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/timestep_0_15.vts");

  std::string filename = fname;
  filename = "--data=" + filename;
  delete [] fname;

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/noise.png");
  std::string noise = fname;
  noise = "--noise=" + noise;
  delete [] fname;

  char** new_argv = new char*[argc+12];
  for (int cc=0; cc < argc; cc++)
  {
    new_argv[cc] = vtksys::SystemTools::DuplicateString(argv[cc]);
  }
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(filename.c_str());
  new_argv[argc++] = vtksys::SystemTools::DuplicateString(noise.c_str());
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--mag=8");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--partitions=1");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--num-steps=100");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--slice-dir=0");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--slice=98");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--zoom-factor=3.0");
  new_argv[argc++] = vtksys::SystemTools::DuplicateString("--test-mode=1");
  int status = vtkStructuredGridLIC2DTestDriver(argc, new_argv);
  for (int kk=0; kk < argc; kk++)
  {
    delete [] new_argv[kk];
  }
  delete [] new_argv;

  return status;
}
