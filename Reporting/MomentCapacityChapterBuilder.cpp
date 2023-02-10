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
#include <Reporting\MomentCapacityChapterBuilder.h>
#include <Reporting\MomentCapacityReportSpecification.h>

#include <IReportManager.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <PGSuperColors.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const COLORREF BACKGROUND_COLOR  = WHITE;
static const COLORREF VOID_COLOR        = WHITE;
static const COLORREF COMPRESSION_FILL_COLOR = RED;
static const COLORREF COMPRESSION_BORDER_COLOR = ORANGERED;
static const COLORREF TENSION_FILL_COLOR     = BLUE;
static const COLORREF TENSION_BORDER_COLOR = ROYALBLUE;


CMomentCapacityChapterBuilder::CMomentCapacityChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

CMomentCapacityChapterBuilder::~CMomentCapacityChapterBuilder(void)
{
   std::vector<std::_tstring>::iterator iter;
   for(const auto& file : m_TemporaryImageFiles)
   {
      ::DeleteFile( file.c_str() );
   }
}

LPCTSTR CMomentCapacityChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity Details");
}

rptChapter* CMomentCapacityChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   auto pSpec = std::dynamic_pointer_cast<const CMomentCapacityReportSpecification>(pRptSpec);
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

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ISpecification, pSpecification);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpecification->GetSpecification().c_str());
   bool bConsiderReinforcementStrainLimits = pSpecEntry->ConsiderReinforcementStrainLimitForMomentCapacity();

   GET_IFACE2(pBroker, IMomentCapacity, pMomentCapacity);
   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);


   GET_IFACE2(pBroker, IMaterials, pMaterials);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool bPCIUHPC = pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC || (pBridgeDesc->HasStructuralLongitudinalJoints() ? pMaterials->GetLongitudinalJointConcreteType() == pgsTypes::PCI_UHPC : false);
   bool bUHPC = pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC || (pBridgeDesc->HasStructuralLongitudinalJoints() ? pMaterials->GetLongitudinalJointConcreteType() == pgsTypes::UHPC : false);
   bool bUnconfinedConcrete = !IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)) || (pBridgeDesc->HasStructuralLongitudinalJoints() ? !IsUHPC(pMaterials->GetLongitudinalJointConcreteType()) : false);

   /////////////////////////////////////
   INIT_UV_PROTOTYPE( rptAreaUnitValue,   area,   pDisplayUnits->GetAreaUnit(),         false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, cg,     pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(),       false);
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(),       true);
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,  pDisplayUnits->GetGeneralForceUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dist,   pDisplayUnits->GetComponentDimUnit(), true );
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true );

   rptRcScalar strain;
   strain.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);
   strain.SetWidth(9);
   strain.SetPrecision(4);

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
   (*pPara) << _T("Depth to Extreme Layer of Tension Reinforcement, ") << Sub2(_T("d"), _T("t")) << _T(" = ") << dist.SetValue(pmcd->dt) << rptNewLine;
   (*pPara) << Sub2(symbol(epsilon), _T("t")) << _T(" x 1000 = ") << strain.SetValue(pmcd->et * 1000) << rptNewLine;
   (*pPara) << symbol(phi) << _T(" =") << scalar.SetValue(pmcd->Phi) << rptNewLine;
   (*pPara) << _T("Nominal Capacity, ") << Sub2(_T("M"), _T("n")) << _T(" = ") << moment.SetValue(pmcd->Mn) << rptNewLine;
   (*pPara) << _T("Nominal Resistance, ") << Sub2(_T("M"),_T("r")) << _T(" = ") << symbol(phi) << Sub2(_T("M"), _T("n")) << _T(" = ") << moment.SetValue(pmcd->Mr) << rptNewLine;
   (*pPara) << _T("Moment Arm = ") << Sub2(_T("d"),_T("e")) << _T(" - ") << Sub2(_T("d"),_T("c")) << _T(" = ") << Sub2(_T("M"),_T("n")) << _T("/T = ") << dist.SetValue(pmcd->MomentArm) << rptNewLine;

   std::array<std::_tstring, 4> strControl
   { _T("concrete crushing at extreme compression fiber"),
     _T("girder concrete crushing"),
     _T("girder concrete crack localization"),
     _T("tension strain limit of reinforcement") 
   };
   
   (*pPara) << Bold(_T("Moment capacity controlled by ") << strControl[+pmcd->Controlling]) << rptNewLine;
   if (pmcd->bDevelopmentLengthReducedStress)
   {
      (*pPara) << _T("Strand stresses are reduced due to lack of full development per LRFD 5.9.4.3.2") << rptNewLine;
   }
   if (!bUHPC && !bConsiderReinforcementStrainLimits)
   {
      (*pPara) << _T("Reinforcement strain exceeds minimum elongation per the material specification") << rptNewLine;
   }
   (*pPara) << rptNewLine;

   // if this is a zero capacity section, just return since there is nothing else to show
   if ( IsZero(pmcd->Mn) )
   {
      return pChapter;
   }

   if(pmcd->ConcreteCrushingSolution)
      ReportSolution(pBroker, _T("Concrete crushing"), pChapter, pmcd->girderShapeIndex,pmcd->deckShapeIndex,pmcd->Section, pmcd->ConcreteCrushingSolution, bPositiveMoment, pDisplayUnits);

   if (pmcd->UHPCGirderCrushingSolution)
      ReportSolution(pBroker, _T("Girder UHPC crushing"), pChapter, pmcd->girderShapeIndex, pmcd->deckShapeIndex, pmcd->Section, pmcd->UHPCGirderCrushingSolution, bPositiveMoment, pDisplayUnits);

   if (pmcd->UHPCCrackLocalizationSolution)
      ReportSolution(pBroker, _T("Girder UHPC crack localization"), pChapter, pmcd->girderShapeIndex, pmcd->deckShapeIndex, pmcd->Section, pmcd->UHPCCrackLocalizationSolution, bPositiveMoment, pDisplayUnits);

   if (pmcd->ReinforcementFractureSolution)
      ReportSolution(pBroker, _T("Reinforcement tension strain limit"), pChapter, pmcd->girderShapeIndex, pmcd->deckShapeIndex, pmcd->Section, pmcd->ReinforcementFractureSolution, bPositiveMoment, pDisplayUnits);

   if (pmcd->ReinforcementStressLimitStateSolution)
      ReportSolution(pBroker, _T("Capacity at reinforcement stress limit state"), pChapter, pmcd->girderShapeIndex, pmcd->deckShapeIndex, pmcd->Section, pmcd->ReinforcementStressLimitStateSolution, bPositiveMoment, pDisplayUnits);

   std::_tstring strImagePath = rptStyleManager::GetImagePath();

   if (bPCIUHPC)
   {
      *pPara << Bold(_T("PCI UHPC Concrete Stress-Strain Model (PCI UHPC SDG E.6.1)")) << rptNewLine;
      *pPara << rptRcImage(strImagePath + _T("PCI_UHPC_Concrete.png")) << rptNewLine;
   }

   if (bUHPC)
   {
      *pPara << Bold(_T("UHPC Concrete Stress-Strain Model (GS 1.4.2.4.3 & 1.4.2.5.4)")) << rptNewLine;
      *pPara << rptRcImage(strImagePath + _T("UHPC_Concrete.png")) << rptNewLine;
   }

   if (bUnconfinedConcrete)
   {
      *pPara << Bold(_T("Unconfined Concrete Stress-Strain Model")) << rptNewLine;
      *pPara << rptRcImage(strImagePath + _T("UnconfinedConcrete.png")) << rptNewLine;
   }

   *pPara << Bold(_T("Prestressing Strand Model")) << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("PowerFormula.png")) << rptNewLine;

   *pPara << Bold(_T("Reinforcement Model")) << rptNewLine;
   *pPara << rptRcImage(strImagePath + _T("Rebar.png")) << rptNewLine;

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CMomentCapacityChapterBuilder::Clone() const
{
   return std::make_unique<CMomentCapacityChapterBuilder>();
}

