///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <Reporting\SectionPropertiesChapterBuilder.h>
#include <Reporting\SectionPropertiesReportSpecification.h>
#include <IFace/Bridge.h>
#include <IFace/Project.h>
#include <IFace/PointOfInterest.h>
#include <PsgLib\BridgeDescription2.h>
#include <AgentTools.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFApp.h>
#include <Graphing/GraphXY.h>
#include <atlimage.h>


CSectionPropertiesChapterBuilder::CSectionPropertiesChapterBuilder(bool bSelect,bool simplifiedVersion) :
CPGSuperChapterBuilder(bSelect),
m_SimplifiedVersion(simplifiedVersion)
{
}

LPCTSTR CSectionPropertiesChapterBuilder::GetName() const
{
   return TEXT("Section Properties");
}

rptChapter* CSectionPropertiesChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pSectPropRptSpec = std::dynamic_pointer_cast<const CSectionPropertiesReportSpecification>(pRptSpec);

   std::shared_ptr<WBFL::EAF::Broker> pBroker = pSectPropRptSpec->GetBroker();

   const auto& poi = pSectPropRptSpec->GetPOI();
   const auto& segmentKey = poi.GetSegmentKey();
   IntervalIndexType intervalIdx = pSectPropRptSpec->GetInterval();

   GET_IFACE2(pBroker, IShapes, pShapes);

   CComPtr<IShape> shape;
   pShapes->GetSegmentShape(intervalIdx, poi, false, pgsTypes::scGirder, &shape);

   CComPtr<IPoint2dCollection> primaryShapePoints;
   CComPtr<IPoint2dCollection> secondaryShapePoints;
   CComQIPtr<ICompositeShape> compShape(shape);

   CComPtr<IShape> primaryShape;
   CComPtr<IShape> secondaryShape;

   if (compShape)
   {
       IndexType nShapes;
       compShape->get_Count(&nShapes);

       CComPtr<ICompositeShapeItem> item;
       compShape->get_Item(0, &item);
       item->get_Shape(&primaryShape);

       shape->get_PolyPoints(&primaryShapePoints);

       CComQIPtr<ICompositeShape> voidedShape(primaryShape);

       if (voidedShape)
       {
           IndexType nVoidShapes;
           voidedShape->get_Count(&nVoidShapes);

           for (IndexType i = 0; i < nVoidShapes; i++)
           {
               CComPtr<ICompositeShapeItem> voidItem;
               voidedShape->get_Item(i, &voidItem);

               CComPtr<IShape> s;
               voidItem->get_Shape(&s);

               CComPtr<IPoint2dCollection> points;
               s->get_PolyPoints(&points);

               CComPtr<IEnumPoint2d> enumPoints;
               points->get__Enum(&enumPoints);
               CComPtr<IPoint2d> point;
               while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
               {
                   primaryShapePoints->Add(point);
                   point.Release();
               }

           }
       }

       if (1 < nShapes)
       {
           item.Release();
           compShape->get_Item(1, &item);
           item->get_Shape(&secondaryShape);
           secondaryShape->get_PolyPoints(&secondaryShapePoints);
       }
   }
   else
   {
       shape->get_PolyPoints(&primaryShapePoints);

       CComQIPtr<ICompositeShape> voidedShape(primaryShape);

       if (voidedShape)
       {
           IndexType nVoidShapes;
           voidedShape->get_Count(&nVoidShapes);

           for (IndexType i = 0; i < nVoidShapes; i++)
           {
               CComPtr<ICompositeShapeItem> voidItem;
               voidedShape->get_Item(i, &voidItem);

               CComPtr<IShape> s;
               voidItem->get_Shape(&s);

               CComPtr<IPoint2dCollection> points;
               s->get_PolyPoints(&points);

               CComPtr<IEnumPoint2d> enumPoints;
               points->get__Enum(&enumPoints);
               CComPtr<IPoint2d> point;
               while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
               {
                   primaryShapePoints->Add(point);
                   point.Release();
               }

           }
       }
   }

   CComPtr<IShape> leftJointShape, rightJointShape;
   pShapes->GetJointShapes(intervalIdx, poi, false, pgsTypes::scGirder, &leftJointShape, &rightJointShape);
   if (leftJointShape)
   {

       CComPtr<IPoint2dCollection> points;
       leftJointShape->get_PolyPoints(&points);

       CComPtr<IEnumPoint2d> enumPoints;
       points->get__Enum(&enumPoints);
       CComPtr<IPoint2d> point;
       while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
       {
           secondaryShapePoints->Add(point);
           point.Release();
       }
   }

   if (rightJointShape)
   {
       CComPtr<IPoint2dCollection> points;
       rightJointShape->get_PolyPoints(&points);

       CComPtr<IEnumPoint2d> enumPoints;
       points->get__Enum(&enumPoints);
       CComPtr<IPoint2d> point;
       while (enumPoints->Next(1, &point, nullptr) != S_FALSE)
       {
           secondaryShapePoints->Add(point);
           point.Release();
       }
   }


   GET_IFACE2(pBroker, IMaterials, pMaterials);
   Float64 EcGdr;
   if (poi.HasAttribute(POI_CLOSURE))
   {
       EcGdr = pMaterials->GetClosureJointEc(poi.GetSegmentKey(), intervalIdx);
   }
   else
   {
       EcGdr = pMaterials->GetSegmentEc(poi.GetSegmentKey(), intervalIdx);
   }

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   Float64 EcDeck;
   if (IsNonstructuralDeck(pBridgeDesc->GetDeckDescription()->GetDeckType()))
   {
       if (pBridgeDesc->HasStructuralLongitudinalJoints())
       {
           EcDeck = pMaterials->GetLongitudinalJointEc(intervalIdx);
       }
       else
       {
           EcDeck = 0.0;
       }
   }
   else
   {
       GET_IFACE2(pBroker, IPointOfInterest, pPoi);
       IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
       ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
       EcDeck = pMaterials->GetDeckEc(deckCastingRegionIdx, intervalIdx);
   }

   std::vector<std::pair<Float64, Float64>> primaryPoints;
   std::vector<std::pair<Float64, Float64>> secondaryPoints;


   IndexType nPoints;
   if (primaryShapePoints)
   {

       primaryShapePoints->get_Count(&nPoints);
       for (IndexType i = 0; i < nPoints; i++)
       {
           CComPtr<IPoint2d> pnt;
           primaryShapePoints->get_Item(i, &pnt);
           Float64 x, y;
           pnt->Location(&x, &y);

           x = IsZero(x) ? 0 : x;
           y = IsZero(y) ? 0 : y;

           primaryPoints.emplace_back(x, y);

       }
   }

   if (secondaryShapePoints)
   {
       secondaryShapePoints->get_Count(&nPoints);
       for (IndexType i = 0; i < nPoints; i++)
       {
           CComPtr<IPoint2d> pnt;
           secondaryShapePoints->get_Item(i, &pnt);
           Float64 x, y;
           pnt->Location(&x, &y);

           x = IsZero(x) ? 0 : x;
           y = IsZero(y) ? 0 : y;

           secondaryPoints.emplace_back(x, y);

       }
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec, level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   rptRcTable* pLayoutTable = rptStyleManager::CreateLayoutTable(4);

   //pLayoutTable->SetColumnStyle(2, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));
   //pLayoutTable->SetColumnStyle(3, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_CENTER));

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDispUnits->ComponentDim, false);

   rptRcTable* pPrimaryPointsTable = rptStyleManager::CreateDefaultTable(2, _T("Basic Section"));

   (*pLayoutTable)(0, 0) << pPrimaryPointsTable;
   (*pPrimaryPointsTable)(0, 0) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
   (*pPrimaryPointsTable)(0, 1) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);




   RowIndexType row = pPrimaryPointsTable->GetNumberOfHeaderRows();
   std::vector<std::pair<Float64, Float64>>::const_iterator iter(primaryPoints.begin());
   std::vector<std::pair<Float64, Float64>>::const_iterator end(primaryPoints.end());
   for (; iter != end; iter++, row++)
   {
       Float64 x = iter->first;
       Float64 y = iter->second;

       (*pPrimaryPointsTable)(row, 0) << length.SetValue(x);
       (*pPrimaryPointsTable)(row, 1) << length.SetValue(y);
   }



   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Area.png"),
       _T("A = \\sum{ (x_{i + 1} - x_i)y_i + \\frac{1}{2}(x_{i + 1} - x_i)(y_{i + 1} - y_i) }")) << rptNewLine << rptNewLine;

   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ix.png"),
       _T("I_x = \\sum{\\frac{{y_i} ^ 3(x_{i + 1} - x_i)}{12} + (x_{i + 1} - x_i)y_i { \\frac{y_i}{2} } ^ 2 + \\frac{(y_{i + 1} - y_i) ^ 3 (x_{i + 1} - x_i)}{36}+ \\frac{1}{2} (x_{i + 1} - x_i)(y_{i + 1} - y_i) \\left[\\frac{(y_{i + 1} - y_i)}{3} + y_i \\right] ^ 3}")) << rptNewLine << rptNewLine;
   
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Iy.png"),
       _T("I_y = \\sum{ \\frac{ y_i(x_{i + 1} - x_i) ^ 3 }{12} + (x_{ i + 1 } - x_i)y_i \\left[x_i + \\frac{ (x_{i + 1} - x_i) }{2} \\right] ^ 2 + \\frac{ (y_{i + 1} - y_i)(x_{i + 1} - x_i) ^ 3 }{36} + \\frac{ 1 }{2} (x_{ i + 1 } - x_i)(y_{ i + 1 } - y_i) \\left[\\frac{ 2(x_{i + 1} - x_i) }{3} + x_i \\right] ^ 3}")) << rptNewLine << rptNewLine;
       
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ixy.png"),
       _T("I_{ xy } = \\sum{(x_{ i + 1 } - x_i)\\frac{ {y_i} ^ 2 }{2} \\left(x_i + \\frac{ x_{i + 1} - x_i }{2} \\right) + \\frac{ 1 }{2} (x_{ i + 1 } - x_i)(y_{ i + 1 } - y_i) \\left(\\frac{ y_{i + 1} - y_i }{3} - y_i  \\right) \\left[\\frac{ 2(x_{i + 1} - x_i) }{3} + x_i \\right] + \\frac{ (x_{i + 1} - x_i) ^ 2(y_{i + 1} - y_i) ^ 2 }{72}}")) << rptNewLine << rptNewLine;

   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Xbar.png"),
       _T("\\overline{ x } = \\frac{ \\sum{ \\frac{1}{2}y_i(x_{i + 1} - x_i)(x_{i + 1} + x_i) + \\frac{1}{2}(x_{i + 1} - x_i)(y_{i + 1} - y_i) \\left[\\frac{2}{3}(x_{i + 1} + x_i) + x_i \\right] } }{A}")) << rptNewLine << rptNewLine;
  
   *pPara << rptRcEquation(std::_tstring(rptStyleManager::GetImagePath()) + _T("Ybar.png"),
       _T("\\overline{y} = \\frac{ \\sum{ \\frac{1}{2}{y_i}^2(x_{i+1} - x_i) + \\frac{1}{2}(x_{i+1} - x_i)(y_{i+1} - y_i) \\left[ \\frac{1}{3}(y_{i+1} + y_i) + y_i \\right] } }{A}")) << rptNewLine << rptNewLine;

   //Properties


   CComPtr<IShapeProperties> pShapeProps;
   primaryShape->get_ShapeProperties(&pShapeProps);
   (*pLayoutTable)(0, 2) << Bold(_T("Basic Section Properties")) << rptNewLine;
   WriteSectionProperties((*pLayoutTable)(0, 2), pShapeProps);

   CComPtr<IShapeProperties> cShapeProps;

   if (secondaryShape)
   {
       rptRcTable* pSecondaryPointsTable = rptStyleManager::CreateDefaultTable(2, _T("Composite Piece"));

       (*pLayoutTable)(0, 1) << pSecondaryPointsTable;
       (*pSecondaryPointsTable)(0, 0) << COLHDR(_T("X"), rptLengthUnitTag, pDispUnits->ComponentDim);
       (*pSecondaryPointsTable)(0, 1) << COLHDR(_T("Y"), rptLengthUnitTag, pDispUnits->ComponentDim);

       row = pSecondaryPointsTable->GetNumberOfHeaderRows();
       iter = secondaryPoints.begin();
       end = secondaryPoints.end();
       for (; iter != end; iter++, row++)
       {
           Float64 x = iter->first;
           Float64 y = iter->second;

           (*pSecondaryPointsTable)(row, 0) << length.SetValue(x);
           (*pSecondaryPointsTable)(row, 1) << length.SetValue(y);
       }


       secondaryShape->get_ShapeProperties(&cShapeProps);
       (*pLayoutTable)(0, 3) << Bold(_T("Composite Section Properties")) << rptNewLine;
       WriteSectionProperties((*pLayoutTable)(0, 3), cShapeProps);

       Float64 n = EcDeck / EcGdr;
       n = ::RoundOff(n, 0.001);
       (*pLayoutTable)(0, 1) << _T("Modular Ratio, n = ") << n << rptNewLine;
   }


   (*pPara) << pLayoutTable;

   *pPara << rptNewPage;

   *pPara << CreateImage(primaryPoints, secondaryPoints) << rptNewLine;

   return pChapter;
}

