// PGSuperExporter.cpp : Implementation of CPGSuperExporter
#include "stdafx.h"
#include "KDOTExport_i.h"
#include "PGSuperDataExporter.h"
#include "ExportDlg.h"

#include <MFCTools\Prompts.h>
#include <MFCTools\VersionInfo.h>

#include "PGSuperInterfaces.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <pgsExt\DeckDescription.h>


   enum Type { A615  = 0x1000,  // ASTM A615
               A706  = 0x2000,  // ASTM A706
               A1035 = 0x4000   // ASTM A1035
   };
static std::_tstring GenerateReinfTypeName(matRebar::Type rtype)
{
   switch(rtype)
   {
   case matRebar::A615:
      return _T("A615");
      break;
   case matRebar::A706:
      return _T("A706");
      break;
   case matRebar::A1035:
      return _T("A1035");
      break;
   default:
      ATLASSERT(0); // new rebar grade?
      return _T("A615");
   }
}

static std::_tstring GenerateReinfGradeName(matRebar::Grade grade)
{
   switch(grade)
   {
   case matRebar::Grade40:
      return _T("40");
      break;
   case matRebar::Grade60:
      return _T("60");
      break;
   case matRebar::Grade75:
      return _T("75");
      break;
   case matRebar::Grade80:
      return _T("80");
      break;
   default:
      ATLASSERT(0); // new rebar grade?
      return _T("40");
   }
}

HRESULT CPGSuperDataExporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperDataExporter

