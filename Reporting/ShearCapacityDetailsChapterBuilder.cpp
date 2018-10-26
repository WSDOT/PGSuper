///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\ShearCapacityDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\InterfaceShearDetails.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\RatingSpecification.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\BeamFactory.h>

#include <Reporter\ReportingUtils.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <algorithm>


// Static data and function for determining if poi is outside of CSS's
   // Locations of CSS's
   static Float64 S_LeftCs;
   static Float64 S_RightCs;

   static bool IsPoiInEndRegion(const pgsPointOfInterest& rPoi)
   {
      //Float64 loc = rPoi.GetDistFromStart();
      //return loc < S_LeftCs || loc > S_RightCs;
#pragma Reminder("UPDATE") // commented out during PGSplice development... need to have this working correctly
      return false;
   }


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CShearCapacityDetailsChapterBuilder
****************************************************************************/

void write_shear_dimensions_table(IBroker* pBroker,
                                IEAFDisplayUnits* pDisplayUnits,
                                const std::vector<pgsPointOfInterest>& pois,
                                rptChapter* pChapter,
                                IntervalIndexType intervalIdx,
                                const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_shear_stress_table(  IBroker* pBroker,
                                IEAFDisplayUnits* pDisplayUnits,
                                const std::vector<pgsPointOfInterest>& pois,
                                rptChapter* pChapter,
                                IntervalIndexType intervalIdx,
                                const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpo_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpc_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpce_table(IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      const std::vector<pgsPointOfInterest>& pois,
                      rptChapter* pChapter,
                      IntervalIndexType intervalIdx,
                      const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Fe_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_btsummary_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_ex_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vs_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_theta_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vc_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vci_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vcw_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vn_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Avs_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_bar_spacing_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShearCapacityDetailsChapterBuilder::CShearCapacityDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CShearCapacityDetailsChapterBuilder::GetName() const
{
   return TEXT("Shear Capacity Details");
}

rptChapter* CShearCapacityDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      bRating = pRatingSpec->AlwaysLoadRate();

      // if none of the rating types are enabled, skip the rating
      if ( !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) &&
           !pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) 
         )
         bRating = false;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount(girderKey)-1;
   std::_tstring stage_name( pIntervals->GetDescription(girderKey,intervalIdx) );

   GET_IFACE2(pBroker,IShearCapacity,pShearCapacity);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CGirderKey thisGirderKey(grpIdx,gdrIdx);

         rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;

         bool bPermit = pLimitStateForces->IsStrengthIIApplicable(thisGirderKey);

         std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(CSegmentKey(thisGirderKey,ALL_SEGMENTS)) );
         pIPOI->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);

         std::vector<pgsTypes::LimitState> vLimitStates;
         if ( bDesign )
         {
            vLimitStates.push_back(pgsTypes::StrengthI);
            if ( bPermit )
               vLimitStates.push_back(pgsTypes::StrengthII);
         }

         if ( bRating )
         {
            GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
               vLimitStates.push_back(pgsTypes::StrengthI_Inventory);

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
               vLimitStates.push_back(pgsTypes::StrengthI_Operating);

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
               vLimitStates.push_back(pgsTypes::StrengthI_LegalRoutine);

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
               vLimitStates.push_back(pgsTypes::StrengthI_LegalSpecial);

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
               vLimitStates.push_back(pgsTypes::StrengthII_PermitRoutine);

            if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
               vLimitStates.push_back(pgsTypes::StrengthII_PermitSpecial);
         }

         std::vector<pgsTypes::LimitState>::iterator iter;
         for ( iter = vLimitStates.begin(); iter != vLimitStates.end(); iter++ )
         {
            pgsTypes::LimitState ls = *iter;

            // Store css location for later use
            std::vector<Float64> vcsLoc(pShearCapacity->GetCriticalSections(ls, thisGirderKey));
            //S_LeftCs = vcsLoc.front();
            //S_RightCs = vcsLoc.back();

            write_shear_dimensions_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);

            if ( shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001 )
            {
               write_shear_stress_table    (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_fpc_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_fpo_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Fe_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_ex_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_btsummary_table       (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vc_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vs_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            }
            else if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
            {
               write_fpo_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_ex_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_btsummary_table       (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vc_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vs_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            }
            else if ( shear_capacity_method == scmVciVcw )
            {
               write_fpce_table            (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vci_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_fpc_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vcw_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_theta_table           (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
               write_Vs_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            }

            write_Vn_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
         }

         if ( bDesign )
         {
            write_Avs_table(        pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, pgsTypes::StrengthI);
            write_bar_spacing_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, pgsTypes::StrengthI);

            if ( bPermit )
            {
               write_Avs_table(        pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, pgsTypes::StrengthII);
               write_bar_spacing_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, pgsTypes::StrengthII);
            }
         }

         /////////////////////////////////////////////
         // Horizontal interface shear - only reported for design
         /////////////////////////////////////////////
         if ( bDesign )
         {
            GET_IFACE2(pBroker,IBridge,pBridge);
            if ( pBridge->IsCompositeDeck() )
            {
               pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               pPara->SetName(_T("Horizontal Interface Shear"));
               *pPara << pPara->GetName() << rptNewLine;
               *pChapter << pPara;
               CInterfaceShearDetails::Build(pBroker, pChapter, thisGirderKey, pDisplayUnits, intervalIdx,  pgsTypes::StrengthI);

               if ( bPermit )
                  CInterfaceShearDetails::Build(pBroker, pChapter, thisGirderKey, pDisplayUnits, intervalIdx,  pgsTypes::StrengthII);
            }
         }
      } // next girder
   } // next group

   return pChapter;
}