void CSectionPropertiesChapterBuilder::WriteSectionProperties(rptParagraph& para, CComPtr<IShapeProperties>& shapeProps) const
{
    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

    INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDispUnits->ComponentDim, true);
    INIT_UV_PROTOTYPE(rptLength2UnitValue, area, pDispUnits->Area, true);
    INIT_UV_PROTOTYPE(rptLength4UnitValue, momentOfInertia, pDispUnits->MomentOfInertia, true);

    Float64 Area;
    shapeProps->get_Area(&Area);
    Float64 Ixx;
    shapeProps->get_Ixx(&Ixx);
    Float64 Iyy;
    shapeProps->get_Iyy(&Iyy);
    Float64 Ixy;
    shapeProps->get_Ixy(&Ixy);
    Float64 yt;
    shapeProps->get_Ytop(&yt);
    Float64 yb;
    shapeProps->get_Ybottom(&yb);
    Float64 xl;
    shapeProps->get_Xleft(&xl);
    Float64 xr;
    shapeProps->get_Xright(&xr);

    CComPtr<IPoint2d> pntCG;
    shapeProps->get_Centroid(&pntCG);

    Float64 xcg, ycg;
    pntCG->Location(&xcg, &ycg);

    para << _T("Area = ") << area.SetValue(Area) << rptNewLine;
    para << Sub2(_T("X"), _T("c.g.")) << _T(" = ") << length.SetValue(xcg) << rptNewLine;
    para << Sub2(_T("Y"), _T("c.g.")) << _T(" = ") << length.SetValue(ycg) << rptNewLine;
    para << _T("Iy = ") << momentOfInertia.SetValue(Iyy) << rptNewLine;
    para << _T("Ixy = ") << momentOfInertia.SetValue(Ixy) << rptNewLine;
}