STDMETHODIMP CPGSuperDataExporter::Init(UINT nCmdID)
{

   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetMenuText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("KDOT CAD Data...");
   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetBitmapHandle(HBITMAP* phBmp)
{
   *phBmp = m_bmpLogo;
   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetCommandHintText(BSTR*  bstrText)
{
   *bstrText = CComBSTR("Export PGSuper data to KDOT CAD XML format\nExport PGSuper data to KDOT CAD format");
   return S_OK;   
}

STDMETHODIMP CPGSuperDataExporter::Export(IBroker* pBroker)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE2(pBroker,ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
      girderKey.girderIndex = 0;
   }
   else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
   {
      girderKey.groupIndex  = selection.GroupIdx;
      girderKey.girderIndex = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex  = 0;
      girderKey.girderIndex = 0;
   }

   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

   CExportDlg  caddlg (pBroker, NULL);
   caddlg.m_GirderKeys = girderKeys;

   // Open the ExportCADData dialog box
   INT_PTR stf = caddlg.DoModal();
   if (stf == IDOK)
   {
	   // Get user's span & beam id values
	   girderKeys = caddlg.m_GirderKeys;
   }
   else
   {
	   // Just do nothing if CANCEL
       return S_OK;
   }

   // use the PGSuper document file name, with the extension changed, as the default file name
   CEAFDocument* pDoc = EAFGetDocument();
   CString strDefaultFileName = pDoc->GetPathName();
   // default name is file name with .xml
   strDefaultFileName.Replace(_T(".pgs"),_T(".xml"));

   CFileDialog dlgFile(FALSE,_T("*.xml"),strDefaultFileName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,_T("KDOT CAD Export(*.xml)|*.xml"));
   if ( dlgFile.DoModal() == IDCANCEL )
   {
      return S_OK;
   }

   CString strFileName = dlgFile.GetPathName();

   HRESULT hr = Export(pBroker,strFileName,girderKeys);

   if ( SUCCEEDED(hr) )
   {
      CString strMsgSuccess;
      strMsgSuccess.Format(_T("Data for was successfully exported to the File \n\n %s"),strFileName);
      AfxMessageBox(strMsgSuccess,MB_OK | MB_ICONINFORMATION);
   }

   return S_OK;
}

HRESULT CPGSuperDataExporter::Export(IBroker* pBroker,CString& strFileName, const std::vector<CGirderKey>& girderKeys)
{
   USES_CONVERSION;

   { // scope the progress window
   GET_IFACE2(pBroker,IProgress,pProgress);
   CEAFAutoProgress autoProgress(pProgress,0);
   pProgress->UpdateMessage(_T("Exporting PGSuper data for KDOT CAD"));

   try
   {
      KDOT::KDOTExport kdot_export;

      GET_IFACE2(pBroker,IBridge,pBridge);
      GET_IFACE2(pBroker,IBridgeDescription,pBridgeDescription);
      GET_IFACE2(pBroker, IGirder,pGirder);
      GET_IFACE2(pBroker, IRoadway,pAlignment);

      const CBridgeDescription2* pBridgeDescr2 = pBridgeDescription->GetBridgeDescription();

      // Bridge level data
      KDOT::BridgeDataType brdata;

      std::_tstring type = pBridgeDescr2->GetLeftRailingSystem()->GetExteriorRailing()->GetName();
      brdata.LeftRailingType(T2A(type.c_str()));

      type = pBridgeDescr2->GetRightRailingSystem()->GetExteriorRailing()->GetName();
      brdata.RightRailingType(T2A(type.c_str()));

      GET_IFACE2(pBroker,IMaterials,pMaterials);
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
      IntervalIndexType finalIntervalIdx = pIntervals->GetLiveLoadInterval();

      Float64 dval = pMaterials->GetDeckFc(compositeDeckIntervalIdx);
      dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
      brdata.SlabFc(dval);

      // Assume uniform deck thickness
      const CDeckDescription2* pDeckDescription = pBridgeDescr2->GetDeckDescription();

      Float64 tDeck(0.0);
      if ( pDeckDescription->DeckType == pgsTypes::sdtCompositeSIP )
      {
         tDeck = pDeckDescription->GrossDepth + pDeckDescription->PanelDepth;
      }
      else
      {
         tDeck = pDeckDescription->GrossDepth;
      }

      dval = ::ConvertFromSysUnits(tDeck, unitMeasure::Inch);
      brdata.SlabThickness(dval);

      dval = pDeckDescription->OverhangEdgeDepth;
      dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
      brdata.OverhangThickness(dval);

      SpanIndexType ns = pBridge->GetSpanCount();
      brdata.NumberOfSpans(ns);

      KDOT::BridgeDataType::SpanLengths_sequence span_lengths;
      KDOT::BridgeDataType::NumberOfGirdersPerSpan_sequence ngs_count;
      for (SpanIndexType is=0; is<ns; is++)
      {
         Float64 sl = pBridge->GetSpanLength(is);
         sl = ::ConvertFromSysUnits(sl, unitMeasure::Inch); 
         span_lengths.push_back(sl);

         GirderIndexType ng = pBridge->GetGirderCountBySpan(is);
         ngs_count.push_back(ng);
      }

      brdata.SpanLengths(span_lengths);
      brdata.NumberOfGirdersPerSpan(ngs_count);

      // Pier data
      KDOT::BridgeDataType::PierData_sequence pds;

      for (SpanIndexType ip=0; ip<ns+1; ip++)
      {
         KDOT::BridgeDataType::PierData_type pd;

         dval = pBridge->GetPierStation(ip);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         pd.Station(dval);

         CComPtr<IAngle> angle;
         pBridge->GetPierSkew(ip, &angle);
         angle->get_Value(&dval);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Radian);
         pd.Skew(dval);

         Float64 backEndOffset(0.0);
         if (ip>0)
         {
            CSegmentKey backSegmentKey(ip-1, 0, 0);
            backEndOffset  = pBridge->GetSegmentEndBearingOffset(backSegmentKey) - pBridge->GetSegmentEndEndDistance(backSegmentKey);
         }

         dval = ::ConvertFromSysUnits(backEndOffset, unitMeasure::Inch);
         pd.backGirderEndOffset(dval);

         Float64 aheadEndOffset(0.0);
         if (ip<ns)
         {
            CSegmentKey aheadSegmentKey(ip, 0, 0);
            aheadEndOffset = pBridge->GetSegmentStartBearingOffset(aheadSegmentKey) - pBridge->GetSegmentStartEndDistance(aheadSegmentKey);
         }

         dval = ::ConvertFromSysUnits(aheadEndOffset, unitMeasure::Inch);
         pd.aheadGirderEndOffset(dval);

         pds.push_back(pd);
      }

      brdata.PierData(pds);

      GET_IFACE2(pBroker,ICamber,pCamber);
      GET_IFACE2(pBroker, ISegmentLifting, pSegmentLifting);
      GET_IFACE2(pBroker, ISegmentHauling, pSegmentHauling);
      GET_IFACE2(pBroker,ISectionProperties,pSectProp);
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      GET_IFACE2(pBroker,ILongitudinalRebar,pLongRebar);
      GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,IProductForces,pProduct);
      GET_IFACE2(pBroker,IPretensionForce,pPretensionForce);

      pgsTypes::BridgeAnalysisType bat = pProduct->GetBridgeAnalysisType(pgsTypes::Minimize);

      // Girder Data
      Float64 bridgeHaunchVolume = 0.0; // will add haunches from all girders to compute this
      KDOT::BridgeDataType::GirderData_sequence gds;
      for( std::vector<CGirderKey>::const_iterator itg = girderKeys.begin(); itg!=girderKeys.end(); itg++)
      {
         SpanIndexType is = itg->groupIndex;
         PierIndexType prevPierIdx = is;
         PierIndexType nextPierIdx = is+1;

         GirderIndexType ng = pBridge->GetGirderCountBySpan(is);

         GirderIndexType ig = itg->girderIndex;
         ATLASSERT(pBridge->GetSegmentCount(is,ig)==1); // new capability in pgsuper?

         KDOT::BridgeDataType::GirderData_type gd;

         ::KDOT::GirderKeyType gkey;
         gkey.SpanIndex(is);
         gkey.GirderIndex(ig);
         gd.GirderKey(gkey);

         CSegmentKey segmentKey(is, ig, 0);
         CGirderKey girderKey(*itg);

         // have to dig into library to get information
         const CGirderGroupData* pGroup = pBridgeDescr2->GetGirderGroup(segmentKey.groupIndex);
         const GirderLibraryEntry* pGirderEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
         std::_tstring strGirderName = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();
         ATLASSERT( strGirderName == pGirderEntry->GetName() );

         gd.GirderType(T2A(strGirderName.c_str()));

         KDOT::BridgeDataType::GirderData_type::SectionDimensions_sequence sds;

         // section dimensions
         const GirderLibraryEntry::Dimensions& dims = pGirderEntry->GetDimensions();
         for(GirderLibraryEntry::Dimensions::const_iterator itd=dims.begin(); itd!=dims.end(); itd++)
         {
            const GirderLibraryEntry::Dimension& dim = *itd;

            KDOT::BridgeDataType::GirderData_type::SectionDimensions_type sd;

            sd.ParameterName(T2A(dim.first.c_str()));

            // assumes are dimensions are reals. This may not be the case (e.g., voided slab has #voids)
            dval = ::ConvertFromSysUnits(dim.second, unitMeasure::Inch);
            sd.Value(dval);

            sds.push_back(sd);
         }

         gd.SectionDimensions(sds);

         // Girder material
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

         dval = pMaterials->GetSegmentFc(segmentKey, releaseIntervalIdx);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Fci(dval);

         dval = pMaterials->GetSegmentFc(segmentKey, finalIntervalIdx);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Fc(dval);

         dval = pMaterials->GetSegmentEc(segmentKey, releaseIntervalIdx);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Eci(dval);

         dval = pMaterials->GetSegmentEc(segmentKey, finalIntervalIdx);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KSI);
         gd.Ec(dval);

         // Girder length 
         Float64 girderLength = pBridge->GetGirderLength(girderKey);
         Float64 girderSpanLength = pBridge->GetGirderSpanLength(girderKey);

         dval = ::ConvertFromSysUnits(girderLength, unitMeasure::Inch);
         gd.GirderLength(dval);

         // Station and offset of girder line ends
         CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
         pGirder->GetSegmentEndPoints(segmentKey,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

         Float64 startStation, startOffset, endStation, endOffset;
         pAlignment->GetStationAndOffset(pntPier1,&startStation,&startOffset);
         pAlignment->GetStationAndOffset(pntPier2,&endStation,  &endOffset);

         Float64 startPierStation = pBridge->GetPierStation(0);
         Float64 startBrDistance = startStation - startPierStation;
         Float64 endBrDistance   = endStation   - startPierStation;

         // Girder spacing
         std::vector<Float64> prevPierSpac = pBridge->GetGirderSpacing(prevPierIdx, pgsTypes::Ahead, pgsTypes::AtPierLine,    pgsTypes::NormalToItem);
         std::vector<Float64> nextPierSpac = pBridge->GetGirderSpacing(nextPierIdx, pgsTypes::Back,  pgsTypes::AtPierLine,    pgsTypes::NormalToItem);

         // left start
         if (ig==0)
         {
            dval = pBridge->GetLeftSlabOverhang(startBrDistance);
         }
         else
         {
            dval = prevPierSpac[ig-1];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingLeftStart(dval);

         // Right start
         if (ig==ng-1)
         {
            dval = pBridge->GetRightSlabOverhang(startBrDistance);
         }
         else
         {
            dval = prevPierSpac[ig];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingRightStart(dval);

         // left end
         if (ig==0)
         {
            dval = pBridge->GetLeftSlabOverhang(endBrDistance);
         }
         else
         {
            dval = nextPierSpac[ig-1];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingLeftEnd(dval);

         // Right end
         if (ig==ng-1)
         {
            dval = pBridge->GetRightSlabOverhang(endBrDistance);
         }
         else
         {
            dval = nextPierSpac[ig];
         }
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.SpacingRightEnd(dval);

         // Harping points
         KDOT::GirderDataType::HarpingPoints_sequence hpseq;
         IndexType nhp = pStrandGeom->GetNumHarpPoints(segmentKey);
         gd.NumberOfHarpingPoints(nhp);

         Float64 holddownforce = pPretensionForce->GetHoldDownForce(segmentKey);

         for (IndexType ihp=0; ihp<nhp; ihp++)
         {
            KDOT::HarpingPointDataType hpdata;
            Float64 lhp, rhp;
            pStrandGeom->GetHarpingPointLocations(segmentKey,&lhp,&rhp);

            dval = ::ConvertFromSysUnits(ihp==0 ? lhp:rhp, unitMeasure::Inch);
            hpdata.Location(dval);

            dval = ::ConvertFromSysUnits(holddownforce/nhp, unitMeasure::Kip);
            hpdata.HoldDownForce(dval);

            hpseq.push_back(hpdata);
         }

         gd.HarpingPoints(hpseq);



         // Lifting location
         dval = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.LiftingLocation(dval);

         // Hauling locations
         dval = pSegmentHauling->GetLeadingOverhang(segmentKey);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.LeadingHaulingLocation(dval);

         dval = pSegmentHauling->GetTrailingOverhang(segmentKey);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);

         gd.TrailingHaulingLocation(dval);

         // A dimensions
         Float64 adstart, adend;
         pBridge->GetSlabOffset(segmentKey, &adstart, &adend);

         gd.StartADimension(adstart);
         gd.EndADimension(adend);

         // Section properties
         bool bIsPrismatic_Final  = pGirder->IsPrismatic(finalIntervalIdx, segmentKey);
         gd.IsPrismatic(bIsPrismatic_Final);

         // Take sample at mid-span
         std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L));
         ATLASSERT(vPoi.size() == 1);
         pgsPointOfInterest mid_poi(vPoi.front());

         pgsPointOfInterest end_poi(segmentKey,0.0); // Left end of girder

          // Non-composite properties
         dval = pSectProp->GetAg(releaseIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch2);
         gd.Area(dval);

         dval = pSectProp->GetIx(releaseIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Ix(dval);

         dval = pSectProp->GetIy(releaseIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Iy(dval);

         Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(Hg, unitMeasure::Inch);
         gd.d(dval);

         dval = pSectProp->GetY(releaseIntervalIdx,mid_poi,pgsTypes::TopGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yt(dval);

         dval = pSectProp->GetY(releaseIntervalIdx,mid_poi,pgsTypes::BottomGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yb(dval);

         dval = pSectProp->GetS(releaseIntervalIdx,mid_poi,pgsTypes::TopGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.St(dval);

         dval = pSectProp->GetS(releaseIntervalIdx,mid_poi,pgsTypes::BottomGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.Sb(dval);

         dval = pSectProp->GetPerimeter(mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.P(dval);

         dval = pSectProp->GetSegmentWeightPerLength(segmentKey);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::KipPerInch);
         gd.W(dval);

         dval = pSectProp->GetSegmentWeight(segmentKey);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Kip);
         gd.Wtotal(dval);

          // Composite properties
         dval = pSectProp->GetAg(finalIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch2);
         gd.Area_c(dval);

         dval = pSectProp->GetIx(finalIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Ix_c(dval);

         dval = pSectProp->GetIy(finalIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch4);
         gd.Iy_c(dval);

         dval = pSectProp->GetHg(finalIntervalIdx,mid_poi);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.d_c(dval);

         dval = pSectProp->GetY(finalIntervalIdx,mid_poi,pgsTypes::TopGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yt_c(dval);

         dval = pSectProp->GetY(finalIntervalIdx,mid_poi,pgsTypes::BottomGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.Yb_c(dval);

         dval = pSectProp->GetS(finalIntervalIdx,mid_poi,pgsTypes::TopGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.St_c(dval);

         dval = pSectProp->GetS(finalIntervalIdx,mid_poi,pgsTypes::BottomGirder);
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch3);
         gd.Sb_c(dval);

         // Strand eccentricities
         Float64 nEff;
         dval = pStrandGeom->GetEccentricity( pgsTypes::sptGrossNoncomposite, releaseIntervalIdx, end_poi, false /*no temporary strands*/, &nEff );
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.StrandEccentricityAtEnds(dval);

         dval = pStrandGeom->GetEccentricity( pgsTypes::sptGrossNoncomposite, releaseIntervalIdx, mid_poi, false /*no temporary strands*/, &nEff );
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         gd.StrandEccentricityAtHPs(dval);

         // prestressing strand material type
         KDOT::PrestressingStrandType pstype;
         const matPsStrand* pmatps = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight);

         std::_tstring name = pmatps->GetName();
         pstype.Name(T2A(name.c_str()));

         dval = pmatps->GetNominalDiameter();
         dval = ::ConvertFromSysUnits(dval, unitMeasure::Inch);
         pstype.NominalDiameter(dval);
         
         gd.PrestressingStrandMaterial(pstype);

         // Strand data
         // Straight strands
         StrandIndexType sival;
         sival = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight);
         gd.NumberOfStraightStrands(sival);

         KDOT::GirderDataType::StraightStrandCoordinates_sequence strCoords;

         CComPtr<IPoint2dCollection> strPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Straight, &strPoints);

         StrandIndexType sicnt;
         strPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> strPoint;
            strPoints->get_Item(istrand, &strPoint);
            Float64 x, y;
            strPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            strCoords.push_back(kpnt);
         }

         gd.StraightStrandCoordinates(strCoords);

         // Debonding
         gd.NumberOfDebondedStraightStrands( pStrandGeom->GetNumDebondedStrands(segmentKey, pgsTypes::Straight) );

         KDOT::GirderDataType::StraightStrandDebonding_sequence debonds;

         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            Float64 dstart, dend;
            if (pStrandGeom->IsStrandDebonded(segmentKey, istrand, pgsTypes::Straight, &dstart, &dend))
            {
               dstart = ::ConvertFromSysUnits(dstart, unitMeasure::Inch);
               dend   = ::ConvertFromSysUnits(dend,   unitMeasure::Inch);

               ::KDOT::DebondDataType debond(istrand+1, dstart, dend);
               debonds.push_back(debond);
            }
         }

         gd.StraightStrandDebonding(debonds);

         // Extended strands
         KDOT::GirderDataType::StraightStrandExtensions_sequence extends;

         StrandIndexType next(0);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            bool extstart = pStrandGeom->IsExtendedStrand(segmentKey, pgsTypes::metStart, istrand, pgsTypes::Straight);
            bool extend = pStrandGeom->IsExtendedStrand(segmentKey, pgsTypes::metEnd, istrand, pgsTypes::Straight);
            if (extstart || extend)
            {
               ::KDOT::StrandExtensionDataType sed(istrand+1, extstart, extend);
               
               extends.push_back(sed);
               next++;
            }
         }

         gd.StraightStrandExtensions(extends);
         gd.NumberOfExtendedStraightStrands( next );

         // Harped strands
         sival = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped);
         gd.NumberOfHarpedStrands(sival);

         // coords at ends
         KDOT::GirderDataType::HarpedStrandCoordinatesAtEnds_sequence heCoords;

         CComPtr<IPoint2dCollection> hePoints;
         pStrandGeom->GetStrandPositions(end_poi, pgsTypes::Harped, &hePoints);

         hePoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> hePoint;
            hePoints->get_Item(istrand, &hePoint);
            Float64 x, y;
            hePoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            heCoords.push_back(kpnt);
         }

         gd.HarpedStrandCoordinatesAtEnds(heCoords);

         // coords at HP
         KDOT::GirderDataType::HarpedStrandCoordinatesAtHP_sequence hpCoords;

         CComPtr<IPoint2dCollection> hpPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Harped, &hpPoints);

         hpPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> hpPoint;
            hpPoints->get_Item(istrand, &hpPoint);
            Float64 x, y;
            hpPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            hpCoords.push_back(kpnt);
         }

         gd.HarpedStrandCoordinatesAtHP(hpCoords);

         // Temporary strands
         sival = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);
         gd.NumberOfTemporaryStrands(sival);

         KDOT::GirderDataType::TemporaryStrandCoordinates_sequence tmpCoords;

         CComPtr<IPoint2dCollection> tmpPoints;
         pStrandGeom->GetStrandPositions(mid_poi, pgsTypes::Temporary, &tmpPoints);

         tmpPoints->get_Count(&sicnt);
         for(StrandIndexType istrand=0; istrand<sicnt; istrand++)
         {
            CComPtr<IPoint2d> tmpPoint;
            tmpPoints->get_Item(istrand, &tmpPoint);
            Float64 x, y;
            tmpPoint->Location(&x, &y);
            y += Hg; // strands in pgsuper measured from top, we want from bottom
            x = ::ConvertFromSysUnits(x, unitMeasure::Inch);
            y = ::ConvertFromSysUnits(y, unitMeasure::Inch);

            KDOT::Point2DType kpnt(x, y);

            tmpCoords.push_back(kpnt);
         }

         gd.TemporaryStrandCoordinates(tmpCoords);

         // Long. rebar materials
         KDOT::RebarMaterialType lrbrmat;

         matRebar::Type rebarType;
         matRebar::Grade rebarGrade;
         pMaterials->GetSegmentLongitudinalRebarMaterial(segmentKey, &rebarType, &rebarGrade);

         std::_tstring grd = GenerateReinfGradeName(rebarGrade);
         lrbrmat.Grade(T2A(grd.c_str()));

         std::_tstring typ = GenerateReinfTypeName(rebarType);
         lrbrmat.Type(T2A(typ.c_str()));

         gd.LongitudinalRebarMaterial(lrbrmat);

         // Longitudinal bar rows
         const CLongitudinalRebarData* pRebarData = pLongRebar->GetSegmentLongitudinalRebarData(segmentKey);
         const std::vector<CLongitudinalRebarData::RebarRow>& rebar_rows = pRebarData->RebarRows;
         CollectionIndexType rowcnt = rebar_rows.size();

         gd.NumberOfLongitudinalRebarRows(rowcnt);

         Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

         KDOT::GirderDataType::LongitudinalRebarRows_sequence rows;
         std::vector<CLongitudinalRebarData::RebarRow>::const_iterator iter(rebar_rows.begin());
         std::vector<CLongitudinalRebarData::RebarRow>::const_iterator end(rebar_rows.end());
         for ( ; iter != end; iter++ )
         {
            const CLongitudinalRebarData::RebarRow& rowData = *iter;

            Float64 startLoc, endLoc;
            bool onGirder = rowData.GetRebarStartEnd(segment_length, &startLoc, &endLoc);

            const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(pRebarData->BarType, pRebarData->BarGrade, rowData.BarSize);
            if (pRebar)
            {
               KDOT::RebarRowInstanceType rebarRow;

               dval = ::ConvertFromSysUnits(startLoc, unitMeasure::Inch);
               rebarRow.BarStart(dval);

               dval = ::ConvertFromSysUnits(endLoc, unitMeasure::Inch);
               rebarRow.BarEnd(dval);

               rebarRow.Face(rowData.Face==pgsTypes::TopFace ? "Top" : "Bottom");

               dval = ::ConvertFromSysUnits(rowData.Cover, unitMeasure::Inch);
               rebarRow.Cover(dval);

               rebarRow.NumberOfBars(rowData.NumberOfBars);

               dval = ::ConvertFromSysUnits(rowData.BarSpacing, unitMeasure::Inch);
               rebarRow.Spacing(dval);

               rebarRow.Size(T2A(pRebar->GetName().c_str()));

               rows.push_back(rebarRow);
            }
         }

         gd.LongitudinalRebarRows(rows);

         // stirrups
         KDOT::RebarMaterialType srbrmat;

         pMaterials->GetSegmentTransverseRebarMaterial(segmentKey, &rebarType, &rebarGrade);

         grd = GenerateReinfGradeName(rebarGrade);
         srbrmat.Grade(T2A(grd.c_str()));

         typ = GenerateReinfTypeName(rebarType);
         srbrmat.Type(T2A(typ.c_str()));

         gd.TransverseReinforcementMaterial(srbrmat);

         // stirrup zones
         ZoneIndexType nz = pStirrupGeometry->GetPrimaryZoneCount(segmentKey);
         gd.NumberOfStirrupZones(nz);

         KDOT::GirderDataType::StirrupZones_sequence szones;

         for (ZoneIndexType iz=0; iz<nz; iz++)
         {
            KDOT::StirrupZoneType szone;

            Float64 zoneStart, zoneEnd;
            pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, iz, &zoneStart, &zoneEnd);

            dval = ::ConvertFromSysUnits(zoneStart, unitMeasure::Inch);
            szone.StartLocation(dval);

            dval = ::ConvertFromSysUnits(zoneEnd, unitMeasure::Inch);
            szone.EndLocation(dval);

            matRebar::Size barSize;
            Float64 spacing;
            Float64 nStirrups;
            pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey,iz,&barSize,&nStirrups,&spacing);

            szone.BarSize(T2A(lrfdRebarPool::GetBarSize(barSize).c_str()));

            dval = ::ConvertFromSysUnits(spacing, unitMeasure::Inch);
            szone.BarSpacing(dval);

            szone.NumVerticalLegs(nStirrups);

            Float64 num_legs = pStirrupGeometry->GetPrimaryHorizInterfaceBarCount(segmentKey,iz);
            szone.NumLegsExtendedIntoDeck(num_legs);

            barSize = pStirrupGeometry->GetPrimaryConfinementBarSize(segmentKey,iz);
            szone.ConfinementBarSize(T2A(lrfdRebarPool::GetBarSize(barSize).c_str()));

            szones.push_back(szone);
         }

         gd.StirrupZones(szones);

         // Camber
         KDOT::GirderDataType::CamberResults_sequence camberResults;

         // First need to compute POI locations - results are at tenth points between pier/girder intersections
         // This is different than normal PGSuper POI's
         // &pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2
         Float64 distFromPierToPier, distFromPierToStartGdr, distFromPierToStartBrg, distFromPierToEndBrg;
         pntPier1->DistanceEx(pntPier2, &distFromPierToPier);
         pntPier1->DistanceEx(pntBrg2, &distFromPierToEndBrg);

         distFromPierToStartBrg = pBridge->GetSegmentStartBearingOffset(segmentKey);
         distFromPierToStartGdr = distFromPierToStartBrg - pBridge->GetSegmentStartEndDistance(segmentKey);

         struct PoiLocType
         {
            Float64 fracLoc;
            Float64 distFromPier;
            Float64 distFromBrg;
            Float64 Adim; // A distance at location
         };

         std::vector<PoiLocType> poiLocs; // possible POI locations
         poiLocs.reserve(11); // 11 possible pnts
         for(Int32 i=0; i<=10; i++) 
         {
            Float64 fracLoc = (Float64)i/10;
            Float64 loc = distFromPierToPier * fracLoc;

            // only use locations at or between bearings
            if (loc>=distFromPierToStartBrg && loc <= distFromPierToEndBrg)
            {
               PoiLocType ploc;
               ploc.fracLoc      = fracLoc;
               ploc.distFromPier = loc;
               ploc.distFromBrg  = loc - distFromPierToStartBrg;

               // A dimension height at location
               ploc.Adim = ::LinInterp( ploc.distFromBrg, adstart, adend, girderSpanLength);

               poiLocs.push_back(ploc);
            }
         }

         for(std::vector<PoiLocType>::const_iterator itl=poiLocs.begin(); itl!=poiLocs.end(); itl++)
         {
            const PoiLocType& poiloc = *itl;

            KDOT::CamberResultType camberResult;
            camberResult.FractionalLocation(poiloc.fracLoc);

            dval = ::ConvertFromSysUnits(poiloc.distFromPier, unitMeasure::Inch);
            camberResult.Location(dval);

            dval = ::ConvertFromSysUnits(poiloc.distFromBrg, unitMeasure::Inch);
            camberResult.LocationFromEndOfGirder(dval);

            pgsPointOfInterest mpoi(segmentKey, poiloc.distFromBrg);

            Float64 sta, offset;
            pBridge->GetStationAndOffset(mpoi,&sta,&offset);

            Float64 elev = pAlignment->GetElevation(sta,offset);
            dval = ::ConvertFromSysUnits(elev, unitMeasure::Inch);

            camberResult.TopOfDeckElevation(dval);

            // Girder chord elevation
            Float64 topOfGirderChord = elev - poiloc.Adim;
            dval = ::ConvertFromSysUnits(topOfGirderChord, unitMeasure::Inch);
            camberResult.TopOfGirderChordElevation(dval);

            // Get cambers at POI
            Float64 DpsRelease  = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,mpoi,bat,rtCumulative,false);
            Float64 DgdrRelease = pProduct->GetDeflection(releaseIntervalIdx,pgsTypes::pftGirder,mpoi,bat,rtCumulative,false);
            Float64 Dcreep = pCamber->GetCreepDeflection( mpoi, ICamber::cpReleaseToDeck, CREEP_MAXTIME, pgsTypes::pddErected );

            Float64 releaseCamber = DpsRelease + DgdrRelease;
            Float64 slabCastingCamber = releaseCamber + Dcreep;
            Float64 excessCamber = pCamber->GetExcessCamber(mpoi,CREEP_MAXTIME);

            Float64 topAtSlabCasting = topOfGirderChord + slabCastingCamber;
            dval = ::ConvertFromSysUnits(topAtSlabCasting, unitMeasure::Inch);
            camberResult.TopOfGirderElevationPriorToSlabCasting(dval);

            Float64 topAtFinal = topOfGirderChord + excessCamber;
            dval = ::ConvertFromSysUnits(topAtFinal, unitMeasure::Inch);
            camberResult.TopOfGirderElevationAtFinal(dval);

            dval = ::ConvertFromSysUnits(releaseCamber, unitMeasure::Inch);
            camberResult.GirderCamberAtRelease(dval);

            dval = ::ConvertFromSysUnits(slabCastingCamber, unitMeasure::Inch);
            camberResult.GirderCamberPriorToDeckCasting(dval);

            dval = ::ConvertFromSysUnits(excessCamber, unitMeasure::Inch);
            camberResult.GirderCamberAtFinal(dval);

            camberResults.push_back(camberResult);
         }

         gd.CamberResults(camberResults);

         // Lastly, compute girder haunch volume and sum for bridge haunch weight
         Float64 haunchWidth = pGirder->GetTopFlangeWidth(mid_poi);

         // volume of haunch assuming flat girder
         Float64 haunchVolNoCamber = girderLength * haunchWidth * ((adstart+adend)/2 - tDeck);

         // assume that camber makes a parabolic shape along entire length of girder
         Float64 midSpanExcessCamber = pCamber->GetExcessCamber(mid_poi,CREEP_MAXTIME);

         // Area under parabolic segment is 2/3(width)(height) 
         Float64 camberVolume = 2.0/3.0 * haunchWidth * midSpanExcessCamber * girderLength;

         // camber adjusted haunch volume
         Float64 haunchVol = haunchVolNoCamber - camberVolume;
         dval = ::ConvertFromSysUnits(haunchVol, unitMeasure::Inch3);
         gd.GirderHaunchVolume(dval);

         bridgeHaunchVolume += haunchVol;

         // The Collection of girders
         gds.push_back(gd);
      }

      brdata.GirderData(gds);

      // total bridge haunch
      dval = ::ConvertFromSysUnits(bridgeHaunchVolume, unitMeasure::Inch3);
      brdata.HaunchVolumeForAllSelectedGirders(dval);

      // Now can compute haunch weight for entire bridge
      Float64 haunchWDensity = pMaterials->GetDeckWeightDensity(compositeDeckIntervalIdx) * unitSysUnitsMgr::GetGravitationalAcceleration();
      Float64 bridgeHaunchWeight = bridgeHaunchVolume * haunchWDensity;

      dval = ::ConvertFromSysUnits(bridgeHaunchWeight, unitMeasure::Kip);
      brdata.HaunchWeightForAllSelectedGirders(dval);

      // Set data for main export class
      kdot_export.BridgeData(brdata);

      // save the XML to a file
      xml_schema::namespace_infomap map;
      map[""].name = "";
      map[""].schema = "KDOTExport.xsd"; // get this from a compiled resource if possible

      std::ofstream ofile(T2A(strFileName.GetBuffer()));

      KDOT::KDOTExport_(ofile,kdot_export,map);
   }
   catch ( const xml_schema::exception& e)
   {
      // an xml schema error has occured. Give the user a nice message and include the message from the exception so we have a
      // fighting chance of resolving the issue.
      CString strMsg;
      strMsg.Format(_T("An error has occured. Your PGSuper data could not be exported for KDOT CAD.\n\n(%s)"),e.what());
      AfxMessageBox(strMsg);
      return S_OK;
   }
   catch (... )
   {
      // an error outside our scope has occured. tell the user we can't export their data... then re-throw the exception and let PGSuper deal with it
      // this happens sometimes if live load distribution factors couldn't be computed or some other THROW_UNWIND type exception
      AfxMessageBox(_T("An error has occured. Your PGSuper data could not be exported for KDOT CAD.\n\nLook in the PGSuper Status Center for additional information."));
      throw;
   }
   } // progress window scope

   return S_OK;
}