CChapterBuilder* CShearCapacityDetailsChapterBuilder::Clone() const
{
   return new CShearCapacityDetailsChapterBuilder(m_bDesign,m_bRating);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

void write_shear_dimensions_table(IBroker* pBroker,
                             IEAFDisplayUnits* pDisplayUnits,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             IntervalIndexType intervalIdx,
                             const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IEventMap,pEventMap);

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   pParagraph->SetName(OLE2T(pEventMap->GetLimitStateName(ls)));
   *pParagraph << _T("Effective Shear Dimensions for ") << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(" [From Article 5.8.2.7]") << rptNewLine;

   
   CGirderKey girderKey = pois.front().GetSegmentKey();


   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->DeckType;

   std::_tstring strPicture = pFactory->GetShearDimensionsSchematicImage(deckType);
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strPicture);

   *pParagraph << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("dv.png")) << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9);
   *pParagraph << table << rptNewLine;


   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1) << COLHDR(Sub2(_T("b"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2) << COLHDR(Sub2(_T("d"),_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR(_T("h"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR(Sub2(_T("0.9d"),_T("e")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR(_T("0.72h"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR(_T("Moment") << rptNewLine << _T("Arm"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,7) << COLHDR(Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,8) << _T("Tension") << rptNewLine << _T("Side");


   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << dim.SetValue( scd.bv );
      (*table)(row,2) << dim.SetValue( scd.de );
      (*table)(row,3) << dim.SetValue( scd.h );
      (*table)(row,4) << dim.SetValue( 0.9*scd.de );
      (*table)(row,5) << dim.SetValue( 0.72*scd.h );
      (*table)(row,6) << dim.SetValue( scd.MomentArm );
      (*table)(row,7) << dim.SetValue( scd.dv );

      (*table)(row,8) << (scd.bTensionBottom ? _T("Bottom") : _T("Top"));

      row++;
   }
}

void write_shear_stress_table(IBroker* pBroker,
                              IEAFDisplayUnits* pDisplayUnits,
                              const std::vector<pgsPointOfInterest>& pois,
                              rptChapter* pChapter,
                              IntervalIndexType intervalIdx,
                              const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   std::_tstring strEquation;
   switch( lrfdVersionMgr::GetVersion() )
   {
      case lrfdVersionMgr::FirstEdition1994:
      case lrfdVersionMgr::FirstEditionWith1996Interims:
      case lrfdVersionMgr::FirstEditionWith1997Interims:
      case lrfdVersionMgr::SecondEdition1998:
      case lrfdVersionMgr::SecondEditionWith1999Interims:
         strEquation = _T(" [Eqn 5.8.3.4.2-1]");
         break;

      case lrfdVersionMgr::SecondEditionWith2000Interims:
      case lrfdVersionMgr::SecondEditionWith2001Interims:
      case lrfdVersionMgr::SecondEditionWith2002Interims:
      case lrfdVersionMgr::SecondEditionWith2003Interims:
      case lrfdVersionMgr::ThirdEdition2004:
         strEquation = _T(" [Eqn 5.8.2.9-1]");
         break;

      case lrfdVersionMgr::ThirdEditionWith2005Interims:
      case lrfdVersionMgr::ThirdEditionWith2006Interims:
         strEquation = _T(" [Eqn 5.8.2.4-1]");
         break;

      case lrfdVersionMgr::FourthEdition2007:
      case lrfdVersionMgr::FourthEditionWith2008Interims:
      case lrfdVersionMgr::FourthEditionWith2009Interims:
      default:
         strEquation = _T(" [Eqn 5.8.2.9-1]");
         break;
   }

   *pParagraph << Italic(_T("v")) << strEquation << rptNewLine;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("vu.png")) << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("vu_2005.png")) << rptNewLine;

   *pParagraph << rptNewLine;

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   CGirderKey girderKey(pois.front().GetSegmentKey());
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   ColumnIndexType nColumns = (nDucts == 0 ? 7 : 9);

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   CString strTitle;
   strTitle.Format(_T("Factored Shear Stresses for %s"),OLE2T(pEventMap->GetLimitStateName(ls)));
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nColumns,strTitle);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   
   if ( 0 < nDucts )
   {
      (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("ps")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("pt")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("p")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR(_T("d") << Sub(_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(_T("b") << Sub(_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Italic(_T("v")) << Sub(_T("u")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,col++) << scd.Phi;
      (*table)(row,col++) << force.SetValue( scd.Vu );
      
      if ( 0 < nDucts )
      {
         (*table)(row,col++) << force.SetValue( scd.Vps );
         (*table)(row,col++) << force.SetValue( scd.Vpt );
      }

      (*table)(row,col++) << force.SetValue( scd.Vp );
      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << dim.SetValue( scd.bv );
      (*table)(row,col++) << stress.SetValue( (scd.vfc * scd.fc) );

      row++;
   }
}

void write_fpc_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   if ( bAfter1999 && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001 ))
      return;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << RPT_STRESS(_T("pc")) << _T(" [for use in Eqn 5.8.3.4.3-3] - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;
   }
   else
   {
      *pParagraph << RPT_STRESS(_T("pc")) << _T(" [for use in Eqn C5.8.3.4.2-1] - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Fpc Pic.jpg")) << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("fpc_2007.png")) << rptNewLine;

   *pParagraph << rptNewLine;


   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   CGirderKey girderKey(pois.front().GetSegmentKey());
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   ColumnIndexType nCols = (0 == nDucts ? 8 : 10);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCols);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   if ( 0 == nDucts )
   {
      (*table)(0,col++) << COLHDR(_T("e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,col++) << COLHDR(_T("P"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,col++) << COLHDR(Sub2(_T("P"),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0,col++) << COLHDR(Sub2(_T("P"),_T("pt")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   }
   (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("g")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(_T("c"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("DC")) << _T(" + ") << Sub2(_T("M"),_T("DW")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(RPT_STRESS(_T("pc")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,    pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,   pDisplayUnits->GetMomentUnit(),          false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());
   
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      FPCDETAILS fpcd;
      pShearCap->GetFpcDetails(poi, &fpcd);

      (*table)(row,col++) << location.SetValue(POI_ERECTED_SEGMENT, poi, end_size );
      if ( 0 == nDucts )
      {
         (*table)(row,col++) << dim.SetValue( fpcd.eps );
         (*table)(row,col++) << force.SetValue( fpcd.Pps );
      }
      else
      {
         (*table)(row,col++) << dim.SetValue( fpcd.eps );
         (*table)(row,col++) << force.SetValue( fpcd.Pps );
         (*table)(row,col++) << dim.SetValue( fpcd.ept );
         (*table)(row,col++) << force.SetValue( fpcd.Ppt );
      }
      (*table)(row,col++) << area.SetValue( fpcd.Ag );
      (*table)(row,col++) << inertia.SetValue( fpcd.Ig );
      (*table)(row,col++) << dim.SetValue( fpcd.c );
      (*table)(row,col++) << moment.SetValue( fpcd.Mg );
      (*table)(row,col++) << stress.SetValue( fpcd.fpc );

      row++;
   }
}

void write_fpce_table(IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      const std::vector<pgsPointOfInterest>& pois,
                      rptChapter* pChapter,
                      IntervalIndexType intervalIdx,
                      const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << Sub2(_T("M"),_T("cre")) << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(" [Eqn 5.8.3.4.3-2]") << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Mcre.png")) << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1)  << COLHDR( RPT_STRESS(_T("r")),   rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2)  << COLHDR( RPT_STRESS(_T("cpe")), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR( Sub2(_T("S"),_T("nc")),  rptLength3UnitTag,  pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,4)  << COLHDR( Sub2(_T("S"),_T("c")),   rptLength3UnitTag,  pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,5)  << COLHDR( Sub2(_T("M"),_T("dnc")), rptMomentUnitTag,   pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR( Sub2(_T("M"),_T("cre")), rptMomentUnitTag,   pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest,   location,       pDisplayUnits->GetSpanLengthUnit(),         false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress,         pDisplayUnits->GetStressUnit(),             false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue,  sect_mod,       pDisplayUnits->GetSectModulusUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,         pDisplayUnits->GetMomentUnit(),             false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, fr_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << stress.SetValue( scd.McrDetails.fr );
      (*table)(row,2) << stress.SetValue( scd.McrDetails.fcpe);
      (*table)(row,3) << sect_mod.SetValue( scd.McrDetails.Sb );
      (*table)(row,4) << sect_mod.SetValue( scd.McrDetails.Sbc );
      (*table)(row,5) << moment.SetValue( scd.McrDetails.Mdnc);
      (*table)(row,6) << moment.SetValue( scd.McrDetails.Mcr );


      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IMaterials,pMaterial);
#pragma Reminder("UPDATE") // next line commented out during development of PGSplice. It needs to be fixed
   //*pParagraph << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetShearFrCoefficient(segmentKey)) << symbol(ROOT) << RPT_FC << rptNewLine;

   *pParagraph << RPT_STRESS(_T("cpe")) << _T(" = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads.") << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("fcpe.png")) << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("nc")) << _T(" = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("c")) << _T(" = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("M"),_T("dnc")) << _T(" = total unfactored dead load moment acting on the monolithic or noncomposite section") << rptNewLine;
}

void write_fpo_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << RPT_STRESS(_T("po"));

   if ( !bAfter1999 )
      *pParagraph << _T(" [Eqn C5.8.3.4.2-1]");

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << _T(" - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   //const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
#pragma Reminder("UPDATE") // this strand material is only for the first segment. it could be different for other segments
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(pois.front().GetSegmentKey(),pgsTypes::Permanent);
   const matPsStrand* pTendon = pMaterial->GetTendonMaterial(pois.front().GetSegmentKey());

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   CGirderKey girderKey(pois.front().GetSegmentKey());
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    true );
   if ( bAfter1999 )
   {
      // See PCI BDM 8.4.1.1.4
      Float64 Kps, Kpt;
      if ( 0 == nDucts )
      {
         if ( pStrand->GetType() == matPsStrand::LowRelaxation )
         {
            Kps = 0.75;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po")) << _T(" = 0.75") << Sub2(_T("f"),_T("pu")) << italic(OFF) << _T(" (PCI BDM 8.4.1.1.4)") << rptNewLine;
         }
         else
         {
            Kps = 0.70;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po")) << _T(" = 0.70") << Sub2(_T("f"),_T("pu")) << italic(OFF) << rptNewLine;
         }

         *pParagraph << RPT_STRESS(_T("po")) << _T(" = ") << stress.SetValue(Kps*pStrand->GetUltimateStrength()) << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Strands") << rptNewLine;
         if ( pStrand->GetType() == matPsStrand::LowRelaxation )
         {
            Kps = 0.75;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po ps")) << _T(" = 0.75") << Sub2(_T("f"),_T("pu")) << italic(OFF) << _T(" (PCI BDM 8.4.1.1.4)") << rptNewLine;
         }
         else
         {
            Kps = 0.70;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po ps")) << _T(" = 0.70") << Sub2(_T("f"),_T("pu")) << italic(OFF) << rptNewLine;
         }

         *pParagraph << RPT_STRESS(_T("po ps")) << _T(" = ") << stress.SetValue(Kps*pStrand->GetUltimateStrength()) << rptNewLine;

         *pParagraph << _T("Tendons") << rptNewLine;
         if ( pTendon->GetType() == matPsStrand::LowRelaxation )
         {
            Kpt = 0.75;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po pt")) << _T(" = 0.75") << Sub2(_T("f"),_T("pu")) << italic(OFF) << _T(" (PCI BDM 8.4.1.1.4)") << rptNewLine;
         }
         else
         {
            Kpt = 0.70;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po pt")) << _T(" = 0.70") << Sub2(_T("f"),_T("pu")) << italic(OFF) << rptNewLine;
         }

         *pParagraph << RPT_STRESS(_T("po pt")) << _T(" = ") << stress.SetValue(Kpt*pTendon->GetUltimateStrength()) << rptNewLine;
      }
   }
   else
   {
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("fpo.png")) << rptNewLine;
   }

   *pParagraph << rptNewLine;

   if ( !bAfter1999 )
   {
      ColumnIndexType nCols = (0 == nDucts ? 6 : 9);

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCols);

      //if ( segmentKey.groupIndex == ALL_GROUPS )
      //{
      //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      //}

      *pParagraph << table << rptNewLine;

      ColumnIndexType col = 0;
      (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      if ( 0 == nDucts )
      {
         (*table)(0,col++) << COLHDR( RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR( italic(ON) << Sub2(_T("f"),_T("pe ps")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR( italic(ON) << Sub2(_T("f"),_T("pe pt")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      (*table)(0,col++) << COLHDR( RPT_FPC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      if ( 0 == nDucts )
      {
         (*table)(0,col++) << COLHDR( RPT_EP,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("ps")),  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
         (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("pt")),  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      }
      
      (*table)(0,col++) << COLHDR( RPT_EC,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );

      if ( 0 == nDucts )
      {
         (*table)(0,col++) << COLHDR( RPT_FPO, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR( italic(ON) << Sub2(_T("f"),_T("po ps")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*table)(0,col++) << COLHDR( italic(ON) << Sub2(_T("f"),_T("po pt")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }

      INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),      false );

      //location.IncludeSpanAndGirder(span == ALL_SPANS);

      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

      RowIndexType row = 1;
      GET_IFACE2(pBroker,IShearCapacity,pShearCap);
      std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
      for ( ; i != end; i++ )
      {
         col = 0;

         const pgsPointOfInterest& poi = *i;

         // Don't print pois outside of CSS's
         if (IsPoiInEndRegion(poi))
            continue;

         SHEARCAPACITYDETAILS scd;
         pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

         (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
         (*table)(row,col++) << stress.SetValue( scd.fpeps );
         
         if ( 0 < nDucts )
         {
            (*table)(row,col++) << stress.SetValue( scd.fpept );
         }

         (*table)(row,col++) << stress.SetValue( scd.fpc );
         (*table)(row,col++) << mod_e.SetValue( scd.Eps );

         if ( 0 < nDucts )
         {
            (*table)(row,col++) << mod_e.SetValue( scd.Ept );
         }

         (*table)(row,col++) << mod_e.SetValue( scd.Ec );
         (*table)(row,col++) << stress.SetValue( scd.fpops );

         if ( 0 < nDucts )
         {
            (*table)(row,col++) << stress.SetValue( scd.fpopt );
         }

         row++;
      }
   }
}

void write_Fe_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims )
      return; // This is not applicable 2000 and later

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << Sub2(_T("F"),symbol(epsilon)) << _T(" [Eqn 5.8.3.4.2-3] - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Fe.png")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << _T("This calculation is applicable only when ") << symbol(epsilon) << Sub(_T("x")) << _T(" is negative.") << rptNewLine;
   *pParagraph << rptNewLine;

   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   CGirderKey girderKey(pois.front().GetSegmentKey());
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   ColumnIndexType nCols = (0 == nDucts ? 8 : 10);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCols);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( RPT_ES, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,col++) << COLHDR( RPT_AS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( RPT_EPS, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,col++) << COLHDR( RPT_APS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   if ( 0 < nDucts )
   {
      (*table)(0,col++) << COLHDR( RPT_EPT, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,col++) << COLHDR( RPT_APT, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   }
   (*table)(0,col++) << COLHDR( RPT_EC, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,col++) << COLHDR( RPT_AC, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << _T("F") << Sub(symbol(epsilon));

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),            false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,     pDisplayUnits->GetAreaUnit(),            false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   scalar.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   scalar.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      col = 0;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue(POI_ERECTED_SEGMENT, poi, end_size );

      if ( scd.Fe < 0 )
      {
         (*table)(row,col++) << _T("-");
         (*table)(row,col++) << _T("-");
         (*table)(row,col++) << _T("-"); 
         (*table)(row,col++) << _T("-");
         if ( 0 < nDucts )
         {
            (*table)(row,col++) << _T("-"); 
            (*table)(row,col++) << _T("-");
         }
         (*table)(row,col++) << _T("-"); 
         (*table)(row,col++) << _T("-"); 
         (*table)(row,col++) << _T("-"); 
      }
      else
      {
         (*table)(row,col++) << mod_e.SetValue( scd.Es );
         (*table)(row,col++) << area.SetValue( scd.As );
         (*table)(row,col++) << mod_e.SetValue( scd.Eps );
         (*table)(row,col++) << area.SetValue( scd.Aps );
         if ( 0 < nDucts )
         {
            (*table)(row,col++) << mod_e.SetValue( scd.Ept );
            (*table)(row,col++) << area.SetValue( scd.Apt );
         }
         (*table)(row,col++) << mod_e.SetValue( scd.Ec );
         (*table)(row,col++) << area.SetValue( scd.Ac );
         (*table)(row,col++) << scalar.SetValue(scd.Fe);
      }

      row++;
   }
}

void write_ex_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );
   bool bAfter2003 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2002Interims ? true : false );
   bool bAfter2004 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEditionWith2005Interims  ? true : false );
   bool bAfter2007 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::FourthEditionWith2008Interims ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Longitudinal Strain ") << Sub2(symbol(epsilon),_T("x"));
   if ( !bAfter1999 )
      *pParagraph << _T(" [Eqn 5.8.3.4.2-2] ");

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << _T("- ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;

   if ( bAfter2007 )
   {
      if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
      {
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2008.png")) << rptNewLine;
      }
      else if ( shear_capacity_method == scmWSDOT2001 )
      {
         // tables with WSDOT modifications
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2003_WSDOT.png")) << rptNewLine;
      }
      else
      {
         // tables
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2005.png")) << rptNewLine;
      }
   }
   else if ( bAfter2004 )
   {
      if ( shear_capacity_method == scmWSDOT2007 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2008.png")) << rptNewLine;
      else if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2003_WSDOT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2005.png")) << rptNewLine;
   }
   else if ( bAfter2003 )
   {
      ATLASSERT(shear_capacity_method != scmWSDOT2007);

      if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2003_WSDOT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2003.png")) << rptNewLine;
   }
   else if ( bAfter1999 )
   {
      if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2000_WSDOT.png")) << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex_2000.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ex.png")) << rptNewLine; // 1999 and earlier
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( !bAfter1999 )
   {
      *pParagraph << Sub2(symbol(epsilon),_T("x")) << _T(" includes ") << Sub2(_T("F"),symbol(epsilon)) << _T(" when applicable") << rptNewLine;
      *pParagraph << rptNewLine;
   }

   Int16 nCol = (bAfter1999 && shear_capacity_method == scmBTTables ? 14 : 12);
   if ( shear_capacity_method == scmWSDOT2001 || 
        shear_capacity_method == scmWSDOT2007 || 
        shear_capacity_method == scmBTEquations 
      )
   {
      nCol--;
   }

   if ( bAfter1999 )
   {
      nCol += 2;
   }


   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
   CGirderKey girderKey(pois.front().GetSegmentKey());
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   if ( 0 < nDucts )
   {
      nCol += 4;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   ColumnIndexType col = 0;

   *pParagraph << table << rptNewLine;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( bAfter1999  && shear_capacity_method == scmBTTables )
   {
      (*table)(0,col++) << _T("Min. Reinf.") << rptNewLine << _T("per 5.8.2.5") ;
      (*table)(0,col++) << _T("Eqn") << rptNewLine << _T("5.8.3.4.2-");
   }

   (*table)(0,col++) << COLHDR( Sub2(_T("M"),_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bAfter1999 )
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      if ( 0 < nDucts )
      {
         (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("ps")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
         (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("pt")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      }
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("p")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      (*table)(0,col++) << COLHDR( _T("|") << Sub2(_T("V"),_T("u")) << _T(" - ") << Sub2(_T("V"),_T("p")) << _T("|"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   (*table)(0,col++) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("s")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("s")), rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("ps")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("ps")), rptStressUnitTag, pDisplayUnits->GetModEUnit() );

   if ( 0 < nDucts )
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("pt")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("pt")), rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   }

   (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("c")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("c")), rptStressUnitTag, pDisplayUnits->GetModEUnit() );

   if ( shear_capacity_method != scmWSDOT2001 && 
        shear_capacity_method != scmWSDOT2007 &&
        shear_capacity_method != scmBTEquations 
      )
   {
      (*table)(0,col++) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   }

   (*table)(0,col++) << Sub2(symbol(epsilon),_T("x")) << rptNewLine << _T("x 1000");

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, mod_e, pDisplayUnits->GetModEUnit(), false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   bool print_footnote1 = false;
   bool print_footnote2 = false;

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      
      ColumnIndexType col = 0;
      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      if ( bAfter1999  && shear_capacity_method == scmBTTables )
      {
         (*table)(row,col++) << (scd.Equation == 1 || scd.Equation == 31 ? _T("Yes") : _T("No"));
         (*table)(row,col++) << (scd.Equation == 1 ? _T("1") : scd.Equation == 2 ? _T("2") : _T("3"));
      }

      (*table)(row,col) << moment.SetValue( scd.Mu );
      if ( bAfter1999 )
      {
         if ( scd.MuLimitUsed)
         {
            print_footnote1 = true;
            (*table)(row,col) << _T(" $");
         }
      }
      col++;

      if ( bAfter1999 )
      {
         (*table)(row,col++) << shear.SetValue( scd.Vu );
         if ( 0 < nDucts )
         {
            (*table)(row,col++) << shear.SetValue( scd.Vps );
            (*table)(row,col++) << shear.SetValue( scd.Vpt );
         }
         (*table)(row,col++) << shear.SetValue( scd.Vp );
         (*table)(row,col++) << shear.SetValue( fabs(scd.Vu - scd.Vp) );
      }
      else
      {
         (*table)(row,col++) << shear.SetValue( scd.Vu );
      }

      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << area.SetValue( scd.As );
      (*table)(row,col++) << mod_e.SetValue( scd.Es );
      (*table)(row,col++) << area.SetValue( scd.Aps );
      (*table)(row,col++) << mod_e.SetValue( scd.Eps );
      if ( 0 < nDucts )
      {
         (*table)(row,col++) << area.SetValue( scd.Apt );
         (*table)(row,col++) << mod_e.SetValue( scd.Ept );
      }

      (*table)(row,col++) << area.SetValue( scd.Ac );
      (*table)(row,col++) << mod_e.SetValue( scd.Ec );
      if (scd.ShearInRange)
      {
         if ( shear_capacity_method != scmWSDOT2001 && 
              shear_capacity_method != scmWSDOT2007 &&
              shear_capacity_method != scmBTEquations 
            )
         {
            (*table)(row,col++) << angle.SetValue( scd.Theta );
         }

         if ( bAfter1999 && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001) )
         {
            (*table)(row,col) << scalar.SetValue( scd.ex * 1000. );
            (*table)(row,col) << _T(" ") << symbol(LTE) << _T(" ") << scalar.SetValue( scd.ex_tbl*1000.0 );
            col++;
         }
         else
         {
            (*table)(row,col++) << scalar.SetValue( scd.ex * 1000. );
         }
      }
      else
      {
         print_footnote2 = true;
         (*table)(row,col++) << _T("*");
         (*table)(row,col++) << _T("*");
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote1 || print_footnote2)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;

      *pParagraph << Sub2(_T("N"),_T("u")) << _T(" = 0") << rptNewLine;

      if ( print_footnote1 )
      {
         if ( shear_capacity_method == scmWSDOT2007 || shear_capacity_method == scmBTEquations )
            *pParagraph << _T("$ - Taken as |") << Sub2(_T("V"),_T("u")) << _T(" - ") << Sub2(_T("V"),_T("p")) << _T("|") << Sub2(_T("d"),_T("v")) << _T(" per definitions given in 5.8.3.4.2") << rptNewLine;
         else
            *pParagraph << _T("$ - Taken as ") << Sub2(_T("V"),_T("u")) << Sub2(_T("d"),_T("v")) << _T(" per definitions given in 5.8.3.4.2") << rptNewLine;
      }

      if ( print_footnote2 )
         *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }

// To be removed from WSDOT BDM... 7/25/2006 RAB
//   if ( !bLrfdMethod )
//      *pParagraph << Sub2(symbol(theta),_T("min")) << _T(" = 25") << symbol(DEGREES) << _T(" beyond end region (1.5H). [WSDOT BDM 5.2.4F.2]") << rptNewLine;

}

void write_btsummary_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   // if this after 2007 spec then shear capacity method should not equal scmWSDOT2007
   bool bAfter2007 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::FourthEditionWith2008Interims ? true : false );
   ATLASSERT( bAfter2007 ? shear_capacity_method != scmWSDOT2007 : true );

   rptParagraph* pParagraph;
   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   CString strTitle;
   strTitle.Format(_T("Shear Parameters Summary - %s"),OLE2T(pEventMap->GetLimitStateName(ls)));
   *pParagraph << strTitle << rptNewLine;

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   ColumnIndexType nCol = 5;

   bool print_sxe = shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 || shear_capacity_method == scmBTTables;


   if (print_sxe)
   {
      nCol = 9; // need room for sx/sxe

      if (shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007)
      {
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("BetaEquation.png")) << rptNewLine;
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("ThetaEquation.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("SxeEqn.png")) << rptNewLine;
      }

      INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdimu,    pDisplayUnits->GetComponentDimUnit(),    true );
      GET_IFACE2(pBroker,IMaterials,pMat);

   #pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
      CSegmentKey segmentKey = pois.front().GetSegmentKey();

      Float64 ag = pMat->GetSegmentMaxAggrSize(segmentKey);
      *pParagraph << _T("a")<<Sub(_T("g"))<<_T(" = ")<<xdimu.SetValue(ag) << rptNewLine;
   }

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if (print_sxe)
   {
      (*table)(0,col++) << _T("Min. Reinf.") << rptNewLine << _T("per 5.8.2.5") ;

      if(shear_capacity_method == scmBTTables)
      {
         (*table)(0,col++) << _T("Table") << rptNewLine << _T("5.8.3.4.2-");
      }
      else
      {
         (*table)(0,col++) << _T("Eqn") << rptNewLine << _T("5.8.3.4.2-");
      }
      (*table)(0,col++) << COLHDR( _T("s")<< Sub(_T("x"))<<_T("*"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR( _T("s")<< Sub(_T("xe")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   (*table)(0,col++) << _T("v") << _T("/") << RPT_FC;
   (*table)(0,col++) << symbol(epsilon) << Sub(_T("x")) << _T(" x 1000");
   (*table)(0,col++) << symbol(beta);
   (*table)(0,col++) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdim,    pDisplayUnits->GetComponentDimUnit(),    false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      if (print_sxe)
      {
         bool do_mtr = (shear_capacity_method == scmBTTables) ? scd.BetaThetaTable == 1 : scd.BetaEqn==1;

         (*table)(row,col++) << (do_mtr  ? _T("Yes") : _T("No"));
         (*table)(row,col++) << (do_mtr  ? _T("1")   : _T("2") );
         if(do_mtr)
         {
            (*table)(row,col++) << _T("---");
            (*table)(row,col++) << _T("---");
         }
         else
         {
            (*table)(row,col++) << xdim.SetValue(scd.sx);
            (*table)(row,col) << xdim.SetValue(scd.sxe);
            if(shear_capacity_method == scmBTTables)
            {
               (*table)(row,col) << _T(" ") << symbol(LTE) << _T(" ") << xdim.SetValue(scd.sxe_tbl);
            }

            col++;
         }
      }

      if ( bAfter1999 && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001) )
      {
         // Don't print vfc if sxe method was used
         bool do_vfc = (shear_capacity_method == scmBTTables) ? scd.BetaThetaTable == 1 : scd.BetaEqn==1;

         if (do_vfc)
         {
            (*table)(row,col) << scalar.SetValue( scd.vfc );
            (*table)(row,col++) << _T(" ") << symbol(LTE) << _T(" ") << scalar.SetValue(scd.vfc_tbl);
         }
         else
         {
            (*table)(row,col++) << _T("---");
         }
      }
      else if ( shear_capacity_method != scmBTEquations && shear_capacity_method != scmWSDOT2007 )
      {
         (*table)(row,col++) << scalar.SetValue( scd.vfc );
      }
      else
      {
         (*table)(row,col++) << scalar.SetValue( scd.vfc );
      }

      if (scd.ShearInRange)
      {
         if( bAfter1999  && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001) )
         {
            (*table)(row,col) << scalar.SetValue( scd.ex * 1000.0);
            (*table)(row,col++) << _T(" ") << symbol(LTE) << _T(" ") << scalar.SetValue( scd.ex_tbl * 1000.0 );
         }
         else
         {
            (*table)(row,col++) << scalar.SetValue( scd.ex * 1000.0);
         }

         (*table)(row,col++) << scalar.SetValue( scd.Beta );
         (*table)(row,col++) << angle.SetValue( scd.Theta );
      }
      else
      {
         print_footnote=true;
         (*table)(row,col++) << _T("**");
         (*table)(row,col++) << _T("**");
         (*table)(row,col++) << _T("**");
      }

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   if (print_sxe)
   {
      *pParagraph << _T("* Value of s")<<Sub(_T("x"))<<_T(" taken equal to d")<<Sub(_T("v"))<< rptNewLine<<rptNewLine;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      *pParagraph << _T("** Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }
}

void write_Vs_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << _T("Shear Resistance Provided By Shear Reinforcement - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("Vs.png")) << rptNewLine;
   *pParagraph << rptNewLine;
   

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1) << COLHDR( RPT_FY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR( Sub2(_T("A"),_T("v")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,3) << COLHDR( _T("s"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR( Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,5) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   (*table)(0,7) << COLHDR( symbol(alpha), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   (*table)(0,8) << COLHDR( Sub2(_T("V"),_T("s")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs, pDisplayUnits->GetAvOverSUnit(), false );

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      bool is_end_rgn = IsPoiInEndRegion(poi);

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << stress.SetValue( scd.fy );
      (*table)(row,2) << area.SetValue( scd.Av );
      (*table)(row,3) << dim.SetValue( scd.S );
      if (scd.S > 0.0)
      {
         (*table)(row,4) << avs.SetValue( scd.Av/scd.S );
      }
      else
      {
         ATLASSERT(scd.Av == 0.0);
         (*table)(row,4) << avs.SetValue( 0.0 );
      }
      (*table)(row,5) << dim.SetValue( scd.dv );

      if (!is_end_rgn)
      {
         if (scd.ShearInRange)
         {
            (*table)(row,6) << angle.SetValue( scd.Theta );
            (*table)(row,7) << angle.SetValue( scd.Alpha );
            (*table)(row,8) << shear.SetValue( scd.Vs );
         }
         else
         {
            print_footnote=true;
            (*table)(row,6) << _T("*");
            (*table)(row,7) << _T("*");
            (*table)(row,8) << _T("*");
         }
      }
      else
      {
         (*table)(row,6) << RPT_NA;
         (*table)(row,7) << RPT_NA;
         (*table)(row,8) << RPT_NA;
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }

}

void write_Vc_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << _T("Shear Resistance Provided By Tensile Stress in the Concrete - ") << OLE2T(pEventMap->GetLimitStateName(ls)) << rptNewLine;

#pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
   CSegmentKey segmentKey = pois.front().GetSegmentKey();

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   std::_tstring strImage;
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   switch( concType )
   {
   case pgsTypes::Normal:
      strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_NWC_US.png") : _T("VcEquation_NWC_SI.png"));
      break;

   case pgsTypes::AllLightweight:
      if ( bHasAggSplittingStrength )
         strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_LWC_US.png") : _T("VcEquation_LWC_SI.png"));
      else
         strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_ALWC_US.png") : _T("VcEquation_ALWC_SI.png"));
      break;

   case pgsTypes::SandLightweight:
      if ( bHasAggSplittingStrength )
         strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_LWC_US.png") : _T("VcEquation_LWC_SI.png"));
      else
         strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_SLWC_US.png") : _T("VcEquation_SLWC_SI.png"));
      break;

   default:
      ATLASSERT(false);
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strImage) << rptNewLine;

   *pParagraph << rptNewLine;

   ColumnIndexType nCols = (concType != pgsTypes::Normal && bHasAggSplittingStrength ? 7 : 6);
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCols);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType colIdx = 0;

   (*table)(0,colIdx++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,colIdx++) << symbol(beta);
   (*table)(0,colIdx++) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   if ( concType != pgsTypes::Normal && bHasAggSplittingStrength )
      (*table)(0,colIdx++) << COLHDR( RPT_STRESS(_T("ct")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   (*table)(0,colIdx++) << COLHDR( Sub2(_T("b"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,colIdx++) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,colIdx++) << COLHDR( Sub2(_T("V"),_T("c")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      colIdx = 0;
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,colIdx++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      if (scd.ShearInRange)
      {
         (*table)(row,colIdx++) << scalar.SetValue( scd.Beta );
      }
      else
      {
         print_footnote=true;
         (*table)(row,colIdx++) << _T("*");
      }

      (*table)(row,colIdx++) << stress.SetValue( scd.fc );

      if ( concType != pgsTypes::Normal && bHasAggSplittingStrength )
         (*table)(row,colIdx++) << stress.SetValue( scd.fct );

      (*table)(row,colIdx++) << dim.SetValue( scd.bv );
      (*table)(row,colIdx++) << dim.SetValue( scd.dv );

      if (scd.ShearInRange)
      {
         (*table)(row,colIdx++) << shear.SetValue( scd.Vc );
      }
      else
      {
         print_footnote=true;
         (*table)(row,colIdx++) << _T("*");
      }
      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }
}


void write_Vci_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(" - ");

#pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
   CSegmentKey segmentKey = pois.front().GetSegmentKey();


   *pParagraph << _T("Shear Resistance Provided by Concrete when inclined cracking results from combined shear and moment") << rptNewLine;
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   std::_tstring strImage;
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   switch( concType )
   {
   case pgsTypes::Normal:
      strImage = _T("Vci_NWC.png");
      break;

   case pgsTypes::AllLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("Vci_LWC.png");
      else
         strImage = _T("Vci_ALWC.png");
      break;

   case pgsTypes::SandLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("Vci_LWC.png");
      else
         strImage = _T("Vci_SLWC.png");
      break;

   default:
      ATLASSERT(false);
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strImage) << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1) << COLHDR( Sub2(_T("b"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR( Sub2(_T("V"),_T("d")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR( Sub2(_T("V"),_T("i")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << COLHDR( Sub2(_T("M"),_T("max")), rptMomentUnitTag,  pDisplayUnits->GetMomentUnit() );
   (*table)(0,6) << COLHDR( Sub2(_T("M"),_T("cre")), rptMomentUnitTag,  pDisplayUnits->GetMomentUnit() );
   (*table)(0,7) << COLHDR( Sub2(_T("V"),_T("ci min")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,8) << COLHDR( Sub2(_T("V"),_T("ci")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << dim.SetValue( scd.bv );
      (*table)(row,2) << dim.SetValue( scd.dv );
      (*table)(row,3) << shear.SetValue( scd.Vd );
      (*table)(row,4) << shear.SetValue( scd.Vi );
      (*table)(row,5) << moment.SetValue( scd.Mu );
      (*table)(row,6) << moment.SetValue( scd.McrDetails.Mcr );
      (*table)(row,7) << shear.SetValue( scd.VciMin );
      (*table)(row,8) << shear.SetValue( scd.Vci );
      row++;
   }
}

void write_Vcw_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(" - ");

   *pParagraph << _T("Shear Resistance Provided by Concrete when inclined cracking results from excessive principal tension in the web.") << rptNewLine;

#pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
   CSegmentKey segmentKey = pois.front().GetSegmentKey();


   GET_IFACE2(pBroker,IMaterials,pMaterial);
   std::_tstring strImage;
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   switch( concType )
   {
   case pgsTypes::Normal:
      strImage = _T("Vcw_NWC.png");
      break;

   case pgsTypes::AllLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("Vcw_LWC.png");
      else
         strImage = _T("Vcw_ALWC.png");
      break;

   case pgsTypes::SandLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("Vcw_LWC.png");
      else
         strImage = _T("Vcw_SLWC.png");
      break;

   default:
      ATLASSERT(false);
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strImage) << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1) << COLHDR(RPT_STRESS(_T("pc")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(Sub2(_T("b"),_T("v")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR(Sub2(_T("d"),_T("v")),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR(Sub2(_T("V"),_T("p")),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << COLHDR(Sub2(_T("V"),_T("cw")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << stress.SetValue( scd.fpc );
      (*table)(row,2) << dim.SetValue( scd.bv );
      (*table)(row,3) << dim.SetValue( scd.dv );
      (*table)(row,4) << shear.SetValue( scd.Vp );
      (*table)(row,5) << shear.SetValue( scd.Vcw );
      row++;
   }
}

void write_theta_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   *pParagraph << OLE2T(pEventMap->GetLimitStateName(ls)) << _T(" - ");

   *pParagraph << _T("Angle of inclination of diagonal compressive stress [LRFD 5.8.3.3 and 5.8.3.4.3]") << rptNewLine;

#pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
   CSegmentKey segmentKey = pois.front().GetSegmentKey();


   GET_IFACE2(pBroker,IMaterials,pMaterial);
   std::_tstring strImage;
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   switch( concType )
   {
   case pgsTypes::Normal:
      strImage = _T("cotan_theta_NWC.png");
      break;

   case pgsTypes::AllLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("cotan_theta_LWC.png");
      else
         strImage = _T("cotan_theta_ALWC.png");
      break;

   case pgsTypes::SandLightweight:
      if ( bHasAggSplittingStrength )
         strImage = _T("cotan_theta_LWC.png");
      else
         strImage = _T("cotan_theta_SLWC.png");
      break;

   default:
      ATLASSERT(false);
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strImage) << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,1) << COLHDR(Sub2(_T("V"),_T("ci")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2) << COLHDR(Sub2(_T("V"),_T("cw")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3) << COLHDR(RPT_STRESS(_T("pc")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << _T("cot ") << symbol(theta);
   (*table)(0,5) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,0) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,1) << shear.SetValue( scd.Vci );
      (*table)(row,2) << shear.SetValue( scd.Vcw );
      (*table)(row,3) << stress.SetValue( scd.fpc );
      (*table)(row,4) << scalar.SetValue( 1/tan(scd.Theta) );
      (*table)(row,5) << angle.SetValue(scd.Theta);
      row++;
   }
}

void write_Vn_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   CString strName;
   strName.Format(_T("Nominal Shear Resistance - %s"),OLE2T(pEventMap->GetLimitStateName(ls)));

   *pChapter << pParagraph;

   CollectionIndexType nCol = (shear_capacity_method == scmVciVcw ? 11 : 12);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol,strName);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   CollectionIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("b"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( shear_capacity_method != scmVciVcw )
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("p")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   if ( shear_capacity_method == scmVciVcw )
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")) << Super(_T("&")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("s")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("n1")) << Super(_T("$")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("n2")) << Super(_T("#")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("n")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR( symbol(phi) << Sub2(_T("V"),_T("n")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(5);
   scalar.SetPrecision(2);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,col++) << stress.SetValue( scd.fc );
      (*table)(row,col++) << dim.SetValue( scd.bv );
      (*table)(row,col++) << dim.SetValue( scd.dv );

      if ( shear_capacity_method != scmVciVcw )
      {
         (*table)(row,col++) << shear.SetValue( scd.Vp );
      }

      if (scd.ShearInRange)
      {
         (*table)(row,col++) << shear.SetValue( scd.Vc );
         (*table)(row,col++) << shear.SetValue( scd.Vs );
         (*table)(row,col++) << shear.SetValue( scd.Vn1 );
      }
      else
      {
         print_footnote=true;
         (*table)(row,col++) << _T("*");
         (*table)(row,col++) << _T("*");
         (*table)(row,col++) << _T("*");
      }

      (*table)(row,col++) << shear.SetValue( scd.Vn2 );
      (*table)(row,col++) << shear.SetValue( scd.Vn );
      (*table)(row,col++) << scalar.SetValue( scd.Phi );
      (*table)(row,col++) << shear.SetValue( scd.pVn );

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << Super(_T("&")) << Sub2(_T("V"),_T("c")) << _T(" = ") << _T("min(") << Sub2(_T("V"),_T("ci")) << _T(",") << Sub2(_T("V"),_T("cw")) << _T(")") << rptNewLine;
   }

   *pParagraph << Super(_T("$")) << Sub2(_T("V"),_T("n1")) << _T(" = ") << Sub2(_T("V"),_T("c")) << _T(" + ") << Sub2(_T("V"),_T("s"));

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << _T(" [Eqn 5.8.3.3-1 with ") << Sub2(_T("V"),_T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(" [Eqn 5.8.3.3-1]")<< rptNewLine;
   }


   *pParagraph << Super(_T("#")) << Sub2(_T("V"),_T("n2")) << _T(" = ") << _T("0.25") << RPT_FC << Sub2(_T("b"),_T("v")) << Sub2(_T("d"),_T("v"));
   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << _T(" [Eqn 5.8.3.3-2 with ") << Sub2(_T("V"),_T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(" [Eqn 5.8.3.3-2]")<< rptNewLine;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded.")<< rptNewLine<<rptNewLine;
}

void write_Avs_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   CString strLabel;
   strLabel.Format(_T("Required Shear Reinforcement - %s"),OLE2T(pEventMap->GetLimitStateName(ls)));

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pParagraph << strLabel << rptNewLine;
   *pChapter << pParagraph;

   if ( shear_capacity_method != scmVciVcw )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("RequiredShearReinforcement1.png"));
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("RequiredShearReinforcement2.png"));

   CollectionIndexType nCol = (shear_capacity_method == scmVciVcw ? 8 : 9);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   CollectionIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("u")) << _T("/") << symbol(phi), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   if ( shear_capacity_method != scmVciVcw )
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("p")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("s")) << _T(" *"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( RPT_FY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( symbol(theta), rptAngleUnitTag,  pDisplayUnits->GetAngleUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );


   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  angle,    pDisplayUnits->GetAngleUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs,      pDisplayUnits->GetAvOverSUnit(),      false );

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );
      (*table)(row,col++) << shear.SetValue( scd.Vu/scd.Phi );
      (*table)(row,col++) << shear.SetValue( scd.Vc );
      
      if ( shear_capacity_method != scmVciVcw )
         (*table)(row,col++) << shear.SetValue( scd.Vp );
      
      (*table)(row,col++) << shear.SetValue( scd.VsReqd );
      (*table)(row,col++) << stress.SetValue( scd.fy );
      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << angle.SetValue( scd.Theta );
      (*table)(row,col++) << avs.SetValue( scd.AvOverS_Reqd );

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << _T("* - Transverse reinforcement not required if ") << Sub2(_T("V"),_T("u")) << _T(" < 0.5") << symbol(phi) << Sub2(_T("V"),_T("c"));
      *pParagraph << _T(" [Eqn 5.8.2.4-1 with ") << Sub2(_T("V"),_T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph << _T("* - Transverse reinforcement not required if ") << Sub2(_T("V"),_T("u")) << _T(" < 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"),_T("c"));
      *pParagraph  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(") [Eqn 5.8.2.4-1]")<< rptNewLine;
   }
}

void write_bar_spacing_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   CString strLabel;
   strLabel.Format(_T("Required Stirrup Spacing - %s"),OLE2T(pEventMap->GetLimitStateName(ls)));

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pParagraph << strLabel << rptNewLine;
   *pChapter << pParagraph;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( _T("Provided") << rptNewLine << Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,col++) << COLHDR( _T("Required") << rptNewLine << Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );


   Float64 nLegs = 2.0;

#pragma Reminder("UPDATE: assuming all segments are the same") // getting data for the first segment
   CSegmentKey segmentKey = pois.front().GetSegmentKey();

   GET_IFACE2(pBroker,IShear,pShear);
   const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);

   if ( 0 < pShearData->ShearZones.size() )
   {
      nLegs = pShearData->ShearZones[0].nVertBars;
   }

   std::_tostringstream os3;
   os3 << nLegs << _T("-#3");

   (*table)(0,col++) << COLHDR( os3.str() << rptNewLine << _T("S"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   std::_tostringstream os4;
   os4 << nLegs << _T("-#4");

   (*table)(0,col++) << COLHDR( os4.str() << rptNewLine << _T("S"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   std::_tostringstream os5;
   os5 << nLegs << _T("-#5");

   (*table)(0,col++) << COLHDR( os5.str() << rptNewLine << _T("S"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );


   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spacing,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs,      pDisplayUnits->GetAvOverSUnit(),      false );

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   lrfdRebarPool* pRebarPool = lrfdRebarPool::GetInstance();

   Float64 Ab3 = pRebarPool->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,matRebar::bs3)->GetNominalArea();
   Float64 Ab4 = pRebarPool->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,matRebar::bs4)->GetNominalArea();
   Float64 Ab5 = pRebarPool->GetRebar(pShearData->ShearBarType,pShearData->ShearBarGrade,matRebar::bs5)->GetNominalArea();

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(pois.front().GetSegmentKey());

   RowIndexType row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(pois.end());
   for ( ; i != end; i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;

      // Don't print pois outside of CSS's
      if (IsPoiInEndRegion(poi))
         continue;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,&scd);

      (*table)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

      if(0.0 < scd.Av && 0.0 < scd.S)
         (*table)(row,col++) << avs.SetValue( scd.Av/scd.S);
      else
         (*table)(row,col++) << _T("---");

      (*table)(row,col++) << avs.SetValue( scd.AvOverS_Reqd );

      if ( !IsZero(scd.AvOverS_Reqd) )
      {
         Float64 S3 = nLegs*Ab3/scd.AvOverS_Reqd;
         Float64 S4 = nLegs*Ab4/scd.AvOverS_Reqd;
         Float64 S5 = nLegs*Ab5/scd.AvOverS_Reqd;

         (*table)(row,col++) << spacing.SetValue(S3);
         (*table)(row,col++) << spacing.SetValue(S4);
         (*table)(row,col++) << spacing.SetValue(S5);
      }
      else
      {
         (*table)(row,col++) << RPT_NA;
         (*table)(row,col++) << RPT_NA;
         (*table)(row,col++) << RPT_NA;
      }

      row++;
   }
}