rptRcImage* CMomentCapacityChapterBuilder::CreateImage(IndexType girderShapeIndex, IndexType deckShapeIndex, CComPtr<IGeneralSection> section, CComPtr<IMomentCapacitySolution> solution,bool bPositiveMoment, IEAFDisplayUnits* pDisplayUnits) const
{
   CImage image;
   DrawSection(image,girderShapeIndex,deckShapeIndex,section,solution,bPositiveMoment,pDisplayUnits);

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

void CMomentCapacityChapterBuilder::DrawSection(CImage& image, IndexType girderShapeIndex,IndexType deckShapeIndex,CComPtr<IGeneralSection> section, CComPtr<IMomentCapacitySolution> solution,bool bPositiveMoment,IEAFDisplayUnits* pDisplayUnits) const
{
   CComPtr<IGeneralSectionSolution> general_solution;
   solution->get_GeneralSectionSolution(&general_solution);
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

   pDC->SetTextAlign(TA_LEFT | TA_BOTTOM);
   CFont font;
   font.CreatePointFont(80, _T("Arial"), pDC);
   CFont* old_font = pDC->SelectObject(&font);
   pDC->SetBkMode(TRANSPARENT);

   // draw each slice
   CPen girderPen(PS_SOLID,1, WHITESMOKE);
   CBrush girderBrush(SEGMENT_FILL_COLOR);
   CBrush voidBrush(VOID_COLOR);
   CPen tensionPen(PS_SOLID, 1, TENSION_BORDER_COLOR);
   CBrush tensionBrush(TENSION_FILL_COLOR);
   CPen compressionPen(PS_SOLID, 1, COMPRESSION_BORDER_COLOR);
   CBrush compressionBrush(COMPRESSION_FILL_COLOR);

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

   // draw neutral slices first
   pOldBrush = pDC->SelectObject(&girderBrush);
   pOldPen = pDC->SelectObject(&girderPen);
   for ( auto sliceIdx : neutralIndices)
   {
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw compression slices
   pDC->SelectObject(&compressionBrush);
   pDC->SelectObject(&compressionPen);
   for ( auto sliceIdx : compressionIndices )
   {
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw the voids on top of the foreground shape
   pDC->SelectObject(&girderPen);
   pDC->SelectObject(&voidBrush);
   for ( auto sliceIdx : voidIndices)
   {
      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx,&slice);

      CComPtr<IShape> shape;
      slice->get_Shape(&shape);

      DrawSlice(shape,pDC,mapper);
   }

   // draw tension slices
   pDC->SelectObject(&tensionBrush);
   pDC->SelectObject(&tensionPen);
   for ( auto sliceIdx : tensionIndices )
   {
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

   // draw strain profiles
   CPoint p;

   CComPtr<IPlane3d> incremental_strain_plane;
   solution->get_IncrementalStrainPlane(&incremental_strain_plane);

   Float64 Ytg{ 0 }, Ybg{ 0 }, Ytd{ 0 }, Ybd{ 0 };
   Float64 etg{ 0 }, ebg{ 0 }, etd{ 0 }, ebd{ 0 };

   if (deckShapeIndex != INVALID_INDEX)
   {
      // strain diagram for deck
      CComPtr<IShape> deck_shape;
      section->get_Shape(deckShapeIndex, &deck_shape);
      CComPtr<IRect2d> bbDeck;
      deck_shape->get_BoundingBox(&bbDeck);
      CComPtr<IPlane3d> deck_initial_strain;
      section->get_InitialStrain(deckShapeIndex, &deck_initial_strain);

      bbDeck->get_Top(&Ytd);
      bbDeck->get_Bottom(&Ybd);

      deck_initial_strain->GetZ(0, Ytd, &etd);
      deck_initial_strain->GetZ(0, Ybd, &ebd);

      Float64 eTop, eBottom; // strain top and bottom
      incremental_strain_plane->GetZ(0, Ytd, &eTop);
      incremental_strain_plane->GetZ(0, Ybd, &eBottom);

      etd += eTop;
      ebd += eBottom;
   }

   if (girderShapeIndex != INVALID_INDEX)
   {
      // strain diagram for girder
      CComPtr<IShape> girder_shape;
      section->get_Shape(girderShapeIndex, &girder_shape);
      CComPtr<IRect2d> bbGirder;
      girder_shape->get_BoundingBox(&bbGirder);
      CComPtr<IPlane3d> girder_initial_strain;
      section->get_InitialStrain(girderShapeIndex, &girder_initial_strain);

      bbGirder->get_Top(&Ytg);
      bbGirder->get_Bottom(&Ybg);

      girder_initial_strain->GetZ(0, Ytg, &etg);
      girder_initial_strain->GetZ(0, Ybg, &ebg);

      Float64 eTop, eBottom; // strain top and bottom
      incremental_strain_plane->GetZ(0, Ytg, &eTop);
      incremental_strain_plane->GetZ(0, Ybg, &eBottom);

      etg += eTop;
      ebg += eBottom;
   }


   // scale strains so that they plot with the same
   // aspect ratio as the section
   Float64 strain = Max(fabs(etd), fabs(ebd), fabs(etg),fabs(ebg));
   Float64 scale = (wx / 4) / strain;
   etg *= scale;
   ebg *= scale;
   etd *= scale;
   ebd *= scale;
   strain *= scale;

   mapper.SetDeviceOrg(3 * image.GetWidth() / 4 + 2, 2);

   // negate the mirror factor so the strain plane draws correctly
   mapper.GetWorldExt(&wx, &wy);
   wx *= mirror_factor;
   mapper.SetWorldExt(wx, wy);

   if (deckShapeIndex != INVALID_INDEX)
   {
      mapper.WPtoDP(0, mirror_factor * Ytd, &p.x, &p.y);
      pDC->MoveTo(p);
      mapper.WPtoDP(etd, mirror_factor * Ytd, &p.x, &p.y);
      pDC->LineTo(p);

      CString str_etd;
      str_etd.Format(_T("%f"), etd / scale);
      pDC->SetTextAlign(TA_LEFT | TA_TOP);
      pDC->TextOut(p.x, p.y, str_etd);

      mapper.WPtoDP(ebd, mirror_factor * Ybd, &p.x, &p.y);
      pDC->LineTo(p);

      CString str_ebd;
      str_ebd.Format(_T("%f"), ebd / scale);
      pDC->SetTextAlign(TA_LEFT | TA_BOTTOM);
      pDC->TextOut(p.x, p.y, str_ebd);

      mapper.WPtoDP(0, mirror_factor * Ybd, &p.x, &p.y);
      pDC->LineTo(p);
      mapper.WPtoDP(0, mirror_factor * Ytd, &p.x, &p.y);
      pDC->LineTo(p);
   }

   if (girderShapeIndex != INVALID_INDEX)
   {
      mapper.WPtoDP(0, mirror_factor * Ytg, &p.x, &p.y);
      pDC->MoveTo(p);
      mapper.WPtoDP(etg, mirror_factor * Ytg, &p.x, &p.y);
      pDC->LineTo(p);
      
      CString str_etg;
      str_etg.Format(_T("%f"), etg / scale);
      pDC->SetTextAlign(TA_LEFT | TA_TOP);
      pDC->TextOut(p.x, p.y, str_etg);

      mapper.WPtoDP(ebg, mirror_factor * Ybg, &p.x, &p.y);
      pDC->LineTo(p);

      CString str_ebg;
      str_ebg.Format(_T("%f"), ebg / scale);
      pDC->SetTextAlign(TA_LEFT | TA_BOTTOM);
      pDC->TextOut(p.x, p.y, str_ebg);

      mapper.WPtoDP(0, mirror_factor * Ybg, &p.x, &p.y);
      pDC->LineTo(p);
      mapper.WPtoDP(0, mirror_factor * Ytg, &p.x, &p.y);
      pDC->LineTo(p);
   }

   // Draw the compression resultant
   CPen cPen(PS_SOLID,5,COMPRESSION_FILL_COLOR);
   pDC->SelectObject(&cPen);

   CComPtr<IPoint2d> pntC;
   solution->get_CompressionResultantLocation(&pntC);
   Float64 y;
   pntC->get_Y(&y);

   y *= mirror_factor;

   mapper.WPtoDP(strain,y,&p.x,&p.y); 
   pDC->MoveTo(p);   

   mapper.WPtoDP(0,y,&p.x,&p.y);
   pDC->LineTo(p);

   // arrow head
   pDC->MoveTo(p.x+5,p.y-5);
   pDC->LineTo(p);
   pDC->MoveTo(p.x+5,p.y+5);
   pDC->LineTo(p);

   Float64 C;
   solution->get_CompressionResultant(&C);
   CString strC;
   strC.Format(_T("%s"), ::FormatDimension(-C, pDisplayUnits->GetGeneralForceUnit()));
   pDC->SetTextAlign(TA_CENTER | TA_BOTTOM);
   mapper.WPtoDP(strain/2, y, &p.x, &p.y);
   pDC->TextOut(p.x, p.y, strC);

   // Draw the tension resultant
   CPen tPen(PS_SOLID,5,TENSION_FILL_COLOR);
   pDC->SelectObject(&tPen);

   CComPtr<IPoint2d> pntT;
   solution->get_TensionResultantLocation(&pntT);
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

   Float64 T;
   solution->get_TensionResultant(&T);
   CString strT;
   strT.Format(_T("%s"), ::FormatDimension(T, pDisplayUnits->GetGeneralForceUnit()));
   pDC->SetTextAlign(TA_CENTER | TA_BOTTOM);
   mapper.WPtoDP(strain / 2, y, &p.x, &p.y);
   pDC->TextOut(p.x, p.y, strT);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(old_font);

   image.ReleaseDC();
}

void CMomentCapacityChapterBuilder::DrawSlice(IShape* pShape,CDC* pDC, WBFL::Graphing::PointMapper& mapper) const
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

void CMomentCapacityChapterBuilder::ReportSolution(IBroker* pBroker,const TCHAR* strTitle,rptChapter* pChapter, IndexType girderShapeIndex, IndexType deckShapeIndex, CComPtr<IGeneralSection> section,CComPtr<IMomentCapacitySolution> solution,bool bPositiveMoment,IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_UV_PROTOTYPE(rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, cg, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), true);
   INIT_UV_PROTOTYPE(rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dist, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature, pDisplayUnits->GetCurvatureUnit(), true);

   rptRcScalar strain;
   strain.SetFormat(WBFL::System::NumericFormatTool::Format::Automatic);
   strain.SetWidth(7);
   strain.SetPrecision(4);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   auto pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   (*pChapter) << pPara;
   pPara->SetName(strTitle);
   (*pPara) << strTitle << rptNewLine;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   Float64 Fz(0.0), Mx(0.0), My(0.0), C(0.0), T(0.0);
   CComPtr<IPoint2d> cgC, cgT;
   Float64 c, dc, de, moment_arm, k;
   solution->get_Fz(&Fz);
   solution->get_Mx(&Mx);
   solution->get_My(&My);
   solution->get_CompressionResultant(&C);
   solution->get_TensionResultant(&T);
   solution->get_CompressionResultantLocation(&cgC);
   solution->get_TensionResultantLocation(&cgT);
   solution->get_DepthToNeutralAxis(&c);
   solution->get_DepthToCompressionResultant(&dc);
   solution->get_DepthToTensionResultant(&de);
   solution->get_MomentArm(&moment_arm);
   solution->get_Curvature(&k);

   Float64 Mn = -Mx;
   ATLASSERT(IsEqual(moment_arm, fabs(Mn) / T));

   (*pPara) << _T("Depth to neutral axis, c = ") << dist.SetValue(c) << rptNewLine;
   (*pPara) << _T("Compression Resultant, C = ") << force.SetValue(C) << rptNewLine;
   (*pPara) << _T("Depth to Compression Resultant, ") << Sub2(_T("d"), _T("c")) << _T(" = ") << dist.SetValue(dc) << rptNewLine;
   (*pPara) << _T("Tension Resultant, T = ") << force.SetValue(T) << rptNewLine;
   (*pPara) << _T("Depth to Tension Resultant, ") << Sub2(_T("d"), _T("e")) << _T(" = ") << dist.SetValue(de) << rptNewLine;
   (*pPara) << _T("Nominal Capacity, ") << Sub2(_T("M"), _T("n")) << _T(" = ") << moment.SetValue(Mn) << rptNewLine;
   (*pPara) << _T("Moment Arm = ") << Sub2(_T("M"), _T("n")) << _T("/T = ") << dist.SetValue(moment_arm) << rptNewLine;
   (*pPara) << _T("Curvature = ") << curvature.SetValue(k) << rptNewLine;


   // Image
   *pPara << CreateImage(girderShapeIndex,deckShapeIndex, section, solution, bPositiveMoment, pDisplayUnits);

   // Table
   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(16, _T(""));
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
   (*pTable)(0, col++) << _T("Initial") << rptNewLine << _T("Strain") << rptNewLine << _T("at ") << Sub2(_T("Y"),_T("cg"));
   (*pTable)(0, col++) << _T("Incremental") << rptNewLine << _T("Strain") << rptNewLine << _T("at ") << Sub2(_T("Y"), _T("cg"));
   (*pTable)(0, col++) << _T("Total") << rptNewLine << _T("Strain") << rptNewLine << _T("at ") << Sub2(_T("Y"), _T("cg"));
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
   solution->get_GeneralSectionSolution(&general_solution);

   RowIndexType row = pTable->GetNumberOfHeaderRows();
   Float64 sum_force = 0;
   Float64 sum_moment = 0;
   CollectionIndexType nSlices;
   general_solution->get_SliceCount(&nSlices);
   for (CollectionIndexType sliceIdx = 0; sliceIdx < nSlices; sliceIdx++)
   {
      col = 0;

      CComPtr<IGeneralSectionSlice> slice;
      general_solution->get_Slice(sliceIdx, &slice);

      IndexType shapeIdx;
      slice->get_ShapeIndex(&shapeIdx);
      CComBSTR bstrName;
      section->get_Name(shapeIdx, &bstrName);

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
      solution->get_NeutralAxis(&naLine);
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

      Float64 fgStress, bgStress, netStress;
      slice->get_ForegroundStress(&fgStress);
      slice->get_BackgroundStress(&bgStress);


      ATLASSERT(IsEqual(f, fgStress));

      netStress = fgStress - bgStress;

      Float64 F = slice_area * netStress;
      Float64 M = F * cgY;

      Float64 fpe = 0.0;
      Float64 fpx_fps = 0.0;
      CComQIPtr<IPowerFormula> pf(ss);
      if (pf)
      {
         // this assumes reinforcement is always modeled with the power formula 
         // that might not aways be the case (stainless steel, frp, etc strands???)
         pf->get_ReductionFactor(&fpx_fps);
         fpe = E * initial_strain;
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
         (*pTable)(row, col++) << color(Red) << total_strain << _T(" (") << (total_strain < 0 ? emin : emax) << _T(")") << color(Black);
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
      (*pTable)(row, col++) << stress.SetValue(fgStress);
      (*pTable)(row, col++) << stress.SetValue(bgStress);
      (*pTable)(row, col++) << stress.SetValue(netStress);
      (*pTable)(row, col++) << force.SetValue(F);
      (*pTable)(row, col++) << moment.SetValue(M);

      sum_force += F;
      sum_moment += M;

      row++;
   }

   force.ShowUnitTag(true);
   moment.ShowUnitTag(true);

   *pPara << _T("Total strain in ") << color(Red) << _T("red") << color(Black) << _T(" text indicates the strain exceeds the usable strain limit of the material. The usable strain limit is show in parentheses.") << rptNewLine;

   *pPara << _T("Resultant Force  = ") << symbol(SUM) << _T("(") << symbol(delta) << _T("F) = ") << force.SetValue(sum_force) << rptNewLine;
   *pPara << _T("Resultant Moment = ") << symbol(SUM) << _T("(") << symbol(delta) << _T("M) = ") << moment.SetValue(sum_moment) << rptNewLine;

   *pPara << rptNewLine;
   *pPara << _T("Foreground (Fg) stress = stress in section materials (including voided areas)") << rptNewLine;
   *pPara << _T("Background (Bg) stress = stress in voids that is subtracted from voided area") << rptNewLine;
   *pPara << rptNewLine;
}