//////////////////////////////////////////////////
// IPGSDocumentation
STDMETHODIMP CPGSuperDataExporter::GetDocumentationSetName(BSTR* pbstrName)
{
   CComBSTR bstrDocSetName(_T("KDOT"));
   bstrDocSetName.CopyTo(pbstrName);
   return S_OK;
}

CString CPGSuperDataExporter::GetDocumentationURL()
{
   USES_CONVERSION;

   CComBSTR bstrDocSetName;
   GetDocumentationSetName(&bstrDocSetName);
   CString strDocSetName(OLE2T(bstrDocSetName));

   CEAFApp* pApp = EAFGetApp();
   CString strDocumentationRootLocation = pApp->GetDocumentationRootLocation();

   CString strDocumentationURL;
   strDocumentationURL.Format(_T("%s%s/"),strDocumentationRootLocation,strDocSetName);

   if ( pApp->UseOnlineDocumentation() )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());

      CVersionInfo verInfo;
      CString strAppName = AfxGetAppName(); // needs module state
      strAppName += _T(".dll");
      verInfo.Load(strAppName);

      CString strVersion = verInfo.GetProductVersionAsString();

      // remove the build and release number
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
      pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);

      CString strURL;
      strURL.Format(_T("%s%s/"),strDocumentationURL,strVersion);
      strDocumentationURL = strURL;
   }

   return strDocumentationURL;
}

STDMETHODIMP CPGSuperDataExporter::LoadDocumentationMap()
{
   USES_CONVERSION;

   CComBSTR bstrDocSetName;
   GetDocumentationSetName(&bstrDocSetName);

   CString strDocSetName(OLE2T(bstrDocSetName));

   CEAFApp* pApp = EAFGetApp();

   CString strDocumentationRootLocation = pApp->GetDocumentationRootLocation();

   CString strDocumentationURL = GetDocumentationURL();

   CString strDocMapFile = EAFGetDocumentationMapFile(strDocSetName,strDocumentationURL,strDocumentationRootLocation);

   EAFLoadDocumentationMap(strDocMapFile,m_HelpTopics);
   return S_OK;
}

STDMETHODIMP CPGSuperDataExporter::GetDocumentLocation(UINT nHID,BSTR* pbstrURL)
{
   std::map<UINT,CString>::iterator found = m_HelpTopics.find(nHID);
   if ( found == m_HelpTopics.end() )
   {
      return E_FAIL;
   }

   CString strURL;
   strURL.Format(_T("%s%s"),GetDocumentationURL(),found->second);
   CComBSTR bstrURL(strURL);
   bstrURL.CopyTo(pbstrURL);
   return S_OK;
}
