///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
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
                                const PoiList& vPoi,
                                rptChapter* pChapter,
                                IntervalIndexType intervalIdx,
                                const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_shear_stress_table(  IBroker* pBroker,
                                IEAFDisplayUnits* pDisplayUnits,
                                const PoiList& vPoi,
                                rptChapter* pChapter,
                                IntervalIndexType intervalIdx,
                                const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpo_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const PoiList& vPoi,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpc_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_fpce_table(IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      const PoiList& vPoi,
                      rptChapter* pChapter,
                      IntervalIndexType intervalIdx,
                      const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Fe_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_btsummary_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const PoiList& vPoi,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC);

void write_ex_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls, bool bUHPC);

void write_Vs_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_theta_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const PoiList& vPoi,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vc_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);


void write_Vf_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName, pgsTypes::LimitState ls);

void write_Vci_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vcw_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls);

void write_Vn_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC);

void write_Avs_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls, bool bUHPC);

void write_bar_spacing_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
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
      if (!pRatingSpec->IsRatingEnabled())
      {
         bRating = false;
      }
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   GET_IFACE2(pBroker, IMaterials, pMaterial);

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;
   std::_tstring stage_name(pIntervals->GetDescription(intervalIdx));

   GET_IFACE2(pBroker,IBridge,pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;

      if (vGirderKeys.size() > 1)
      {
         // report where we are if more than one girder in report
         *pPara << pgsGirderLabel::GetGirderLabel(thisGirderKey) << rptNewLine;
      }

      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(thisGirderKey);

      // vBasicPoi does not contain CS for shear POI
      PoiList vBasicPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(thisGirderKey, ALL_SEGMENTS), &vBasicPoi);;
      pPoi->RemovePointsOfInterest(vBasicPoi,POI_BOUNDARY_PIER);

      std::vector<pgsTypes::LimitState> vLimitStates;
      if ( bDesign )
      {
         vLimitStates.push_back(pgsTypes::StrengthI);
         if ( bPermit )
         {
            vLimitStates.push_back(pgsTypes::StrengthII);
         }
      }

      if ( bRating )
      {
         GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Inventory) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_Inventory);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) && pRatingSpec->RateForShear(pgsTypes::lrDesign_Operating) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_Operating);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Routine) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalRoutine);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Special) )
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalSpecial);
         }

         if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency) && pRatingSpec->RateForShear(pgsTypes::lrLegal_Emergency))
         {
            vLimitStates.push_back(pgsTypes::StrengthI_LegalEmergency);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Routine) )
         {
            vLimitStates.push_back(pgsTypes::StrengthII_PermitRoutine);
         }

         if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) && pRatingSpec->RateForShear(pgsTypes::lrPermit_Special) )
         {
            vLimitStates.push_back(pgsTypes::StrengthII_PermitSpecial);
         }
      }

      for (const auto& ls : vLimitStates)
      {
         PoiList vCSPoi;
         pPoi->GetCriticalSections(ls, thisGirderKey,&vCSPoi);
         PoiList vPoi;
         pPoi->MergePoiLists(vBasicPoi, vCSPoi,&vPoi); // merge, sort, and remove duplicates

         pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(vPoi.front().get().GetSegmentKey());
         bool bUHPC = (concType == pgsTypes::UHPC) ? true : false;

         if (1 < vLimitStates.size())
         {
            rptParagraph* pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pParagraph;
            pParagraph->SetName(GetLimitStateString(ls));
         }

         write_shear_dimensions_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);

         if ( shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001 )
         {
            write_shear_stress_table    (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_fpc_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_fpo_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_Fe_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_ex_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls, bUHPC);
            write_btsummary_table       (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls, bUHPC);
            write_Vc_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            if (bUHPC)
            {
               write_Vf_table(pBroker, pDisplayUnits, vPoi, pChapter, intervalIdx, stage_name, ls);
            }
            write_Vs_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
         }
         else if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
         {
            write_shear_stress_table    (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_fpo_table             (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            write_ex_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls, bUHPC);
            write_btsummary_table       (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls, bUHPC);
            write_Vc_table              (pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls);
            if (bUHPC)
            {
               write_Vf_table(pBroker, pDisplayUnits, vPoi, pChapter, intervalIdx, stage_name, ls);
            }
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
            if (bUHPC)
            {
               write_Vf_table(pBroker, pDisplayUnits, vPoi, pChapter, intervalIdx, stage_name, ls);
            }
         }

         write_Vn_table(pBroker, pDisplayUnits, vPoi,  pChapter, intervalIdx, stage_name, ls, bUHPC);

         write_Avs_table(pBroker, pDisplayUnits, vPoi, pChapter, intervalIdx, stage_name, ls, bUHPC);
         write_bar_spacing_table(pBroker, pDisplayUnits, vPoi, pChapter, intervalIdx, stage_name, ls);
      }
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
                             const PoiList& vPoi,
                             rptChapter* pChapter,
                             IntervalIndexType intervalIdx,
                             const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Effective Shear Dimensions for ") << GetLimitStateString(ls) << _T(" [From Article ") << LrfdCw8th(_T("5.8.2.7"),_T("5.7.2.6")) << _T("]") << rptNewLine;
   
   CGirderKey girderKey = vPoi.front().get().GetSegmentKey();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   std::_tstring strPicture = pFactory->GetShearDimensionsSchematicImage(deckType);
   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strPicture);

   *pParagraph << rptNewLine;
   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("dv.png")) << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(9);
   *pParagraph << table << rptNewLine;


   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi,nullptr,&scd);

      (*table)(row,0) << location.SetValue( POI_SPAN, poi );
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
                              const PoiList& vPoi,
                              rptChapter* pChapter,
                              IntervalIndexType intervalIdx,
                              const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   std::_tstring strEquation;
   switch( lrfdVersionMgr::GetVersion() )
   {
      case lrfdVersionMgr::FirstEdition1994:
      case lrfdVersionMgr::FirstEditionWith1996Interims:
      case lrfdVersionMgr::FirstEditionWith1997Interims:
      case lrfdVersionMgr::SecondEdition1998:
      case lrfdVersionMgr::SecondEditionWith1999Interims:
         strEquation = std::_tstring(_T(" [Eqn ")) + LrfdCw8th(_T("5.8.3.4.2-1"),_T("5.7.3.4.2-1")) + _T("]");
         break;

      case lrfdVersionMgr::SecondEditionWith2000Interims:
      case lrfdVersionMgr::SecondEditionWith2001Interims:
      case lrfdVersionMgr::SecondEditionWith2002Interims:
      case lrfdVersionMgr::SecondEditionWith2003Interims:
      case lrfdVersionMgr::ThirdEdition2004:
         strEquation = std::_tstring(_T(" [Eqn ")) + LrfdCw8th(_T("5.8.2.9-1"),_T("5.7.2.8-1")) + _T("]");
         break;

      case lrfdVersionMgr::ThirdEditionWith2005Interims:
      case lrfdVersionMgr::ThirdEditionWith2006Interims:
         strEquation = std::_tstring(_T(" [Eqn ")) + LrfdCw8th(_T("5.8.2.4-1"),_T("5.7.2.3-1")) + _T("]");
         break;

      case lrfdVersionMgr::FourthEdition2007:
      case lrfdVersionMgr::FourthEditionWith2008Interims:
      case lrfdVersionMgr::FourthEditionWith2009Interims:
      default:
         strEquation =  std::_tstring(_T(" [Eqn ")) + LrfdCw8th(_T("5.8.2.9-1"),_T("5.7.2.8-1")) + _T("]");
         break;
   }

   *pParagraph << Italic(_T("v")) << strEquation << rptNewLine;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("vu.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("vu_2005.png")) << rptNewLine;
   }

   *pParagraph << rptNewLine;

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker,IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   ColumnIndexType nColumns = 7;
   if (0 < (nMaxSegmentDucts + nGirderDucts))
   {
      nColumns++;
   }

   if (0 < nMaxSegmentDucts)
   {
      nColumns++;
   }

   if (0 < nGirderDucts)
   {
      nColumns++;
   }

   CString strTitle;
   strTitle.Format(_T("Factored Shear Stresses for %s"),GetLimitStateString(ls));
   rptRcTable* table = rptStyleManager::CreateDefaultTable(nColumns,strTitle);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   if (0 < (nMaxSegmentDucts + nGirderDucts))
   {
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      *pParagraph << Sub2(_T("V"), _T("ps")) << _T(" = Vertical Component of Prestress from Strands") << rptNewLine;
      if (0 < nMaxSegmentDucts)
      {
         *pParagraph << Sub2(_T("V"), _T("pts")) << _T(" = Vertical Component of Prestress from Segment Tendons") << rptNewLine;
      }

      if (0 < nGirderDucts)
      {
         *pParagraph << Sub2(_T("V"), _T("ptg")) << _T(" = Vertical Component of Prestress from Girder Tendons") << rptNewLine;
      }
   }


   ColumnIndexType col = 0;
   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << symbol(phi);
   (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if (0 < (nMaxSegmentDucts + nGirderDucts))
   {
      (*table)(0, col++) << COLHDR(_T("V") << Sub(_T("ps")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   }
   
   if (0 < nMaxSegmentDucts)
   {
      (*table)(0, col++) << COLHDR(_T("V") << Sub(_T("pts")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   }

   if ( 0 < nGirderDucts )
   {
      (*table)(0,col++) << COLHDR(_T("V") << Sub(_T("ptg")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
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

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << scd.Phi;
      (*table)(row,col++) << force.SetValue( scd.Vu );

      if (0 < (nMaxSegmentDucts + nGirderDucts))
      {
         (*table)(row, col++) << force.SetValue(scd.Vps);
      }

      if (0 < nMaxSegmentDucts)
      {
         (*table)(row, col++) << force.SetValue(scd.VptSegment);
      }

      if (0 < nGirderDucts)
      {
         (*table)(row, col++) << force.SetValue(scd.VptGirder);
      }

      (*table)(row,col++) << force.SetValue( scd.Vp );
      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << dim.SetValue( scd.bv );
      (*table)(row,col++) << stress.SetValue( (scd.vu) );

      row++;
   }
}

void write_fpc_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   if ( bAfter1999 && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001 ))
   {
      return;
   }

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << RPT_STRESS(_T("pc")) << _T(" [for use in Eqn 5.8.3.4.3-3] - ") << GetLimitStateString(ls) << rptNewLine;
   }
   else
   {
      *pParagraph << RPT_STRESS(_T("pc")) << _T(" [for use in Eqn ") << LrfdCw8th(_T("C5.8.3.4.2-1"),_T("C5.7.3.4.2-1")) << _T("] - ") << GetLimitStateString(ls) << rptNewLine;
   }

   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Fpc Pic.jpg")) << rptNewLine;
   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("fpc_2007.png")) << rptNewLine;

   *pParagraph << rptNewLine;

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi,&vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

      ColumnIndexType nCols = 8;
      if (0 < nMaxSegmentDucts)
      {
         nCols += 2;
      }
      
      if (0 < nGirderDucts)
      {
         nCols += 2;
      }

      if ( 1 < vGirderKeys.size() )
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols);

      //if ( segmentKey.groupIndex == ALL_GROUPS )
      //{
      //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      //}

      *pParagraph << table << rptNewLine;

      ColumnIndexType col = 0;

      (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      if (nMaxSegmentDucts == 0 && nGirderDucts == 0 )
      {
         (*table)(0,col++) << COLHDR(_T("e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0,col++) << COLHDR(_T("P"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
      }
      else
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("ps")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*table)(0,col++) << COLHDR(Sub2(_T("P"),_T("ps")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );

         if (0 < nMaxSegmentDucts)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("pts")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("P"), _T("pts")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
         }

         if (0 < nGirderDucts)
         {
            (*table)(0, col++) << COLHDR(Sub2(_T("e"), _T("ptg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
            (*table)(0, col++) << COLHDR(Sub2(_T("P"), _T("ptg")), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
         }
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

      RowIndexType row = table->GetNumberOfHeaderRows();

      GET_IFACE2(pBroker,IShearCapacity,pShearCap);

      for (const pgsPointOfInterest& poi : vPoi)
      {
         col = 0;

         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         if ( CGirderKey(segmentKey) != girderKey )
         {
            continue;
         }

         // Don't print poi that are inside of a CSS zone
         if (pPoi->IsInCriticalSectionZone(poi,ls))
         {
            continue;
         }

         Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

         FPCDETAILS fpcd;
         pShearCap->GetFpcDetails(poi, nullptr, &fpcd);

         (*table)(row,col++) << location.SetValue(POI_SPAN, poi );
         if (nMaxSegmentDucts == 0 && nGirderDucts == 0)
         {
            (*table)(row,col++) << dim.SetValue( fpcd.eps );
            (*table)(row,col++) << force.SetValue( fpcd.Pps );
         }
         else
         {
            (*table)(row,col++) << dim.SetValue( fpcd.eps );
            (*table)(row,col++) << force.SetValue( fpcd.Pps );

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << dim.SetValue(fpcd.eptSegment);
               (*table)(row, col++) << force.SetValue(fpcd.PptSegment);
            }

            if (0 < nGirderDucts)
            {
               (*table)(row, col++) << dim.SetValue(fpcd.eptGirder);
               (*table)(row, col++) << force.SetValue(fpcd.PptGirder);
            }
         }
         (*table)(row,col++) << area.SetValue( fpcd.Ag );
         (*table)(row,col++) << inertia.SetValue( fpcd.Ig );
         (*table)(row,col++) << dim.SetValue( fpcd.c );
         (*table)(row,col++) << moment.SetValue( fpcd.Mg );
         (*table)(row,col++) << stress.SetValue( fpcd.fpc );

         row++;
      }
   }
}

void write_fpce_table(IBroker* pBroker,
                      IEAFDisplayUnits* pDisplayUnits,
                      const PoiList& vPoi,
                      rptChapter* pChapter,
                      IntervalIndexType intervalIdx,
                      const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << Sub2(_T("M"),_T("cre")) << GetLimitStateString(ls) << _T(" [Eqn 5.8.3.4.3-2]") << rptNewLine;

   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Mcre.png")) << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(7);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();

   std::set<CSegmentKey> segmentKeys; // keep track of the segments that we reported on

   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      segmentKeys.insert(poi.GetSegmentKey());

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,0) << location.SetValue( POI_SPAN, poi );
      (*table)(row,1) << stress.SetValue( scd.McrDetails.fr );
      (*table)(row,2) << stress.SetValue( scd.McrDetails.fcpe);
      (*table)(row,3) << sect_mod.SetValue( scd.McrDetails.Sb );
      (*table)(row,4) << sect_mod.SetValue( scd.McrDetails.Sbc );
      (*table)(row,5) << moment.SetValue( scd.McrDetails.Mdnc);
      (*table)(row,6) << moment.SetValue( scd.McrDetails.Mcr );


      row++;
   }

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   std::set<CSegmentKey>::iterator iter(segmentKeys.begin());
   std::set<CSegmentKey>::iterator endIter(segmentKeys.end());
   for ( ; iter != endIter; iter++ )
   {
      CSegmentKey segmentKey(*iter);
      if ( segmentKeys.size() < 2 )
      {
         *pParagraph << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetSegmentShearFrCoefficient(segmentKey));
         if ( bLambda )
         {
            *pParagraph << symbol(lambda);
         }
         *pParagraph << symbol(ROOT) << RPT_FC << rptNewLine;
      }
      else
      {
         *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(": ") << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetSegmentShearFrCoefficient(segmentKey));
         if ( bLambda )
         {
            *pParagraph << symbol(lambda);
         }
         *pParagraph << symbol(ROOT) << RPT_FC << rptNewLine;

         *pParagraph << _T("Closure Joint ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(": ") << RPT_STRESS(_T("r")) << _T(" = ") << fr_coefficient.SetValue(pMaterial->GetClosureJointShearFrCoefficient(segmentKey));
         if (bLambda)
         {
            *pParagraph << symbol(lambda);
         }
         *pParagraph << symbol(ROOT) << RPT_FC << rptNewLine;
      }
   }

   *pParagraph << RPT_STRESS(_T("cpe")) << _T(" = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads.") << rptNewLine;
   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("fcpe.png")) << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("nc")) << _T(" = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("S"),_T("c")) << _T(" = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads") << rptNewLine;
   *pParagraph << Sub2(_T("M"),_T("dnc")) << _T(" = total unfactored dead load moment acting on the monolithic or noncomposite section") << rptNewLine;
}

void write_fpo_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << RPT_STRESS(_T("po"));

   if ( !bAfter1999 )
   {
      *pParagraph << _T(" [Eqn ") << LrfdCw8th(_T("C5.8.3.4.2-1"),_T("C5.7.3.4.2-1")) << _T("]");
   }

   *pParagraph << _T(" - ") << GetLimitStateString(ls) << rptNewLine;

   pParagraph = new rptParagraph();
   *pChapter << pParagraph;

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    true );

   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi,&vGirderKeys);
   if ( bAfter1999 )
   {
      // See PCI BDM 8.4.1.1.4
      Float64 Kps, Kpt;

      GET_IFACE2(pBroker,IMaterials,pMaterial);
      for (const auto& girderKey : vGirderKeys)
      {
         DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);
         DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

         if ( 1 < vGirderKeys.size() )
         {
            *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
         }

         std::vector<CSegmentKey> vSegmentKeys;
         pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);

         for (const auto& segmentKey : vSegmentKeys)
         {
            if ( 1 < vSegmentKeys.size() )
            {
               *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << rptNewLine;
            }


            if (0 < (nMaxSegmentDucts + nGirderDucts))
            {
               *pParagraph << _T("Strands") << rptNewLine;
            }
            const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Permanent);
            Kps = 0.70;

            if (0 < (nMaxSegmentDucts + nGirderDucts))
            {
               *pParagraph << RPT_STRESS(_T("po ps")) << _T(" = 0.70") << RPT_FPU;
            }
            else
            {
               *pParagraph << RPT_STRESS(_T("po")) << _T(" = 0.70") << RPT_FPU;
            }

            *pParagraph << _T(" = ") << stress.SetValue(Kps*pStrand->GetUltimateStrength()) << rptNewLine;
            
            if (0 < nMaxSegmentDucts)
            {
               *pParagraph << _T("Segment Tendons") << rptNewLine;
               const matPsStrand* pTendon = pMaterial->GetSegmentTendonMaterial(segmentKey);
               Kpt = 0.70;
               *pParagraph << RPT_STRESS(_T("po pts")) << _T(" = 0.70") << RPT_FPU;
               *pParagraph << _T(" = ") << stress.SetValue(Kpt*pTendon->GetUltimateStrength()) << rptNewLine;
            }

            *pParagraph << rptNewLine;
         }

         if ( 0 < nGirderDucts )
         {
            *pParagraph << _T("Girder Tendons") << rptNewLine;
            const matPsStrand* pTendon = pMaterial->GetGirderTendonMaterial(girderKey);
            Kpt = 0.70;
            *pParagraph << italic(ON) << Sub2(_T("f"),_T("po ptg")) << _T(" = 0.70") << Sub2(_T("f"),_T("pu")) << italic(OFF);
            *pParagraph << _T(" = ") << stress.SetValue(Kpt*pTendon->GetUltimateStrength()) << rptNewLine;
         }
      }
   }

   *pParagraph << rptNewLine;

   if ( !bAfter1999 )
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("fpo.png")) << rptNewLine;

      for (const auto& girderKey : vGirderKeys)
      {
         DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);
         DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

         ColumnIndexType nCols = 6;
         if (0 < nMaxSegmentDucts)
         {
            nCols += 3;
         }

         if (0 < nGirderDucts)
         {
            nCols += 3;
         }

         rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols);

         if ( 1 < vGirderKeys.size() )
         {
            *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
         }

         *pParagraph << table << rptNewLine;

         ColumnIndexType col = 0;
         (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         if ( 0 == nMaxSegmentDucts+nGirderDucts )
         {
            (*table)(0,col++) << COLHDR( RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         else
         {
            (*table)(0, col++) << COLHDR(italic(ON) << Sub2(_T("f"), _T("pe ps")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());

            if (nMaxSegmentDucts)
            {
               (*table)(0, col++) << COLHDR(italic(ON) << Sub2(_T("f"), _T("pe pts")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            }

            if (nGirderDucts)
            {
               (*table)(0, col++) << COLHDR(italic(ON) << Sub2(_T("f"), _T("pe ptg")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            }
         }

         (*table)(0,col++) << COLHDR( RPT_FPC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         if ( 0 == (nMaxSegmentDucts + nGirderDucts))
         {
            (*table)(0,col++) << COLHDR( RPT_EP,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
         }
         else
         {
            (*table)(0,col++) << COLHDR( Sub2(_T("E"),_T("ps")),  rptStressUnitTag, pDisplayUnits->GetModEUnit() );

            if (nMaxSegmentDucts)
            {
               (*table)(0, col++) << COLHDR(Sub2(_T("E"), _T("pts")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
            }

            if (nGirderDucts)
            {
               (*table)(0, col++) << COLHDR(Sub2(_T("E"), _T("ptg")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
            }
         }
         
         (*table)(0,col++) << COLHDR( RPT_EC,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );

         if ( 0 == (nMaxSegmentDucts + nGirderDucts))
         {
            (*table)(0,col++) << COLHDR( RPT_FPO, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         else
         {
            (*table)(0,col++) << COLHDR( italic(ON) << Sub2(_T("f"),_T("po ps")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

            if (nMaxSegmentDucts)
            {
               (*table)(0, col++) << COLHDR(italic(ON) << Sub2(_T("f"), _T("po pts")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            }

            if (nGirderDucts)
            {
               (*table)(0, col++) << COLHDR(italic(ON) << Sub2(_T("f"), _T("po ptg")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit());
            }
         }

         INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),false );
         INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    false );
         INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),      false );

         //location.IncludeSpanAndGirder(span == ALL_SPANS);

         GET_IFACE2(pBroker,IBridge,pBridge);
         Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

         RowIndexType row = table->GetNumberOfHeaderRows();
      
         GET_IFACE2(pBroker,IShearCapacity,pShearCap);
         for (const pgsPointOfInterest& poi : vPoi)
         {
            col = 0;

            if ( CGirderKey(poi.GetSegmentKey()) != girderKey )
            {
               continue;
            }

            // Don't print poi that are inside of a CSS zone
            if (pPoi->IsInCriticalSectionZone(poi,ls))
            {
               continue;
            }

            SHEARCAPACITYDETAILS scd;
            pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

            (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
            (*table)(row,col++) << stress.SetValue( scd.fpeps );

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << stress.SetValue(scd.fpeptSegment);
            }

            if (0 < nGirderDucts)
            {
               (*table)(row, col++) << stress.SetValue(scd.fpeptGirder);
            }

            (*table)(row,col++) << stress.SetValue( scd.fpc );
            (*table)(row,col++) << mod_e.SetValue( scd.Eps );

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << mod_e.SetValue(scd.EptSegment);
            }

            if (0 < nGirderDucts)
            {
               (*table)(row, col++) << mod_e.SetValue(scd.EptGirder);
            }

            (*table)(row,col++) << mod_e.SetValue( scd.Ec );
            (*table)(row,col++) << stress.SetValue( scd.fpops );

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << stress.SetValue(scd.fpoptSegment);
            }

            if (0 < nGirderDucts)
            {
               (*table)(row, col++) << stress.SetValue(scd.fpoptGirder);
            }

            row++;
         } // next poi
      } // next girder
   } // end if
}

void write_Fe_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType())
   {
      return; // This is not applicable 2000 and later
   }

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << Sub2(_T("F"),symbol(epsilon)) << _T(" [Eqn ") << LrfdCw8th(_T("5.8.3.4.2-3"),_T("5.7.3.4.2-3")) << _T("] - ") << GetLimitStateString(ls) << rptNewLine;

   *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Fe.png")) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << _T("This calculation is applicable only when ") << symbol(epsilon) << Sub(_T("x")) << _T(" is negative.") << rptNewLine;
   *pParagraph << rptNewLine;

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(pBroker,IGirderTendonGeometry,pGirderTendonGeometry);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker, IBridge, pBridge);

   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi,&vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if (1 < vGirderKeys.size())
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);;
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

      ColumnIndexType nCols = 8;
      if (0 < nMaxSegmentDucts)
      {
         nCols++;
      }

      if (0 < nGirderDucts)
      {
         nCols++;
      }
      
      rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols);

      *pParagraph << table << rptNewLine;

      ColumnIndexType col = 0;

      (*table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      (*table)(0,col++) << COLHDR( RPT_ES, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,col++) << COLHDR( RPT_AS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,col++) << COLHDR( RPT_EPS, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,col++) << COLHDR( RPT_APS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );

      if (0 < nMaxSegmentDucts)
      {
         (*table)(0, col++) << COLHDR(RPT_EPT << Sub(_T("s")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
         (*table)(0, col++) << COLHDR(RPT_APT << Sub(_T("s")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit());
      }

      if ( 0 < nGirderDucts )
      {
         (*table)(0,col++) << COLHDR( RPT_EPT << Sub(_T("g")), rptStressUnitTag, pDisplayUnits->GetModEUnit() );
         (*table)(0,col++) << COLHDR( RPT_APT << Sub(_T("g")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      }

      (*table)(0,col++) << COLHDR( RPT_EC, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,col++) << COLHDR( RPT_AC, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
      (*table)(0,col++) << _T("F") << Sub(symbol(epsilon));

      INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),            false );
      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,     pDisplayUnits->GetAreaUnit(),            false );

      //location.IncludeSpanAndGirder(span == ALL_SPANS);

      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());


      RowIndexType row = table->GetNumberOfHeaderRows();
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);
      GET_IFACE2(pBroker,IShearCapacity,pShearCap);
      for (const pgsPointOfInterest& poi : vPoi)
      {
         const CSegmentKey& segmentKey(poi.GetSegmentKey());

         if ( CGirderKey(segmentKey) != girderKey )
         {
            continue;
         }

         // Don't print poi that are inside of a CSS zone
         if (pPoi->IsInCriticalSectionZone(poi,ls))
         {
            continue;
         }

         col = 0;

         Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

         SHEARCAPACITYDETAILS scd;
         pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

         (*table)(row,col++) << location.SetValue(POI_SPAN, poi );

         if ( scd.Fe < 0 )
         {
            (*table)(row,col++) << _T("-");
            (*table)(row,col++) << _T("-");
            (*table)(row,col++) << _T("-"); 
            (*table)(row,col++) << _T("-");

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << _T("-");
               (*table)(row, col++) << _T("-");
            }

            if ( 0 < nGirderDucts )
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

            if (0 < nMaxSegmentDucts)
            {
               (*table)(row, col++) << mod_e.SetValue(scd.EptSegment);
               (*table)(row, col++) << area.SetValue(scd.AptSegment);
            }

            if ( 0 < nGirderDucts )
            {
               (*table)(row,col++) << mod_e.SetValue( scd.EptGirder );
               (*table)(row,col++) << area.SetValue( scd.AptGirder );
            }
            (*table)(row,col++) << mod_e.SetValue( scd.Ec );
            (*table)(row,col++) << area.SetValue( scd.Ac );
            (*table)(row,col++) << scalar.SetValue(scd.Fe);
         }

         row++;
      }
   }
}

void write_ex_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   auto specType = pSpecEntry->GetSpecificationType();
   bool bAfter1999 = ( lrfdVersionMgr::SecondEditionWith2000Interims  <= specType ? true : false );
   bool bAfter2003 = ( lrfdVersionMgr::SecondEditionWith2002Interims  <= specType ? true : false );
   bool bAfter2004 = ( lrfdVersionMgr::ThirdEditionWith2005Interims   <= specType ? true : false );
   bool bAfter2007 = ( lrfdVersionMgr::FourthEditionWith2008Interims  <= specType ? true : false );
   bool bAfter2009 = ( lrfdVersionMgr::FifthEdition2010               <= specType ? true : false );
   bool bAfter2016 = ( lrfdVersionMgr::SeventhEditionWith2016Interims <= specType ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();


   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   GET_IFACE2(pBroker, ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(pBroker, IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Longitudinal Strain ") << Sub2(symbol(epsilon),_T("x"));
   if ( !bAfter1999 )
   {
      *pParagraph << _T(" [Eqn 5.8.3.4.2-2] ");
   }

   *pParagraph << _T("- ") << GetLimitStateString(ls) << rptNewLine;

   if ( bAfter2007 )
   {
      if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2008.png")) << rptNewLine;
      }
      else if ( shear_capacity_method == scmWSDOT2001 )
      {
         // tables with WSDOT modifications
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2003_WSDOT.png")) << rptNewLine;
      }
      else
      {
         // tables
         if (bAfter2016)
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2017.png")) << rptNewLine;
         }
         else
         {
            *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2005.png")) << rptNewLine;
         }
      }
   }
   else if ( bAfter2004 )
   {
      if ( shear_capacity_method == scmWSDOT2007 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2008.png")) << rptNewLine;
      }
      else if ( shear_capacity_method == scmWSDOT2001 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2003_WSDOT.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2005.png")) << rptNewLine;
      }
   }
   else if ( bAfter2003 )
   {
      ATLASSERT(shear_capacity_method != scmWSDOT2007);

      if ( shear_capacity_method == scmWSDOT2001 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2003_WSDOT.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2003.png")) << rptNewLine;
      }
   }
   else if ( bAfter1999 )
   {
      if ( shear_capacity_method == scmWSDOT2001 )
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2000_WSDOT.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex_2000.png")) << rptNewLine;
      }
   }
   else
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ex.png")) << rptNewLine; // 1999 and earlier
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if (bUHPC || pSpecEntry->LimitNetTensionStrainToPositiveValues())
   {
      *pParagraph << _T("When the computed strain is negative, it is taken as zero (LRFD 5.7.3.4.1)") << rptNewLine;
   }

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

   if (0 < (nMaxSegmentDucts + nGirderDucts))
   {
      nCol++;
   }

   if (0 < nMaxSegmentDucts)
   {
      nCol += 3;
   }

   if (0 < nGirderDucts)
   {
      nCol += 3;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   ColumnIndexType col = 0;

   *pParagraph << table << rptNewLine;

   if (0 < (nMaxSegmentDucts + nGirderDucts))
   {
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      *pParagraph << Sub2(_T("V"), _T("ps")) << _T(" = Vertical Component of Prestress from Strands") << rptNewLine;
      if (0 < nMaxSegmentDucts)
      {
         *pParagraph << Sub2(_T("V"), _T("pts")) << _T(" = Vertical Component of Prestress from Segment Tendons") << rptNewLine;
         *pParagraph << Sub2(_T("A"), _T("pts")) << _T(" = Area of Segment Tendons") << rptNewLine;
         *pParagraph << Sub2(_T("E"), _T("pts")) << _T(" = Modulus of Elasticity of Segment Tendons") << rptNewLine;
      }

      if (0 < nGirderDucts)
      {
         *pParagraph << Sub2(_T("V"), _T("ptg")) << _T(" = Vertical Component of Prestress from Girder Tendons") << rptNewLine;
         *pParagraph << Sub2(_T("A"), _T("ptg")) << _T(" = Area of Girder Tendons") << rptNewLine;
         *pParagraph << Sub2(_T("E"), _T("ptg")) << _T(" = Modulus of Elasticity of Girder Tendons") << rptNewLine;
      }
   }

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( bAfter1999  && shear_capacity_method == scmBTTables )
   {
      (*table)(0,col++) << _T("Min. Reinf.") << rptNewLine << _T("per ") << LrfdCw8th(_T("5.8.2.5"),_T("5.7.2.5"));
      (*table)(0,col++) << _T("Eqn") << rptNewLine << (bAfter2009 ? _T("B5.2-") : _T("5.8.3.4.2-"));
   }

   (*table)(0,col++) << COLHDR( Sub2(_T("M"),_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( bAfter1999 )
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("u")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
      if (0 < (nMaxSegmentDucts + nGirderDucts))
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("V"), _T("ps")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      }

      if (0 < nMaxSegmentDucts)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("V"), _T("pts")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      }

      if (0 < nGirderDucts)
      {
         (*table)(0, col++) << COLHDR(Sub2(_T("V"), _T("ptg")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
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

   if (0 < nMaxSegmentDucts)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("pts")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("E"), _T("pts")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   }

   if (0 < nGirderDucts)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("A"), _T("ptg")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("E"), _T("ptg")), rptStressUnitTag, pDisplayUnits->GetModEUnit());
   }

   (*table)(0,col++) << COLHDR( Sub2(_T("A"),_T("ct")), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
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

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   bool print_footnote1 = false;
   bool print_footnote2 = false;

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      
      ColumnIndexType col = 0;
      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );

      if ( bAfter1999  && shear_capacity_method == scmBTTables )
      {
         (*table)(row,col++) << (scd.Equation == 1 || scd.Equation == 31 ? _T("Yes") : _T("No"));
         if (bAfter2016)
         {
            (*table)(row, col++) << (scd.Equation == 1 ? _T("3") : scd.Equation == 2 ? _T("4") : _T("5"));
         }
         else
         {
            (*table)(row, col++) << (scd.Equation == 1 ? _T("1") : scd.Equation == 2 ? _T("2") : _T("3"));
         }
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
         if (0 < (nMaxSegmentDucts + nGirderDucts))
         {
            (*table)(row, col++) << shear.SetValue(scd.Vps);
         }

         if (0 < nMaxSegmentDucts)
         {
            (*table)(row, col++) << shear.SetValue(scd.VptSegment);
         }

         if (0 < nGirderDucts)
         {
            (*table)(row, col++) << shear.SetValue(scd.VptGirder);
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

      if (0 < nMaxSegmentDucts)
      {
         (*table)(row, col++) << area.SetValue(scd.AptSegment);
         (*table)(row, col++) << mod_e.SetValue(scd.EptSegment);
      }

      if ( 0 < nGirderDucts )
      {
         (*table)(row,col++) << area.SetValue( scd.AptGirder );
         (*table)(row,col++) << mod_e.SetValue( scd.EptGirder );
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
      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;

      *pParagraph << Sub2(_T("N"),_T("u")) << _T(" = 0") << rptNewLine;

      if ( print_footnote1 )
      {
         if ( shear_capacity_method == scmWSDOT2007 || shear_capacity_method == scmBTEquations )
         {
            *pParagraph << _T("$ - Taken as |") << Sub2(_T("V"),_T("u")) << _T(" - ") << Sub2(_T("V"),_T("p")) << _T("|") << Sub2(_T("d"),_T("v")) << _T(" per definitions given in ") << LrfdCw8th(_T("5.8.3.4.2"),_T("5.7.3.4.2")) << rptNewLine;
         }
         else
         {
            *pParagraph << _T("$ - Taken as ") << Sub2(_T("V"),_T("u")) << Sub2(_T("d"),_T("v")) << _T(" per definitions given in ") << LrfdCw8th(_T("5.8.3.4.2"),_T("5.7.3.4.2")) << rptNewLine;
         }
      }

      if ( print_footnote2 )
      {
         *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
      }
   }

// To be removed from WSDOT BDM... 7/25/2006 RAB
//   if ( !bLrfdMethod )
//      *pParagraph << Sub2(symbol(theta),_T("min")) << _T(" = 25") << symbol(DEGREES) << _T(" beyond end region (1.5H). [WSDOT BDM 5.2.4F.2]") << rptNewLine;

}

void write_btsummary_table(IBroker* pBroker,
                       IEAFDisplayUnits* pDisplayUnits,
                       const PoiList& vPoi,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   // if this after 2007 spec then shear capacity method should not equal scmWSDOT2007
   bool bAfter2007 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::FourthEditionWith2008Interims ? true : false );
   ATLASSERT( bAfter2007 ? shear_capacity_method != scmWSDOT2007 : true );

   rptParagraph* pParagraph;
   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   CString strTitle;
   strTitle.Format(_T("Shear Parameter Summary - %s  [%s]"),GetLimitStateString(ls),LrfdCw8th(_T("5.8.3.4.2"),_T("5.7.3.4.2")));
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
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BetaEquation.png")) << rptNewLine;
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ThetaEquation.png")) << rptNewLine;
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("SxeEqn.png")) << rptNewLine;
      }

      if (bUHPC)
      {
         *pParagraph << symbol(beta) << _T(" is computed with LRFD Eq. 5.7.3.4.2-1") << rptNewLine;
      }

      INIT_UV_PROTOTYPE( rptLengthUnitValue,  xdimu,    pDisplayUnits->GetComponentDimUnit(),    true );
      GET_IFACE2(pBroker,IMaterials,pMat);
      GET_IFACE2(pBroker,IPointOfInterest,pPoi);

      std::vector<CGirderKey> vGirderKeys;
      pPoi->GetGirderKeys(vPoi,&vGirderKeys);
      for (const auto& girderKey : vGirderKeys)
      {
         if ( 1 < vGirderKeys.size() )
         {
            *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
         }

         std::vector<CSegmentKey> vSegmentKeys;
         pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
         for (const auto& segmentKey : vSegmentKeys)
         {
            Float64 ag = pMat->GetSegmentMaxAggrSize(segmentKey);
            if ( 1 < vSegmentKeys.size() )
            {
               *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ") << Sub2(_T("a"),_T("g")) << _T(" = ") << xdimu.SetValue(ag) << rptNewLine;
            }
            else
            {
               *pParagraph << Sub2(_T("a"),_T("g")) << _T(" = ") << xdimu.SetValue(ag) << rptNewLine;
            }
         }
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if (print_sxe)
   {
      (*table)(0,col++) << _T("Min. Reinf.") << rptNewLine << _T("per ") << LrfdCw8th(_T("5.8.2.5"),_T("5.7.2.5"));

      if(shear_capacity_method == scmBTTables)
      {
         if ( lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::FourthEdition2007 )
         {
            (*table)(0,col++) << _T("Table") << rptNewLine << _T("5.8.3.4.2-");
         }
         else
         {
            // tables moved to appendx B5 in 2008
            (*table)(0,col++) << _T("Table") << rptNewLine << _T("B5.2-");
         }
      }
      else
      {
         (*table)(0,col++) << _T("Eqn") << rptNewLine << LrfdCw8th(_T("5.8.3.4.2-"),_T("5.7.3.4.2-"));
      }
      (*table)(0,col++) << COLHDR( _T("s")<< Sub(_T("x"))<<_T("*"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR( _T("s")<< Sub(_T("xe")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   if(bAfter1999)
   {
      (*table)(0,col++) << RPT_vu << _T("/") << RPT_FC;
   }
   else
   {
      (*table)(0,col++) << _T("v") << _T("/") << RPT_FC;
   }

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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );

      bool bSufficientTransverseReinforcement = (shear_capacity_method == scmBTTables) ? (scd.BetaThetaTable == 1) : (scd.BetaEqn == 1);

      if (print_sxe)
      {

         (*table)(row,col++) << (bSufficientTransverseReinforcement  ? _T("Yes") : _T("No"));
         (*table)(row,col++) << (bSufficientTransverseReinforcement  ? _T("1")   : _T("2") );
         if(bSufficientTransverseReinforcement)
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
         if (bSufficientTransverseReinforcement)
         {
            (*table)(row,col) << scalar.SetValue( scd.vufc );
            (*table)(row,col++) << _T(" ") << symbol(LTE) << _T(" ") << scalar.SetValue(scd.vfc_tbl);
         }
         else
         {
            (*table)(row,col++) << _T("---");
         }
      }
      else if ( shear_capacity_method != scmBTEquations && shear_capacity_method != scmWSDOT2007 )
      {
         (*table)(row,col++) << scalar.SetValue( scd.vufc );
      }
      else
      {
         (*table)(row,col++) << scalar.SetValue( scd.vufc );
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

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;

   if (print_sxe)
   {
      *pParagraph << _T("* Value of ") << Sub2(_T("s"),_T("x")) << _T(" taken equal to ") << Sub2(_T("d"),_T("v")) << rptNewLine<<rptNewLine;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      *pParagraph << _T("** Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }
}

void write_Vs_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Shear Resistance Provided By Shear Reinforcement - ") << GetLimitStateString(ls) << rptNewLine;

   if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Vs_2020.png")) << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Vs.png")) << rptNewLine;
   }
   *pParagraph << rptNewLine;
   

   ColumnIndexType nCols = 9;

   if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      nCols += 4;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0, col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, col++) << COLHDR( RPT_FY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0, col++) << COLHDR( Sub2(_T("A"),_T("v")), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0, col++) << COLHDR( _T("s"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++) << COLHDR( Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0, col++) << COLHDR( Sub2(_T("d"),_T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0, col++) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   (*table)(0, col++) << COLHDR( symbol(alpha), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
   {
      (*table)(0, col++) << symbol(delta);
      (*table)(0, col++) << COLHDR(Sub2(symbol(phi), _T("duct")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0, col++) << COLHDR(Sub2(_T("b"), _T("w")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
      (*table)(0, col++) << Sub2(symbol(lambda),_T("duct"));
   }
   (*table)(0, col++) << COLHDR( Sub2(_T("V"),_T("s")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs, pDisplayUnits->GetAvOverSUnit(), false );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

//   location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      col = 0;

      (*table)(row, col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row, col++) << stress.SetValue( scd.fy );
      (*table)(row, col++) << area.SetValue( scd.Av );
      (*table)(row, col++) << dim.SetValue( scd.S );
      if (0.0 < scd.S)
      {
         (*table)(row, col++) << avs.SetValue( scd.Av/scd.S );
      }
      else
      {
         ATLASSERT(scd.Av == 0.0);
         (*table)(row, col++) << avs.SetValue( 0.0 );
      }
      (*table)(row, col++) << dim.SetValue( scd.dv );

      if (scd.ShearInRange)
      {
         (*table)(row, col++) << angle.SetValue( scd.Theta );
         (*table)(row, col++) << angle.SetValue( scd.Alpha );
         if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
         {
            (*table)(row, col++) << scd.delta;
            (*table)(row, col++) << dim.SetValue(scd.duct_diameter);
            (*table)(row, col++) << dim.SetValue(scd.bw);
            (*table)(row, col++) << scalar.SetValue(scd.lambda_duct);
         }
         (*table)(row, col++) << shear.SetValue( scd.Vs );
      }
      else
      {
         print_footnote=true;
         (*table)(row, col++) << _T("*");
         (*table)(row, col++) << _T("*");
         if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
         {
            (*table)(row, col++) << _T("*");
            (*table)(row, col++) << _T("*");
            (*table)(row, col++) << _T("*");
            (*table)(row, col++) << _T("*");
         }
         (*table)(row, col++) << _T("*");
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }

}

void write_Vc_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Shear Resistance Provided By Tensile Stress in the Concrete - ") << GetLimitStateString(ls) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if ( 1 < vGirderKeys.size() )
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      std::vector<CSegmentKey> vSegmentKeys;
      pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
      for (const auto& segmentKey : vSegmentKeys)
      {
         if ( 1 < vSegmentKeys.size() )
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ");
         }

         std::_tstring strImage;
         if ( bLambda )
         {
            strImage = _T("VcEquation_2016.png");
         }
         else
         {
            pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
            bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
            switch( concType )
            {
            case pgsTypes::Normal:
            case pgsTypes::UHPC:
               strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_NWC_US.png") : _T("VcEquation_NWC_SI.png"));
               break;

            case pgsTypes::AllLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_LWC_US.png") : _T("VcEquation_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_ALWC_US.png") : _T("VcEquation_ALWC_SI.png"));
               }
               break;

            case pgsTypes::SandLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_LWC_US.png") : _T("VcEquation_LWC_SI.png"));
               }
               else
               {
                  strImage = (IS_US_UNITS(pDisplayUnits) ? _T("VcEquation_SLWC_US.png") : _T("VcEquation_SLWC_SI.png"));
               }
               break;

            default:
               ATLASSERT(false);
            }
         }

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         *pParagraph << rptNewLine;
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(7);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType colIdx = 0;

   (*table)(0,colIdx++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,colIdx++) << symbol(beta);
   (*table)(0,colIdx++) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
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

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      colIdx = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
      bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,colIdx++) << location.SetValue( POI_SPAN, poi );

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
      {
         (*table)(row,colIdx++) << stress.SetValue( scd.fct );
      }
      else
      {
         (*table)(row,colIdx++) << _T("-");
      }

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
      pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded")<< rptNewLine<<rptNewLine;
   }
}


void write_Vf_table(IBroker* pBroker,
   IEAFDisplayUnits* pDisplayUnits,
   const PoiList& vPoi,
   rptChapter* pChapter,
   IntervalIndexType intervalIdx,
   const std::_tstring& strStageName, pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << _T("Shear Resistance Provided By Fibers in the Concrete - ") << GetLimitStateString(ls) << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   GET_IFACE2(pBroker, IMaterials, pMaterial);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if (1 < vGirderKeys.size())
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      std::vector<CSegmentKey> vSegmentKeys;
      pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
      for (const auto& segmentKey : vSegmentKeys)
      {
         ATLASSERT(pMaterial->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC);

         if (1 < vSegmentKeys.size())
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ");
         }

         std::_tstring strImage(_T("Vf.png"));

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         *pParagraph << rptNewLine;
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType colIdx = 0;

   (*table)(0, colIdx++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0, colIdx++) << symbol(PSI) << RPT_STRESS(_T("fu"));
   (*table)(0, colIdx++) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit());
   (*table)(0, colIdx++) << COLHDR(Sub2(_T("b"), _T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0, colIdx++) << COLHDR(Sub2(_T("d"), _T("v")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0, colIdx++) << COLHDR(Sub2(_T("V"), _T("f")), rptForceUnitTag, pDisplayUnits->GetShearUnit());

   INIT_UV_PROTOTYPE(rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false);
   INIT_UV_PROTOTYPE(rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false);

   //location.IncludeSpanAndGirder(span == ALL_SPANS);

   GET_IFACE2(pBroker, IBridge, pBridge);

   bool print_footnote = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker, IShearCapacity, pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      colIdx = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi, ls))
      {
         continue;
      }

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

      pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls, intervalIdx, poi, nullptr, &scd);

      (*table)(row, colIdx++) << location.SetValue(POI_SPAN, poi);
      (*table)(row, colIdx++) << stress.SetValue(scd.FiberStress);
      (*table)(row, colIdx++) << angle.SetValue(scd.Theta);
      (*table)(row, colIdx++) << dim.SetValue(scd.bv);
      (*table)(row, colIdx++) << dim.SetValue(scd.dv);
      (*table)(row, colIdx++) << shear.SetValue(scd.Vf);

      row++;
   }
}

void write_Vci_table(IBroker* pBroker,
                    IEAFDisplayUnits* pDisplayUnits,
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << GetLimitStateString(ls) << _T(" - ");

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   *pParagraph << _T("Shear Resistance Provided by Concrete when inclined cracking results from combined shear and moment") << rptNewLine;
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if ( 1 < vGirderKeys.size() )
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      std::vector<CSegmentKey> vSegmentKeys;
      pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
      for (const auto& segmentKey : vSegmentKeys)
      {
         if ( 1 < vSegmentKeys.size() )
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ");
         }

         std::_tstring strImage;
         if ( bLambda )
         {
            strImage = _T("Vci_2016.png");
         }
         else
         {
            pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
            bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
            switch( concType )
            {
            case pgsTypes::Normal:
            case pgsTypes::UHPC:
               strImage = _T("Vci_NWC.png");
               break;

            case pgsTypes::AllLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("Vci_LWC.png");
               }
               else
               {
                  strImage = _T("Vci_ALWC.png");
               }
               break;

            case pgsTypes::SandLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("Vci_LWC.png");
               }
               else
               {
                  strImage = _T("Vci_SLWC.png");
               }
               break;

            default:
               ATLASSERT(false);
            }
         }

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         *pParagraph << rptNewLine;
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(9);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,0) << location.SetValue( POI_SPAN, poi );
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
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << GetLimitStateString(ls) << _T(" - ");

   *pParagraph << _T("Shear Resistance Provided by Concrete when inclined cracking results from excessive principal tension in the web.") << rptNewLine;

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if ( 1 < vGirderKeys.size() )
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      std::vector<CSegmentKey> vSegmentKeys;
      pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
      for (const auto& segmentKey : vSegmentKeys)
      {
         if ( 1 < vSegmentKeys.size() )
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ");
         }

         std::_tstring strImage;
         if ( bLambda )
         {
            strImage = _T("Vcw_2016.png");
         }
         else
         {
            pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
            bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
            switch( concType )
            {
            case pgsTypes::Normal:
            case pgsTypes::UHPC:
               strImage = _T("Vcw_NWC.png");
               break;

            case pgsTypes::AllLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("Vcw_LWC.png");
               }
               else
               {
                  strImage = _T("Vcw_ALWC.png");
               }
               break;

            case pgsTypes::SandLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("Vcw_LWC.png");
               }
               else
               {
                  strImage = _T("Vcw_SLWC.png");
               }
               break;

            default:
               ATLASSERT(false);
            }
         }

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         *pParagraph << rptNewLine;
      }
   }


   rptRcTable* table = rptStyleManager::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,0) << location.SetValue( POI_SPAN, poi );
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
                       const PoiList& vPoi,
                       rptChapter* pChapter,
                       IntervalIndexType intervalIdx,
                       const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pParagraph;

   *pParagraph << GetLimitStateString(ls) << _T(" - ");

   *pParagraph << _T("Angle of inclination of diagonal compressive stress [LRFD 5.8.3.3 and 5.8.3.4.3]") << rptNewLine;

   bool bLambda = (lrfdVersionMgr::SeventhEditionWith2016Interims <= lrfdVersionMgr::GetVersion() ? true : false);

   GET_IFACE2(pBroker,IMaterials,pMaterial);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      if ( 1 < vGirderKeys.size() )
      {
         *pParagraph << _T("Girder ") << LABEL_GIRDER(girderKey.girderIndex) << rptNewLine;
      }

      std::vector<CSegmentKey> vSegmentKeys;
      pPoi->GetSegmentKeys(vPoi, girderKey, &vSegmentKeys);
      for (const auto& segmentKey : vSegmentKeys)
      {
         if ( 1 < vSegmentKeys.size() )
         {
            *pParagraph << _T("Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) << _T(" ");
         }

         std::_tstring strImage;
         if ( bLambda )
         {
            strImage = _T("cotan_theta_2016.png");
         }
         else
         {
            pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
            bool bHasAggSplittingStrength = pMaterial->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
            switch( concType )
            {
            case pgsTypes::Normal:
            case pgsTypes::UHPC:
               strImage = _T("cotan_theta_NWC.png");
               break;

            case pgsTypes::AllLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("cotan_theta_LWC.png");
               }
               else
               {
                  strImage = _T("cotan_theta_ALWC.png");
               }
               break;

            case pgsTypes::SandLightweight:
               if ( bHasAggSplittingStrength )
               {
                  strImage = _T("cotan_theta_LWC.png");
               }
               else
               {
                  strImage = _T("cotan_theta_SLWC.png");
               }
               break;

            default:
               ATLASSERT(false);
            }
         }

         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + strImage) << rptNewLine;

         *pParagraph << rptNewLine;
      }
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,0) << location.SetValue( POI_SPAN, poi );
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
                    const PoiList& vPoi,
                    rptChapter* pChapter,
                    IntervalIndexType intervalIdx,
                    const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC)
{
   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());

   CString strName;
   strName.Format(_T("Nominal Shear Resistance - %s"),GetLimitStateString(ls));

   *pChapter << pParagraph;

   CollectionIndexType nCol = (shear_capacity_method == scmVciVcw ? 11 : 12);
   if (bUHPC)
   {
      nCol++;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCol,strName);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
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
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")) << Super(_T("&")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }
   else
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   }

   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("V"), _T("f")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   }

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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
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
         if (bUHPC)
         {
            (*table)(row, col++) << shear.SetValue(scd.Vf);
         }
         (*table)(row,col++) << shear.SetValue( scd.Vs );
         (*table)(row,col++) << shear.SetValue( scd.Vn1 );
      }
      else
      {
         print_footnote=true;
         (*table)(row,col++) << _T("*");
         if (bUHPC)
         {
            (*table)(row, col++) << _T("*");
         }
         (*table)(row, col++) << _T("*");
         (*table)(row,col++) << _T("*");
      }

      (*table)(row,col++) << shear.SetValue( scd.Vn2 );
      (*table)(row,col++) << shear.SetValue( scd.Vn );
      (*table)(row,col++) << scalar.SetValue( scd.Phi );
      (*table)(row,col++) << shear.SetValue( scd.pVn );

      row++;
   }

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << Super(_T("&")) << Sub2(_T("V"),_T("c")) << _T(" = ") << _T("min(") << Sub2(_T("V"),_T("ci")) << _T(",") << Sub2(_T("V"),_T("cw")) << _T(")") << rptNewLine;
   }

   *pParagraph << Super(_T("$")) << Sub2(_T("V"), _T("n1")) << _T(" = ") << Sub2(_T("V"), _T("c"));
   if (bUHPC)
   {
      *pParagraph << _T(" + ") << Sub2(_T("V"), _T("f"));
   }
   *pParagraph << _T(" + ") << Sub2(_T("V"), _T("s"));

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << _T(" [Eqn ") << LrfdCw8th(_T("5.8.3.3-1"),_T("5.7.3.3-1")) << _T(" with ") << Sub2(_T("V"),_T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(" [Eqn ") << LrfdCw8th(_T("5.8.3.3-1"),_T("5.7.3.3-1")) << _T("]")<< rptNewLine;
   }


   std::_tstring strK(bUHPC ? _T("0.18") : _T("0.25"));
   *pParagraph << Super(_T("#")) << Sub2(_T("V"),_T("n2")) << _T(" = ") << strK << RPT_FC << Sub2(_T("b"),_T("v")) << Sub2(_T("d"),_T("v"));
   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << _T(" [Eqn  ") << LrfdCw8th(_T("5.8.3.3-2"),_T("5.7.3.3-2")) << _T(" with ") << Sub2(_T("V"),_T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(" [Eqn ") << LrfdCw8th(_T("5.8.3.3-2"),_T("5.7.3.3-2")) << _T("]")<< rptNewLine;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      *pParagraph << _T("* - Value could not be calculated. Shear crushing capacity of section exceeded.")<< rptNewLine<<rptNewLine;
   }
}

