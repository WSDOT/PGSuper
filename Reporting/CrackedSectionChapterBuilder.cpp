///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#include "StdAfx.h"
#include <Reporting\CrackedSectionChapterBuilder.h>
#include <Reporting\CrackedSectionReportSpecification.h>

#include <IReportManager.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\CrackedSection.h>

#include <PgsExt\GirderLabel.h>

#include <PGSuperColors.h>

#include <algorithm>
//#include "Helpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const COLORREF BACKGROUND_COLOR  = WHITE;
static const COLORREF VOID_COLOR        = WHITE;
static const COLORREF COMPRESSION_COLOR = RED;
static const COLORREF TENSION_COLOR     = BLUE;


CCrackedSectionChapterBuilder::CCrackedSectionChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

CCrackedSectionChapterBuilder::~CCrackedSectionChapterBuilder(void)
{
   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_TemporaryImageFiles.begin(); iter != m_TemporaryImageFiles.end(); iter++ )
   {
      std::_tstring file = *iter;
      ::DeleteFile( file.c_str() );
   }
}

LPCTSTR CCrackedSectionChapterBuilder::GetName() const
{
   return TEXT("Cracked Section Computation Details");
}

rptChapter* CCrackedSectionChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   auto pSpec = std::dynamic_pointer_cast<const CCrackedSectionReportSpecification>(pRptSpec);
   pgsPointOfInterest poi = pSpec->GetPOI();
   bool bPositiveMoment = pSpec->IsPositiveMoment();

   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   GET_IFACE2(pBroker,ICrackedSection,pCrackedSection);
   const CRACKEDSECTIONDETAILS* pCSD = pCrackedSection->GetCrackedSectionDetails(poi,bPositiveMoment);

   /////////////////////////////////////
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cg,     pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetModEUnit(),       false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetSmallMomentUnit(),       true);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dist,   pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, momI, pDisplayUnits->GetMomentOfInertiaUnit(), true);