rptRcImage* CSectionPropertiesChapterBuilder::CreateImage(
    const Points2D& primaryPoints, const Points2D& secondaryPoints) const
{
    CEAFApp* pApp = EAFGetApp();
    const WBFL::Units::IndirectMeasure* pDispUnits = pApp->GetDisplayUnits();

    CImage image;
    image.Create(500, 500, 32);
    CRect rect(CPoint(0, 0), CSize(image.GetWidth(), image.GetHeight()));

    CDC* pDC = CDC::FromHandle(image.GetDC());

    // fill the background of the image (otherwise it will be black)
    CBrush brush(ALICEBLUE);
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    pDC->Rectangle(rect);
    pDC->SelectObject(pOldBrush);

    WBFL::Units::LengthTool lengthTool(pDispUnits->ComponentDim);
    WBFL::Graphing::GraphXY graph(&lengthTool, &lengthTool);

    graph.SetOutputRect(rect);
    //graph.SetClientAreaColor(GRAPH_BACKGROUND);
    //graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

    graph.IsotropicAxes(true);
    graph.DrawGrid(true);
    graph.DrawLegend(false);

    //graph.SetTitle(_T("Interaction Diagram"));

    // Setup X-axis
    CString strXAxis;
    strXAxis.Format(_T("X (%s)"), pDispUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
    graph.SetXAxisTitle(strXAxis.LockBuffer());
    strXAxis.UnlockBuffer();
    graph.XAxisNiceRange(true);
    graph.SetXAxisNumberOfMinorTics(0);
    graph.SetXAxisNumberOfMajorTics(11);
    graph.SetXAxisLabelAngle(350); // 35 degrees

    // Setup Y-axis
    CString strYAxis;
    strYAxis.Format(_T("Y (%s)"), pDispUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
    graph.SetYAxisTitle(strYAxis.LockBuffer());
    strYAxis.UnlockBuffer();
    graph.YAxisNiceRange(true);
    graph.SetYAxisNumberOfMinorTics(0);
    graph.SetYAxisNumberOfMajorTics(11);

    IndexType primarySeries = graph.CreateDataSeries(_T(""), PS_SOLID, 1, BLUE);
    IndexType secondarySeries = graph.CreateDataSeries(_T(""), PS_SOLID, 1, BLUE);

    std::vector<std::pair<Float64, Float64>>::const_iterator iter(primaryPoints.begin());
    std::vector<std::pair<Float64, Float64>>::const_iterator end(primaryPoints.end());
    for (; iter != end; iter++)
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(iter->first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(iter->second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(primarySeries, point);
    }

    if (0 < primaryPoints.size())
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(primaryPoints.front().first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(primaryPoints.front().second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(primarySeries, point);
    }

    iter = secondaryPoints.begin();
    end = secondaryPoints.end();
    for (; iter != end; iter++)
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(iter->first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(iter->second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(secondarySeries, point);
    }

    if (0 < secondaryPoints.size())
    {
        WBFL::Graphing::Point point(WBFL::Units::ConvertFromSysUnits(secondaryPoints.front().first, pDispUnits->ComponentDim.UnitOfMeasure), WBFL::Units::ConvertFromSysUnits(secondaryPoints.front().second, pDispUnits->ComponentDim.UnitOfMeasure));
        graph.AddPoint(secondarySeries, point);
    }

    // Updates the graph metrics based on "nice" axis ranges
    graph.UpdateGraphMetrics(pDC->GetSafeHdc());
    graph.Draw(pDC->GetSafeHdc());

    image.ReleaseDC();


    // get a temporary file name for the image
    TCHAR temp_path[_MAX_PATH];
    TCHAR temp_file[_MAX_PATH];
    bool should_delete = true;

    if (::GetTempPath(_MAX_PATH, temp_path) == 0)
        _tcscpy_s(temp_path, _MAX_PATH, _T("C:\\")); // Couldn't establish a temp path, just use the root drive.

    //
    // Make sure the temp path actually exists
    // We do this by looking for any file in the directory.  If nothing shows up, then the
    // path doesn't exist. (Well, this isn't exactly true, but its the best I can come up
    // with).
    CFileFind finder;
    BOOL bExist;
    CString path(temp_path);
    if (path[path.GetLength() - 1] != '\\')
        path += _T("\\");
    path += _T("*.*");
    bExist = finder.FindFile(path);
    if (!bExist)
        _tcscpy_s(temp_path, _MAX_PATH, _T("C:\\"));

    // This creates a file called _T("temp_file").TMP
    if (::GetTempFileName(temp_path, _T("gencomp_"), 0, temp_file) == 0)
    {
        // We could not get a temp name, so just use this default
        // (Use a tmp extension so it is in the same format as the one
        //  the OS would have created for us)
        _tcscpy_s(temp_file, _MAX_PATH, _T("gencomp.tmp"));
        should_delete = false;
    }

    // Replace the TMP extension with png
    std::_tstring strFilename;
    strFilename.assign(temp_file);
    strFilename.replace(strFilename.end() - 3, strFilename.end(), _T("png"));

    // We don't want the file Windows created for us
    if (should_delete)
        ::DeleteFile(temp_file);

    make_upper(strFilename.begin(), strFilename.end());

    // this is a const function so we have to cast away const-ness to save
    // the file name
    m_TemporaryFiles.push_back(strFilename);

    image.Save(strFilename.c_str(), Gdiplus::ImageFormatPNG);

    rptRcImage* pImage = new rptRcImage(strFilename.c_str(), rptRcImage::Baseline, rptRcImage::None);
    return pImage;
}
