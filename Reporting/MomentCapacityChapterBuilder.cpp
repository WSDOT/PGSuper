///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <Reporting\MomentCapacityChapterBuilder.h>
#include <Reporting\MomentCapacityReportSpecification.h>

#include <IReportManager.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>
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


CMomentCapacityChapterBuilder::CMomentCapacityChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

CMomentCapacityChapterBuilder::~CMomentCapacityChapterBuilder(void)
{
   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_TemporaryImageFiles.begin(); iter != m_TemporaryImageFiles.end(); iter++ )
   {
      std::_tstring file = *iter;
      ::DeleteFile( file.c_str() );
   }
}

LPCTSTR CMomentCapacityChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity Details");
}

rptChapter* CMomentCapacityChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CMomentCapacityReportSpecification* pSpec = dynamic_cast<CMomentCapacityReportSpecification*>(pRptSpec);
   pgsPointOfInterest poi( pSpec->GetPOI() );
   bool bPositiveMoment = pSpec->IsPositiveMoment();

   const CSegmentKey& segmentKey( poi.GetSegmentKey() );

   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   /////////////////////////////////////
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cg,     pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dist,   pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar strain;
   strain.SetFormat(sysNumericFormatTool::Automatic);
   strain.SetWidth(7);
   strain.SetPrecision(3);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());


   location.PrefixAttributes(false); // put the attributes after the location
   location.IncludeSpanAndGirder(false);

   // Results
   (*pPara) << (bPositiveMoment ? _T("Positive Moment") : _T("Negative Moment")) << rptNewLine;
   (*pPara) << _T("Span ") << LABEL_SPAN(spanKey.spanIndex) << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex) << rptNewLine;
   (*pPara) << _T("Location from Start of Span ") << location.SetValue(POI_SPAN,poi) << rptNewLine;
   (*pPara) << _T("Depth to neutral axis, c = ") << dist.SetValue(pmcd->c) << rptNewLine;
   (*pPara) << _T("Compression Resultant, C = ") << force.SetValue(pmcd->C) << rptNewLine;
   (*pPara) << _T("Depth to Compression Resultant, ") << Sub2(_T("d"),_T("c")) << _T(" = ") << dist.SetValue(pmcd->dc) << rptNewLine;
   (*pPara) << _T("Tension Resultant, T = ") << force.SetValue(pmcd->T) << rptNewLine;
   (*pPara) << _T("Depth to Tension Resultant, ") << Sub2(_T("d"),_T("e")) << _T(" = ") << dist.SetValue(pmcd->de) << rptNewLine;
   (*pPara) << _T("Depth to Tension Resultant (for shear), ") << Sub2(_T("d"),_T("e")) << _T(" = ") << dist.SetValue(pmcd->de_shear) << rptNewLine;
   (*pPara) << Sub2(symbol(epsilon), _T("t")) << _T(" x 1000 = ") << strain.SetValue(pmcd->et * 1000) << rptNewLine;
   (*pPara) << symbol(phi) << _T(" =") << scalar.SetValue(pmcd->Phi) << rptNewLine;
   (*pPara) << _T("Nominal Capacity, ") << Sub2(_T("M"), _T("n")) << _T(" = ") << moment.SetValue(pmcd->Mn) << rptNewLine;
   (*pPara) << _T("Nominal Resistance, ") << Sub2(_T("M"),_T("r")) << _T(" = ") << symbol(phi) << Sub2(_T("M"), _T("n")) << _T(" = ") << moment.SetValue(pmcd->Mr) << rptNewLine;
   (*pPara) << _T("Moment Arm = ") << Sub2(_T("d"),_T("e")) << _T(" - ") << Sub2(_T("d"),_T("c")) << _T(" = ") << Sub2(_T("M"),_T("n")) << _T("/T = ") << dist.SetValue(pmcd->MomentArm) << rptNewLine;

   //std::array<std::_tstring, 3> strControl{ _T("concrete crushing"), _T("maximum reinforcement strain"), _T("maximum reinforcement strain with stress limited by lack of full development [5.9.4.3.2]") };
   //(*pPara) << _T("Moment capacity controlled by ") << strControl[pmcd->Controlling] << rptNewLine;

   // if this is a zero capacity section, just return since there is nothing else to show
   if ( IsZero(pmcd->Mn) )
   {
      return pChapter;
   }

   pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Strain Compatibility Analysis") << rptNewLine;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   // Image
   *pPara << CreateImage(pmcd->CapacitySolution, bPositiveMoment);

   // Table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(16,_T(""));
   (*pPara) << pTable << rptNewLine;

   pTable->SetColumnStyle(1, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(1, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("Slice");
   (*pTable)(0, col++) << _T("Piece");
   (*pTable)(0, col++) << COLHDR(_T("Area"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("Y"), _T("top")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("Y"), _T("bot")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(Sub2(_T("Y"), _T("cg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*pTable)(0, col++) << COLHDR(RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << _T("Initial") << rptNewLine << _T("Strain");
   (*pTable)(0, col++) << _T("Incremental") << rptNewLine << _T("Strain");
   (*pTable)(0, col++) << _T("Total") << rptNewLine << _T("Strain");
   (*pTable)(0, col++) << RPT_FPX << _T("/") << RPT_FPS;
   //(*pTable)(0, col++) << _T("Strain") << rptNewLine << _T("Limit"); // want to report this if strains are limited
   (*pTable)(0, col++) << COLHDR(_T("Foreground") << rptNewLine << _T("Stress (Fg)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(_T("Background") << rptNewLine << _T("Stress (Bg)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(_T("Stress") << rptNewLine << _T("(Fg - Bg)"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
   (*pTable)(0, col++) << COLHDR(symbol(delta) << _T("F =") << rptNewLine << _T("(Area)(Stress)"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
   (*pTable)(0, col++) << COLHDR(symbol(delta) << _T("M =") << rptNewLine << _T("(") << symbol(delta) << _T("F") << _T(")(") << Sub2(_T("Y"), _T("cg")) << _T(")"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());

   force.ShowUnitTag(false);
   moment.ShowUnitTag(false);


   CComPtr<IGeneralSectionSolution> general_solution;
   pmcd->CapacitySolution->get_GeneralSectionSolution(&general_solution);

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   Float64 sum_force = 0;
   Float64 sum_moment = 0;
   CollectionIndexType nSlices;
   general_solution->get_SliceCount(&nSlices);
   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      col = 0;

      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      IndexType shapeIdx;
      slice->get_ShapeIndex(&shapeIdx);
      CComBSTR bstrName;
      pmcd->Section->get_Name(shapeIdx, &bstrName);

      Float64 slice_area;
      slice->get_Area(&slice_area);

      CComPtr<IPoint2d> pntCG;
      slice->get_CG(&pntCG);

      Float64 cgY;
      pntCG->get_Y(&cgY);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);
      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> pntTop, pntBottom;
      position->get_LocatorPoint(lpTopCenter, &pntTop);
      position->get_LocatorPoint(lpBottomCenter, &pntBottom);
      Float64 Yt, Yb;
      pntTop->get_Y(&Yt);
      pntBottom->get_Y(&Yb);

      CComPtr<ILine2d> naLine;
      pmcd->CapacitySolution->get_NeutralAxis(&naLine);
      CComPtr<IPoint2d> p;
      CComPtr<IVector2d> v;
      naLine->GetExplicit(&p, &v);
      Float64 angle;
      v->get_Direction(&angle);
      Yt *= cos(angle);
      Yb *= cos(angle);
      if (Yt < Yb) std::swap(Yt, Yb);

      Float64 initial_strain;
      slice->get_InitialStrain(&initial_strain);

      Float64 incremental_strain;
      slice->get_IncrementalStrain(&incremental_strain);

      Float64 total_strain;
      slice->get_TotalStrain(&total_strain);

      Float64 f = 0;
      Float64 emin = 0, emax = 0;
      Float64 E = 0;

      CComPtr<IStressStrain> ss;
      slice->get_ForegroundMaterial(&ss);
      if (ss)
      {
         ss->ComputeStress(total_strain, &f);
         ss->StrainLimits(&emin, &emax);
         ss->get_ModulusOfElasticity(&E);
      }

      Float64 fgStress,bgStress,netStress;
      slice->get_ForegroundStress(&fgStress);
      slice->get_BackgroundStress(&bgStress);


      ATLASSERT(IsEqual(f, fgStress));

      netStress = fgStress - bgStress;

      Float64 F = slice_area * netStress;
      Float64 M = F*cgY;

      Float64 fpe = E * initial_strain;

      Float64 fpx_fps = 0.0;
      CComQIPtr<IPowerFormula> pf(ss);
      if (pf)
      {
         pf->get_ReductionFactor(&fpx_fps);
      }

      (*pTable)(row, col++) << row;
      (*pTable)(row, col++) << OLE2T(bstrName);
      (*pTable)(row, col++) << area.SetValue(slice_area);
      (*pTable)(row, col++) << cg.SetValue(Yt);
      (*pTable)(row, col++) << cg.SetValue(Yb);
      (*pTable)(row, col++) << cg.SetValue(cgY);
      if (IsZero(fpe))
      {
         (*pTable)(row, col++) << _T("");
      }
      else
      {
         (*pTable)(row, col++) << stress.SetValue(fpe);
      }
      (*pTable)(row, col++) << initial_strain;
      (*pTable)(row, col++) << incremental_strain;

      VARIANT_BOOL vbExceededStrainLimit;
      slice->ExceededStrainLimit(&vbExceededStrainLimit);
      if (vbExceededStrainLimit == VARIANT_TRUE)
      {
         (*pTable)(row, col).SetFillBackGroundColor(rptRiStyle::Red);
         (*pTable)(row, col++) << total_strain << _T(" (") << (total_strain < 0 ? emin : emax) << _T(")");
      }
      else
      {
         (*pTable)(row, col++) << total_strain;
      }

      if (IsZero(fpx_fps))
      {
         (*pTable)(row, col++) << _T("");
      }
      else
      {
         (*pTable)(row, col++) << fpx_fps;
      }
      //(*pTable)(row, col++) << (fgStress < 0 ? emin : emax); // report if strains are limited
      (*pTable)(row, col++) << stress.SetValue(fgStress);
      (*pTable)(row, col++) << stress.SetValue(bgStress);
      (*pTable)(row, col++) << stress.SetValue(netStress);
      (*pTable)(row, col++) << force.SetValue(F);
      (*pTable)(row, col++) << moment.SetValue(M);

      sum_force  += F;
      sum_moment += M;

      row++;
   }

   force.ShowUnitTag(true);
   moment.ShowUnitTag(true);

   *pPara << _T("Resultant Force  = ") << symbol(SUM) << _T("(") << symbol(delta) << _T("F) = ") << force.SetValue(sum_force)   << rptNewLine;
   *pPara << _T("Resultant Moment = ") << symbol(SUM) << _T("(") << symbol(delta) << _T("M) = ") << moment.SetValue(sum_moment) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Foreground (Fg) stress = stress in section materials (including voided areas)") << rptNewLine;
   *pPara << _T("Background (Bg) stress = stress in voids that is subtracted from voided area") << rptNewLine;
   *pPara << rptNewLine;

   std::_tstring strImagePath = rptStyleManager::GetImagePath();

   GET_IFACE2(pBroker, IMaterials, pMaterials);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool bUHPC = pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC || (pBridgeDesc->HasStructuralLongitudinalJoints() ? pMaterials->GetLongitudinalJointConcreteType() == pgsTypes::PCI_UHPC : false);
   if (bUHPC)
   {
      *pPara << _T("PCI UHPC Concrete Stress-Strain Model (PCI UHPC SDG E.6.1)") << rptNewLine;
      *pPara << rptRcImage(strImagePath + _T("PCIUHPCConcrete.png")) << rptNewLine;
   }

   bool bUnconfinedConcrete = pMaterials->GetSegmentConcreteType(segmentKey) != pgsTypes::PCI_UHPC || (pBridgeDesc->HasStructuralLongitudinalJoints() ? pMaterials->GetLongitudinalJointConcreteType() != pgsTypes::PCI_UHPC : false);
   if (bUnconfinedConcrete)
   {
      *pPara << _T("Unconfined Concrete Stress-Strain Model") << rptNewLine;
      *pPara << rptRcImage(strImagePath + _T("UnconfinedConcrete.png")) << rptNewLine;
   }

   *pPara << _T("Prestressing Strand Model") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("PowerFormula.png")) << rptNewLine;

   *pPara << _T("Reinforcement Model") << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("Rebar.png")) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CMomentCapacityChapterBuilder::Clone() const
{
   return new CMomentCapacityChapterBuilder;
}

rptRcImage* CMomentCapacityChapterBuilder::CreateImage(IMomentCapacitySolution* pSolution,bool bPositiveMoment) const
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

   // This creates a file called "temp_file".TMP
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
   CMomentCapacityChapterBuilder* pMe = const_cast<CMomentCapacityChapterBuilder*>(this);
   pMe->m_TemporaryImageFiles.push_back(strFilename);

   image.Save(strFilename.c_str(),Gdiplus::ImageFormatPNG);

   rptRcImage* pImage = new rptRcImage(strFilename.c_str(),rptRcImage::Baseline);
   return pImage;

}

void CMomentCapacityChapterBuilder::DrawSection(CImage& image,IMomentCapacitySolution* pSolution,bool bPositiveMoment) const
{
   CComPtr<IGeneralSectionSolution> general_solution;
   pSolution->get_GeneralSectionSolution(&general_solution);
   CollectionIndexType nSlices;
   general_solution->get_SliceCount(&nSlices);

   // determine the bounding box
   CComPtr<IRect2d> bbox;
   for ( CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++ )
   {
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

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
   grlibPointMapper mapper;
   mapper.SetMappingMode(grlibPointMapper::Isotropic);
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
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      Float64 fgStress, bgStress;
      slice->get_ForegroundStress(&fgStress);
      slice->get_BackgroundStress(&bgStress);

      Float64 stress = fgStress - bgStress;

      CComPtr<IStressStrain> fgMaterial,bgMaterial;
      slice->get_ForegroundMaterial(&fgMaterial);
      slice->get_BackgroundMaterial(&bgMaterial);

      if ( fgMaterial == nullptr && bgMaterial != nullptr )
      {
         // this is a void
         // save the slice index and go to the next slice
         voidIndices.push_back(sliceIdx);
         continue;
      }

      if ( stress < 0 )
      {
         compressionIndices.push_back(sliceIdx);
      }
      else if ( 0 < stress )
      {
         tensionIndices.push_back(sliceIdx);
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
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw compression slices
   pDC->SelectObject(&compressionBrush);
   for ( iter = compressionIndices.begin(); iter != compressionIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw the voids on top of the foreground shape
   pDC->SelectObject(&voidBrush);
   for ( iter = voidIndices.begin(); iter != voidIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw tension slices
   pDC->SelectObject(&tensionBrush);
   CPen tensionPen(PS_SOLID,1,TENSION_COLOR);
   pDC->SelectObject(&tensionPen);
   for ( iter = tensionIndices.begin(); iter != tensionIndices.end(); iter++ )
   {
      CollectionIndexType sliceIdx = *iter;
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   pDC->SelectObject(pOldBrush);
   pDC->SelectObject(pOldPen);

   // draw the strain profile
   CPen pen(PS_SOLID,1,BLACK);
   pDC->SelectObject(&pen);

   Float64 top, bottom, left, right;
   bbox->get_Top(&top);
   bbox->get_Bottom(&bottom);
   bbox->get_Left(&left);
   bbox->get_Right(&right);

   //// Draw Y = 0 line
   CPoint p;
   //mapper.WPtoDP(left,0,&p.x,&p.y);
   //pDC->MoveTo(p);
   //mapper.WPtoDP(right,0,&p.x,&p.y);
   //pDC->LineTo(p);

   CComPtr<IPlane3d> strain_plane;
   pSolution->get_StrainPlane(&strain_plane);

   Float64 eTop, eBottom; // strain top and bottom
   strain_plane->GetZ(0,mirror_factor*top,&eTop);
   strain_plane->GetZ(0,mirror_factor*bottom,&eBottom);

   // scale strains so that they plot with the same
   // aspect ratio as the section
   Float64 strain = Max(fabs(eBottom),fabs(eTop));
   Float64 scale = (wx/4)/strain;
   eTop    *= scale;
   eBottom *= scale;
   strain  *= scale;

   mapper.SetDeviceOrg(3*image.GetWidth()/4+2,2);

   if ( !bPositiveMoment )
   {
      std::swap(top,bottom);
      std::swap(eTop,eBottom);
   }

   // negate the mirror factor so the strain plane draws correctly
   mapper.GetWorldExt(&wx,&wy);
   wx *= mirror_factor;
   mapper.SetWorldExt(wx,wy);

   mapper.WPtoDP(0,top,&p.x,&p.y);
   pDC->MoveTo(p);
   mapper.WPtoDP(eTop,top,&p.x,&p.y);
   pDC->LineTo(p);
   mapper.WPtoDP(eBottom,bottom,&p.x,&p.y);
   pDC->LineTo(p);
   mapper.WPtoDP(0,bottom,&p.x,&p.y);
   pDC->LineTo(p);
   mapper.WPtoDP(0,top,&p.x,&p.y);
   pDC->LineTo(p);

   // Draw the compression resultant
   CPen cPen(PS_SOLID,5,COMPRESSION_COLOR);
   pDC->SelectObject(&cPen);

   CComPtr<IPoint2d> pntC;
   pSolution->get_CompressionResultantLocation(&pntC);
   Float64 y;
   pntC->get_Y(&y);

   if ( !bPositiveMoment )
   {
      y *= -1;
   }

   mapper.WPtoDP(strain,y,&p.x,&p.y); 
   pDC->MoveTo(p);   
   mapper.WPtoDP(0,y,&p.x,&p.y);
   pDC->LineTo(p);

   // arrow head
   pDC->MoveTo(p.x+5,p.y-5);
   pDC->LineTo(p);
   pDC->MoveTo(p.x+5,p.y+5);
   pDC->LineTo(p);

   // Draw the tension resultant
   CPen tPen(PS_SOLID,5,TENSION_COLOR);
   pDC->SelectObject(&tPen);

   CComPtr<IPoint2d> pntT;
   pSolution->get_TensionResultantLocation(&pntT);
   pntT->get_Y(&y);

   if ( !bPositiveMoment )
      y *= -1;

   mapper.WPtoDP(0,y,&p.x,&p.y);
   pDC->MoveTo(p);   
   mapper.WPtoDP(strain,y,&p.x,&p.y);
   pDC->LineTo(p);   


   // arrow head
   pDC->MoveTo(p.x-5,p.y-5);
   pDC->LineTo(p);
   pDC->MoveTo(p.x-5,p.y+5);
   pDC->LineTo(p);

   pDC->SelectObject(pOldPen);

   image.ReleaseDC();
}

void CMomentCapacityChapterBuilder::DrawSlice(IShape* pShape,CDC* pDC,grlibPointMapper& mapper) const
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