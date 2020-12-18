/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointsPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test verifies that error bar functionality in vtkPlotPoints and
// vtkPlotLine works.

#include "vtkRenderWindow.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkPlotLine.h"
#include "vtkPlotPoints.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkAxis.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestPlotErrorBars(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(600, 450);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> xArray;
  xArray->SetName("x");
  table->AddColumn(xArray);
  vtkNew<vtkFloatArray> sinArray;
  sinArray->SetName("sin");
  table->AddColumn(sinArray);
  vtkNew<vtkFloatArray> cosArray;
  cosArray->SetName("cos");
  table->AddColumn(cosArray);
  vtkNew<vtkFloatArray> sumArray;
  sumArray->SetName("sum");
  table->AddColumn(sumArray);
  vtkNew<vtkFloatArray> subArray;
  subArray->SetName("subtract");
  table->AddColumn(subArray);
  vtkNew<vtkFloatArray> multArray;
  multArray->SetName("multiply");
  table->AddColumn(multArray);
  vtkNew<vtkFloatArray> xErrorArray;
  xErrorArray->SetName("x error");
  table->AddColumn(xErrorArray);
  vtkNew<vtkFloatArray> cosErrorArray;
  cosErrorArray->SetName("cos error");
  table->AddColumn(cosErrorArray);
  vtkNew<vtkFloatArray> sumErrorArray;
  sumErrorArray->SetName("sum error");
  table->AddColumn(sumErrorArray);
  vtkNew<vtkFloatArray> subErrorArray;
  subErrorArray->SetName("sub error");
  table->AddColumn(subErrorArray);

  // Add some data points to the chart
  int numPoints = 100;
  float maxX = 15.0f;
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    float num = i * maxX / numPoints;
    table->SetValue(i, 0, num);
    table->SetValue(i, 1, sin(num));
    table->SetValue(i, 2, cos(num) + 1.0);
    table->SetValue(i, 3, sin(num) + cos(num) + 3.0);
    table->SetValue(i, 4, sin(num) - cos(num) + 4.0);
    table->SetValue(i, 5, sin(num) * cos(num) + 5.0);
    table->SetValue(i, 6, abs(num / 10.0));
    table->SetValue(i, 7, abs(cos(num) / 3.0));
    table->SetValue(i, 8, abs((sin(num) + cos(num)) / 3.0));
    table->SetValue(i, 9, abs((sin(num) - cos(num)) / 3.0));
  }

  // Add a line plot without errors (sin)
  auto sinPlot = vtkSmartPointer<vtkPlotLine>::New();
  chart->AddPlot(sinPlot);
  sinPlot->SetInputData(table, 0, 1);
  sinPlot->SetColor(0, 0, 0, 255);
  sinPlot->SetWidth(1.0);

   // Add a points plot with errors (cos)
   auto cosPlot = vtkSmartPointer<vtkPlotPoints>::New();
   chart->AddPlot(cosPlot);
   cosPlot->SetInputData(table, 0, 2, 6, 7);
   cosPlot->SetPlotErrorBars(1);
   cosPlot->SetColor(0, 255, 0, 255);
   cosPlot->SetWidth(2.0);

   // Add a points plot with errors, but don't plot the errors (sum)
   auto sumPlot = vtkSmartPointer<vtkPlotPoints>::New();
   chart->AddPlot(sumPlot);
   sumPlot->SetInputData(table, 0, 3, 6, 8);
   sumPlot->SetPlotErrorBars(0);
   sumPlot->SetColor(0, 0, 255, 255);
   sumPlot->SetWidth(2.0);

   // Add a line plot with errors (subtract)
   auto subPlot = vtkSmartPointer<vtkPlotLine>::New();
   chart->AddPlot(subPlot);
   subPlot->SetInputData(table, 0, 4, 6, 9);
   subPlot->SetPlotErrorBars(1);
   subPlot->SetColor(255, 0, 0, 255);
   subPlot->SetWidth(1.0);

  // Add a points plot without errors, but set PlotErrorBars = true (multiply)
  // (no error bars should be plotted)
   auto multPlot = vtkSmartPointer<vtkPlotPoints>::New();
   chart->AddPlot(multPlot);
   multPlot->SetInputData(table, 0, 5);
   multPlot->SetPlotErrorBars(1);
   multPlot->SetColor(255, 0, 255, 255);
   multPlot->SetWidth(2.0);


  // Tell the axes to use the new tick label placement algorithm.
  chart->GetAxis(vtkAxis::LEFT)
      ->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);
  chart->GetAxis(vtkAxis::BOTTOM)
      ->SetTickLabelAlgorithm(vtkAxis::TICK_WILKINSON_EXTENDED);

  // Finally, render the scene.
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