void write_Avs_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls,bool bUHPC)
{
   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   CString strLabel;
   strLabel.Format(_T("Required Shear Reinforcement - %s"),GetLimitStateString(ls));

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pParagraph << strLabel << rptNewLine;
   *pChapter << pParagraph;

   if ( shear_capacity_method != scmVciVcw )
   {
      if (bUHPC)
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_RequiredShearReinforcement1.png"));
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("RequiredShearReinforcement1.png"));
      }
   }
   else
   {
      if (bUHPC)
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("UHPC_RequiredShearReinforcement2.png"));
      }
      else
      {
         *pParagraph << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("RequiredShearReinforcement2.png"));
      }
   }

   CollectionIndexType nCol = (shear_capacity_method == scmVciVcw ? 8 : 9);
   if (bUHPC)
   {
      nCol++;
   }

   rptRcTable* table = rptStyleManager::CreateDefaultTable(nCol);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   CollectionIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("u")) << _T("/") << symbol(phi), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("c")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   if (bUHPC)
   {
      (*table)(0, col++) << COLHDR(Sub2(_T("V"), _T("f")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   }

   if ( shear_capacity_method != scmVciVcw )
   {
      (*table)(0,col++) << COLHDR( Sub2(_T("V"),_T("p")), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   }

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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );
      (*table)(row,col++) << shear.SetValue( scd.Vu/scd.Phi );
      (*table)(row,col++) << shear.SetValue( scd.Vc );

      if (bUHPC)
      {
         (*table)(row, col++) << shear.SetValue(scd.Vf);
      }
      
      if ( shear_capacity_method != scmVciVcw )
      {
         (*table)(row,col++) << shear.SetValue( scd.Vp );
      }
      
      (*table)(row,col++) << shear.SetValue( scd.VsReqd );
      (*table)(row,col++) << stress.SetValue( scd.fy );
      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << angle.SetValue( scd.Theta );
      (*table)(row,col++) << avs.SetValue( scd.AvOverS_Reqd );

      row++;
   }

   pParagraph = new rptParagraph(rptStyleManager::GetFootnoteStyle());
   *pChapter << pParagraph;

   if (shear_capacity_method == scmVciVcw)
   {
      *pParagraph << _T("* - Transverse reinforcement required if ") << Sub2(_T("V"), _T("u")) << _T(" > 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"), _T("c"));
      *pParagraph << _T(")");
      *pParagraph << _T(" [LRFD Eqn 5.8.2.4-1 with ") << Sub2(_T("V"), _T("p")) << _T(" taken to be 0]") << rptNewLine;
   }
   else
   {
      *pParagraph << _T("* - Transverse reinforcement required if ") << Sub2(_T("V"),_T("u")) << _T(" > 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"),_T("c"));
      *pParagraph  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(") [LRFD Eqn ") << LrfdCw8th(_T("5.8.2.4-1"),_T("5.7.2.3-1")) << _T("]")<< rptNewLine;
   }
}