#pragma Reminder("UPDATE: assuming precast girder bridge")
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   SpanIndexType spanIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   location.PrefixAttributes(false); // put the attributes after the location

   //// Results
   (*pPara) << (bPositiveMoment ? _T("Positive Moment") : _T("Negative Moment")) << rptNewLine;
   (*pPara) << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
   (*pPara) << _T("Location from Left Support ") << location.SetValue(POI_SPAN, poi) << rptNewLine;
   (*pPara) << _T("Depth to neutral axis, c = ") << dist.SetValue(pCSD->c) << rptNewLine;
   (*pPara) << _T("Cracked section moment of inertia, ") << Sub2(_T("I"),_T("cr")) << _T(" = ") << momI.SetValue(pCSD->Icr) << rptNewLine;

   // Image
   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Cracked Section Analysis") << rptNewLine;

   pPara = new rptParagraph;
   (*pChapter) << pPara;
   rptRcImage* pImage = CreateImage(pCSD->CrackedSectionSolution,bPositiveMoment);
   (*pPara) << pImage << rptNewLine << rptNewLine;

   // Table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(7,_T(""));
   (*pPara) << pTable << rptNewLine;

   ColumnIndexType col = 0;
   (*pTable)(0,col++) << _T("Slice");
   (*pTable)(0,col++) << COLHDR(_T("Area"),rptAreaUnitTag,pDisplayUnits->GetAreaUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(_T("Y"),_T("cg")), rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(_T("E"),_T("fg")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   (*pTable)(0,col++) << COLHDR(Sub2(_T("E"),_T("bg")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   (*pTable)(0,col++) << COLHDR(_T("(") << Sub2(_T("E"),_T("fg")) << _T(" - ") << Sub2(_T("E"),_T("bg")) << _T(")(Area)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*pTable)(0,col++) << COLHDR(_T("(") << Sub2(_T("E"),_T("fg")) << _T(" - ") << Sub2(_T("E"),_T("bg")) << _T(")(Area)(") << Sub2(_T("Y"),_T("cg")) << _T(")"), rptMomentUnitTag, pDisplayUnits->GetSmallMomentUnit() );

   force.ShowUnitTag(false);
   moment.ShowUnitTag(false);

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   Float64 EA  = 0;
   Float64 EAY = 0;
   CollectionIndexType nSlices;
   pCSD->CrackedSectionSolution->get_SliceCount(&nSlices);
   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      CComPtr<ICrackedSectionSlice> slice;
      pCSD->CrackedSectionSolution->get_Slice(sliceIdx,&slice);

      Float64 slice_area;
      slice->get_Area(&slice_area);

      CComPtr<IPoint2d> pntCG;
      slice->get_CG(&pntCG);

      Float64 cgY;
      pntCG->get_Y(&cgY);

      Float64 Efg, Ebg;
      slice->get_Efg(&Efg);
      slice->get_Ebg(&Ebg);

      Float64 E = Efg - Ebg;

      Float64 ea = E*slice_area;
      Float64 eay = ea*cgY;

      col = 0;
      (*pTable)(row,col++) << row;
      (*pTable)(row,col++) << area.SetValue(slice_area);
      (*pTable)(row,col++) << cg.SetValue(cgY);
      (*pTable)(row,col++) << stress.SetValue(Efg);
      (*pTable)(row,col++) << stress.SetValue(Ebg);
      (*pTable)(row,col++) << force.SetValue(ea);
      (*pTable)(row,col++) << moment.SetValue(eay);

      EA  += ea;
      EAY += eay;

      row++;
   }

   force.ShowUnitTag(true);
   moment.ShowUnitTag(true);

   (*pPara) << symbol(SUM) << _T("EA  = ") << force.SetValue(EA)   << rptNewLine;
   (*pPara) << symbol(SUM) << _T("EAY = ") << moment.SetValue(EAY)   << rptNewLine;
   (*pPara) << _T("Y = ") << symbol(SUM) << _T("EAY") << _T("/") << symbol(SUM) << _T("EA  = ") << dist.SetValue( EAY/EA ) << rptNewLine;
   (*pPara) << rptNewLine;
   (*pPara) << _T("The cracked section modulus ") << Sub2(_T("I"),_T("cr")) << _T(" is used for the permit rating analysis to evaluate yielding of flexural reinforcement at the Service I limit state. See MBE 6A.5.4.2.2b for guidance.") << rptNewLine;

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CCrackedSectionChapterBuilder::Clone() const
{
   return std::make_unique<CCrackedSectionChapterBuilder>();
}

rptRcImage* CCrackedSectionChapterBuilder::CreateImage(ICrackedSectionSolution* pSolution,bool bPositiveMoment) const
{
   CImage image;
   DrawSection(image,pSolution,bPositiveMoment);

   // get a temporary file name for the image
   TCHAR temp_path[ _MAX_PATH ];
   TCHAR temp_file[ _MAX_PATH ];
   bool should_delete = true;

   if ( ::GetTempPath( _MAX_PATH, temp_path ) == 0 )
      _tcscpy_s(temp_path,_MAX_PATH,_T("\\")); // Couldn't establish a temp path, just use the root drive.

   //
   // Make sure the temp path actually exists
   // We do this by looking for any file in the directory.  If nothing shows up, then the
   // path doesn't exist. (Well, this isn't exactly true, but its the best I can come up
   // with).
   CFileFind finder;
   BOOL bExist;
   CString path(temp_path);
   if ( path[path.GetLength()-1] != '\\' )
   {
      path += _T("\\");
   }

   path += _T("*.*");
   bExist = finder.FindFile(path);
   if ( !bExist )
   {
      _tcscpy_s( temp_path,_MAX_PATH, _T("\\") );
   }

   // This creates a file called _T("temp_file").TMP
   if ( ::GetTempFileName( temp_path, _T("bsexp_"), 0, temp_file ) == 0 )
   {
      // We could not get a temp name, so just use this default
      // (Use a tmp extension so it is in the same format as the one
      //  the OS would have created for us)
      _tcscpy_s( temp_file, _MAX_PATH, _T("bsexp.tmp") );
      should_delete = false;
   }

   // Replace the TMP extension with png
   std::_tstring strFilename;
   strFilename.assign( temp_file );
   strFilename.replace( strFilename.end() - 3, strFilename.end(), _T("png") );

   // We don't want the file Windows created for us
   if ( should_delete )
      ::DeleteFile( temp_file );

   std::transform(std::begin(strFilename), std::end(strFilename), std::begin(strFilename), [](auto& c) {return toupper(c); });

   // this is a const function so we have to cast away const-ness to save
   // the file name
   CCrackedSectionChapterBuilder* pMe = const_cast<CCrackedSectionChapterBuilder*>(this);
   pMe->m_TemporaryImageFiles.push_back(strFilename);

   image.Save(strFilename.c_str(),Gdiplus::ImageFormatPNG);

   rptRcImage* pImage = new rptRcImage(strFilename.c_str(),rptRcImage::Baseline);
   return pImage;

}

void CCrackedSectionChapterBuilder::DrawSection(CImage& image,ICrackedSectionSolution* pSolution,bool bPositiveMoment) const
{
   CollectionIndexType nSlices;
   pSolution->get_SliceCount(&nSlices);

   // determine the bounding box
   CComPtr<IRect2d> bbox;
   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      CComPtr<ICrackedSectionSlice> slice;
      pSolution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      if ( sliceIdx == 0 )
      {
         shape->get_BoundingBox(&bbox);
      }
      else
      {
         CComPtr<IRect2d> box;
         shape->get_BoundingBox(&box);
         bbox->Union(box);
      }
   }

   // select image size
   Float64 wx,wy;
   bbox->get_Width(&wx);
   bbox->get_Height(&wy);
   Float64 aspect_ratio = wx/wy;

   int base_dimension = 400;
   Float64 width_scale = 1.5;
   if ( aspect_ratio < 1 )
   {
      // section is taller than it is wide
      image.Create(int(width_scale*aspect_ratio*base_dimension)+15,base_dimension+15,32);
   }
   else
   {
      // section is wider than it is tall
      image.Create(int(width_scale*base_dimension)+15,int(base_dimension/aspect_ratio)+15,32);
   }


   // set up coordinate mapping
   Float64 mirror_factor = (bPositiveMoment ? 1 : -1); // if neg moment, mirror x and y (so the result is right side up)
   WBFL::Graphing::PointMapper mapper;
   mapper.SetMappingMode(WBFL::Graphing::PointMapper::MapMode::Isotropic);
   mapper.SetWorldExt(mirror_factor*width_scale*wx,mirror_factor*wy);

   Float64 orgY;
   if ( bPositiveMoment )
      bbox->get_Top(&orgY);
   else
      bbox->get_Bottom(&orgY);

   mapper.SetWorldOrg(0,orgY);

   mapper.SetDeviceExt(image.GetWidth(),image.GetHeight());
   mapper.SetDeviceOrg(int(image.GetWidth()/(width_scale*2))+2,2);

   // Fill image background (otherwise it will be black)
   CDC* pDC = CDC::FromHandle(image.GetDC());
   CRect rect(CPoint(0,0),CSize(image.GetWidth(),image.GetHeight()));
   CBrush bgBrush(BACKGROUND_COLOR);
   CPen bgPen(PS_SOLID,1,BACKGROUND_COLOR);
   CBrush* pOldBrush = pDC->SelectObject(&bgBrush);
   CPen* pOldPen = pDC->SelectObject(&bgPen);
   pDC->Rectangle(rect);
   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   // draw each slice
   CPen girderPen(PS_SOLID,1,WHITESMOKE);
   CBrush girderBrush(SEGMENT_FILL_COLOR);
   CBrush voidBrush(VOID_COLOR);
   CBrush tensionBrush(TENSION_COLOR);
   CBrush compressionBrush(COMPRESSION_COLOR);

   pOldPen = pDC->SelectObject(&girderPen);

   std::vector<CollectionIndexType> voidIndices;        // contains slice index for void slices
   std::vector<CollectionIndexType> neutralIndices;     // contains slice index for neutral slices
   std::vector<CollectionIndexType> tensionIndices;     // contains slice index for tension slices
   std::vector<CollectionIndexType> compressionIndices; // contains slice index for compression slices

   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      CComPtr<ICrackedSectionSlice> slice;
      pSolution->get_Slice(sliceIdx,&slice);

      Float64 Efg, Ebg;
      slice->get_Efg(&Efg);
      slice->get_Ebg(&Ebg);

      if ( IsZero(Efg) && !IsZero(Ebg) )
      {
         // this is a void
         // save the slice index and go to the next slice
         voidIndices.push_back(sliceIdx);
         continue;
      }

      if ( !IsZero(Efg) )
      {
         compressionIndices.push_back(sliceIdx);
      }
      else
      {
         neutralIndices.push_back(sliceIdx);
      }
   }

   std::vector<CollectionIndexType>::iterator iter;
   // draw neutral slices first
   pOldBrush = pDC->SelectObject(&girderBrush);
   for ( iter = neutralIndices.begin(); iter != neutralIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<ICrackedSectionSlice> slice;
      pSolution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw compression slices
   pDC->SelectObject(&compressionBrush);
   for ( iter = compressionIndices.begin(); iter != compressionIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<ICrackedSectionSlice> slice;
      pSolution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   //// draw tension slices
   //pDC->SelectObject(&tensionBrush);
   //for ( iter = tensionIndices.begin(); iter != tensionIndices.end(); iter++ )
   //{
   //   CollectionIndexType sliceIdx = *iter;
   //   CComPtr<IGeneralSectionSlice> slice;
   //   general_solution->get_Slice(sliceIdx,&slice);

   //   CComPtr<IShape> shape;
   //   slice->get_Shape(&shape);

   //   DrawSlice(shape,pDC,mapper);
   //}

   // draw the voids on top of the foreground shape
   pDC->SelectObject(&voidBrush);
   for ( iter = voidIndices.begin(); iter != voidIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<ICrackedSectionSlice> slice;
      pSolution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   image.ReleaseDC();
}

void CCrackedSectionChapterBuilder::DrawSlice(IShape* pShape,CDC* pDC, WBFL::Graphing::PointMapper& mapper) const
{
   CComPtr<IPoint2dCollection> objPoints;
   pShape->get_PolyPoints(&objPoints);

   CollectionIndexType nPoints;
   objPoints->get_Count(&nPoints);
   if (nPoints < 3)
   {
       CComPtr<IPoint2d> pnt;
       objPoints->get_Item(0, &pnt);

       Float64 x, y;
       pnt->Location(&x, &y);

       LONG dx, dy;
       mapper.WPtoDP(x, y, &dx, &dy);

       CRect box(CPoint(dx, dy), CSize(0, 0));
       box.top -= 2;
       box.bottom += 2;
       box.left -= 2;
       box.right += 2;

       pDC->Rectangle(box);
   }
   else
   {
       CPoint* points = new CPoint[nPoints];
       for (CollectionIndexType pntIdx = 0; pntIdx < nPoints; pntIdx++)
       {
           CComPtr<IPoint2d> point;
           objPoints->get_Item(pntIdx, &point);
           Float64 x, y;
           point->get_X(&x);
           point->get_Y(&y);

           LONG dx, dy;
           mapper.WPtoDP(x, y, &dx, &dy);

           points[pntIdx] = CPoint(dx, dy);
       }

       pDC->Polygon(points, (int)nPoints);

       delete[] points;
   }
}