void write_bar_spacing_table(IBroker* pBroker,
                     IEAFDisplayUnits* pDisplayUnits,
                     const PoiList& vPoi,
                     rptChapter* pChapter,
                     IntervalIndexType intervalIdx,
                     const std::_tstring& strStageName,pgsTypes::LimitState ls)
{
   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   CString strLabel;
   strLabel.Format(_T("Required Stirrup Spacing - %s"),GetLimitStateString(ls));

   pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pParagraph << strLabel << rptNewLine;
   *pChapter << pParagraph;

   rptRcTable* table = rptStyleManager::CreateDefaultTable(6);

   //if ( segmentKey.groupIndex == ALL_GROUPS )
   //{
   //   table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   //   table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   //}

   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;

   (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   (*table)(0,col++) << COLHDR( _T("Provided") << rptNewLine << Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );
   (*table)(0,col++) << COLHDR( _T("Required") << rptNewLine << Sub2(_T("A"),_T("v")) << _T("/S"), rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );


   Float64 nLegs = 2.0;

   // use the same number of legs throught the girder.
   // determine the number of legs based on the current stirrup input
   // since the first segment is just as good as any other segment, use it
   // to determine the number of stirrup legs
   const CSegmentKey& segmentKey = vPoi.front().get().GetSegmentKey();

   GET_IFACE2(pBroker,IShear,pShear);
   const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);

   if ( 0 < pShearData->ShearZones.size() )
   {
      nLegs = pShearData->ShearZones[0].nVertBars;
   }

   if (nLegs == 0)
   {
      nLegs = 2;
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
   Float64 end_size = pBridge->GetSegmentStartEndDistance(vPoi.front().get().GetSegmentKey());

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;

      // Don't print poi that are inside of a CSS zone
      if (pPoi->IsInCriticalSectionZone(poi,ls))
      {
         continue;
      }

      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,intervalIdx,poi, nullptr, &scd);

      (*table)(row,col++) << location.SetValue( POI_SPAN, poi );

      if(0.0 < scd.Av && 0.0 < scd.S)
      {
         (*table)(row,col++) << avs.SetValue( scd.Av/scd.S);
      }
      else
      {
         (*table)(row,col++) << _T("---");
      }

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
