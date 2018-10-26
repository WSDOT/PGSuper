///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\ShearData.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\BridgeDescription.h>

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

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
                                IDisplayUnits* pDisplayUnits,
                                SpanIndexType span,
                                GirderIndexType gdr,
                                const std::vector<pgsPointOfInterest>& pois,
                                rptChapter* pChapter,
                                pgsTypes::Stage stage,
                                const std::string& strStageName,pgsTypes::LimitState ls);

void write_shear_stress_table(  IBroker* pBroker,
                                IDisplayUnits* pDisplayUnits,
                                SpanIndexType span,
                                GirderIndexType gdr,
                                const std::vector<pgsPointOfInterest>& pois,
                                rptChapter* pChapter,
                                pgsTypes::Stage stage,
                                const std::string& strStageName,pgsTypes::LimitState ls);

void write_fpo_table(IBroker* pBroker,
                       IDisplayUnits* pDisplayUnits,
                       SpanIndexType span,
                       GirderIndexType gdr,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       pgsTypes::Stage stage,
                       const std::string& strStageName,pgsTypes::LimitState ls);

void write_fpc_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls);

void write_fpce_table(IBroker* pBroker,
                      IDisplayUnits* pDisplayUnits,
                      SpanIndexType span,
                      GirderIndexType gdr,
                      const std::vector<pgsPointOfInterest>& pois,
                      rptChapter* pChapter,
                      pgsTypes::Stage stage,
                      const std::string& strStageName,pgsTypes::LimitState ls);

void write_Fe_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_btsummary_table(IBroker* pBroker,
                       IDisplayUnits* pDisplayUnits,
                       SpanIndexType span,
                       GirderIndexType gdr,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       pgsTypes::Stage stage,
                       const std::string& strStageName,pgsTypes::LimitState ls);

void write_ex_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_Vs_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_theta_table(IBroker* pBroker,
                       IDisplayUnits* pDisplayUnits,
                       SpanIndexType span,
                       GirderIndexType gdr,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       pgsTypes::Stage stage,
                       const std::string& strStageName,pgsTypes::LimitState ls);

void write_Vc_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_Vci_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_Vcw_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_Vn_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls);

void write_Avs_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls);

void write_bar_spacing_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CShearCapacityDetailsChapterBuilder::CShearCapacityDetailsChapterBuilder(bool bDesign,bool bRating)
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
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

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
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph();
   *pChapter << pPara;

   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi;

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   pgsTypes::Stage stage(pgsTypes::BridgeSite3);
   const std::string stage_name("Bridge Site Stage III");

   vPoi = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite3, span,girder, POI_SHEAR|POI_TABULAR);

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
      write_shear_dimensions_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);

      if ( shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001 )
      {
         write_shear_stress_table    (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_fpc_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_fpo_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Fe_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_ex_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_btsummary_table       (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vc_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vs_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
      }
      else if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
      {
         write_fpo_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_ex_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_btsummary_table       (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vc_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vs_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
      }
      else if ( shear_capacity_method == scmVciVcw )
      {
         write_fpce_table            (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vci_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_fpc_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vcw_table             (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_theta_table           (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
         write_Vs_table              (pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
      }

      write_Vn_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, ls);
   }

   if ( bDesign )
   {
      write_Avs_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, pgsTypes::StrengthI);
      write_bar_spacing_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, pgsTypes::StrengthI);

      if ( bPermit )
      {
         write_Avs_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, pgsTypes::StrengthII);
         write_bar_spacing_table(pBroker,pDisplayUnits,span,girder, vPoi,  pChapter, stage, stage_name, pgsTypes::StrengthII);
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
         pPara->SetName("Horizontal Interface Shear");
         *pPara << pPara->GetName() << rptNewLine;
         *pChapter << pPara;
         CInterfaceShearDetails::Build(pBroker, pChapter, span, girder, pDisplayUnits, stage,  pgsTypes::StrengthI);

         if ( bPermit )
            CInterfaceShearDetails::Build(pBroker, pChapter, span, girder, pDisplayUnits, stage,  pgsTypes::StrengthII);
      }
   }

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
                             IDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                             const std::vector<pgsPointOfInterest>& pois,
                             rptChapter* pChapter,
                             pgsTypes::Stage stage,
                             const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IStageMap,pStageMap);

   // Setup the table
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;

   CString strName;
   strName.Format("Effective Shear Dimensions for %s",OLE2A(pStageMap->GetLimitStateName(ls)));
   pParagraph->SetName(strName);

   *pParagraph << pParagraph->GetName() << " [From Article 5.8.2.7]" << rptNewLine;

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);

   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->DeckType;

   std::string strPicture = pFactory->GetShearDimensionsSchematicImage(deckType);
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + strPicture);

   *pParagraph << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "dv Equation.jpg") << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR(Sub2("b","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2) << COLHDR("Moment" << rptNewLine << "Arm", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR(Sub2("d","e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR("h", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR(Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << "Tension" << rptNewLine << "Side";

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << dim.SetValue( scd.bv );
      (*table)(row,2) << dim.SetValue( scd.MomentArm );
      (*table)(row,3) << dim.SetValue( scd.de );
      (*table)(row,4) << dim.SetValue( scd.h );
      (*table)(row,5) << dim.SetValue( scd.dv );

      (*table)(row,6) << (scd.bTensionBottom ? "Bottom" : "Top");

      row++;
   }
}

void write_shear_stress_table(IBroker* pBroker,
                              IDisplayUnits* pDisplayUnits,
                             SpanIndexType span,
                             GirderIndexType gdr,
                              const std::vector<pgsPointOfInterest>& pois,
                              rptChapter* pChapter,
                              pgsTypes::Stage stage,
                              const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName("Shear Stress");
   *pChapter << pParagraph;

   std::string strEquation;
   switch( lrfdVersionMgr::GetVersion() )
   {
      case lrfdVersionMgr::FirstEdition1994:
      case lrfdVersionMgr::FirstEditionWith1996Interims:
      case lrfdVersionMgr::FirstEditionWith1997Interims:
      case lrfdVersionMgr::SecondEdition1998:
      case lrfdVersionMgr::SecondEditionWith1999Interims:
         strEquation = " [Eqn 5.8.3.4.2-1]";
         break;

      case lrfdVersionMgr::SecondEditionWith2000Interims:
      case lrfdVersionMgr::SecondEditionWith2001Interims:
      case lrfdVersionMgr::SecondEditionWith2002Interims:
      case lrfdVersionMgr::SecondEditionWith2003Interims:
      case lrfdVersionMgr::ThirdEdition2004:
         strEquation = " [Eqn 5.8.2.9-1]";
         break;

      case lrfdVersionMgr::ThirdEditionWith2005Interims:
      case lrfdVersionMgr::ThirdEditionWith2006Interims:
         strEquation = " [Eqn 5.8.2.4-1]";
         break;

      case lrfdVersionMgr::FourthEdition2007:
      case lrfdVersionMgr::FourthEditionWith2008Interims:
      case lrfdVersionMgr::FourthEditionWith2009Interims:
      default:
         strEquation = " [Eqn 5.8.2.9-1]";
         break;
   }

   *pParagraph << Italic("v") << strEquation << rptNewLine;
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "v Equation.jpg") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "v Equation 2005.gif") << rptNewLine;

   *pParagraph << rptNewLine;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   CString strTitle;
   strTitle.Format("Factored Shear Stresses for %s",OLE2A(pStageMap->GetLimitStateName(ls)));
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,std::string(strTitle));

   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << symbol(phi);
   (*table)(0,2) << COLHDR("V" << Sub("u"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3) << COLHDR("V" << Sub("p"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR("d" << Sub("v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR("b" << Sub("v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR(Italic("v") << Sub("u"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  force,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << scd.Phi;
      (*table)(row,2) << force.SetValue( scd.Vu );
      (*table)(row,3) << force.SetValue( scd.Vp );
      (*table)(row,4) << dim.SetValue( scd.dv );
      (*table)(row,5) << dim.SetValue( scd.bv );
      (*table)(row,6) << stress.SetValue( (scd.vfc * scd.fc) );

      row++;
   }
}

void write_fpc_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls)
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
   pParagraph->SetName("fpc");
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << Sub2("f","pc") << " [for use in Eqn 5.8.3.4.3-3] - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;
   }
   else
   {
      *pParagraph << Sub2("f","pc") << " [for use in Eqn C5.8.3.4.2-1] - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;
   }

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Fpc Pic.jpg") << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "fpc_2007.gif") << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR("e", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
   (*table)(0,2) << COLHDR("P", rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit() );
   (*table)(0,3) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,4) << COLHDR(Sub2("I","g"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,5) << COLHDR("c",rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,6) << COLHDR(Sub2("M","DC") << " + " << Sub2("M","DW"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,7) << COLHDR(Sub2("f","pc"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,    pDisplayUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  dim,      pDisplayUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,     pDisplayUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,   pDisplayUnits->GetMomentUnit(),          false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      FPCDETAILS fpcd;
      pShearCap->GetFpcDetails(poi, &fpcd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << dim.SetValue( fpcd.e );
      (*table)(row,2) << force.SetValue( fpcd.P );
      (*table)(row,3) << area.SetValue( fpcd.Ag );
      (*table)(row,4) << inertia.SetValue( fpcd.Ig );
      (*table)(row,5) << dim.SetValue( fpcd.c );
      (*table)(row,6) << moment.SetValue( fpcd.Mg );
      (*table)(row,7) << stress.SetValue( fpcd.fpc );

      row++;
   }
}

void write_fpce_table(IBroker* pBroker,
                      IDisplayUnits* pDisplayUnits,
                      SpanIndexType span,
                      GirderIndexType gdr,
                      const std::vector<pgsPointOfInterest>& pois,
                      rptChapter* pChapter,
                      pgsTypes::Stage stage,
                      const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName("Mcre");
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << Sub2("M","cre") << OLE2A(pStageMap->GetLimitStateName(ls)) << " [Eqn 5.8.3.4.3-2]" << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Mcre.gif") << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1)  << COLHDR( Sub2("f","r"),   rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2)  << COLHDR( Sub2("f","cpe"), rptPressureUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3)  << COLHDR( Sub2("S","nc"),  rptLength3UnitTag,  pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,4)  << COLHDR( Sub2("S","c"),   rptLength3UnitTag,  pDisplayUnits->GetSectModulusUnit() );
   (*table)(0,5)  << COLHDR( Sub2("M","dnc"), rptMomentUnitTag,   pDisplayUnits->GetMomentUnit() );
   (*table)(0,6)  << COLHDR( Sub2("M","cre"), rptMomentUnitTag,   pDisplayUnits->GetMomentUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest,   location,       pDisplayUnits->GetSpanLengthUnit(),         false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,   stress,         pDisplayUnits->GetStressUnit(),             false );
   INIT_UV_PROTOTYPE( rptLength3UnitValue,  sect_mod,       pDisplayUnits->GetSectModulusUnit(),        false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,         pDisplayUnits->GetMomentUnit(),             false );
   INIT_UV_PROTOTYPE( rptSqrtPressureValue, fr_coefficient, pDisplayUnits->GetTensionCoefficientUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
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

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   *pParagraph << Sub2("f","r") << " = " << fr_coefficient.SetValue(pMaterial->GetShearFrCoefficient()) << symbol(ROOT) << RPT_FC << rptNewLine;

   *pParagraph << Sub2("f","cpe") << " = compressive stress in concrete due to effective prestress force only (after allowance for all prestress losses) at extreme fiber of section where tensile stress is caused by externally applied loads." << rptNewLine;
   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "fcpe.gif") << rptNewLine;
   *pParagraph << Sub2("S","nc") << " = section modulus for the extreme fiber of the monolithic or noncomposite section where tensile stress is caused by externally applied loads" << rptNewLine;
   *pParagraph << Sub2("S","c") << " = section modulus for the extreme fiber of the composite section where tensile stress is caused by externally applied loads" << rptNewLine;
   *pParagraph << Sub2("M","dnc") << " = total unfactored dead load moment acting on the monolithic or noncomposite section" << rptNewLine;
}

void write_fpo_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName("fpo");
   *pChapter << pParagraph;
   *pParagraph << Sub2("f","po");

   if ( !bAfter1999 )
      *pParagraph << " [Eqn C5.8.3.4.2-1]";

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << " - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;

   if ( bAfter1999 )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "fpo Equation 2000.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "fpo Equation.jpg") << rptNewLine;

   *pParagraph << rptNewLine;

   if ( bAfter1999 )
   {
      GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
      const matPsStrand* pStrand = pMaterial->GetStrand(span,gdr);

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    true );

      pParagraph = new rptParagraph();
      *pChapter << pParagraph;

      *pParagraph << Sub2("f","po") << " = " << stress.SetValue(0.7*pStrand->GetUltimateStrength()) << rptNewLine;
   }
   else
   {
      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,"");
      *pParagraph << table << rptNewLine;

      if ( stage == pgsTypes::CastingYard )
         (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
      else
         (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      (*table)(0,1) << COLHDR( RPT_FPE, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,2) << COLHDR( RPT_FPC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR( RPT_EP,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,4) << COLHDR( RPT_EC,  rptStressUnitTag, pDisplayUnits->GetModEUnit() );
      (*table)(0,5) << COLHDR( RPT_FPO, rptStressUnitTag, pDisplayUnits->GetStressUnit() );

      INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),    false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),      false );

      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
      if ( stage == pgsTypes::CastingYard )
         end_size = 0; // don't adjust if CY stage

      Int16 row = 1;
      GET_IFACE2(pBroker,IShearCapacity,pShearCap);
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = pois.begin(); i != pois.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;
         SHEARCAPACITYDETAILS scd;
         pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

         (*table)(row,0) << location.SetValue( poi, end_size );

         (*table)(row,1) << stress.SetValue( scd.fpe );
         (*table)(row,2) << stress.SetValue( scd.fpc );
         (*table)(row,3) << mod_e.SetValue( scd.Ep );
         (*table)(row,4) << mod_e.SetValue( scd.Ec );
         (*table)(row,5) << stress.SetValue( scd.fpo );

         row++;
      }
   }
}

void write_Fe_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims )
      return; // This is not applicable 2000 and later

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName("Fe");
   *pChapter << pParagraph;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << Sub2("F",symbol(epsilon)) << " [Eqn 5.8.3.4.2-3] - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Fe Equation.jpg") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;
   *pParagraph << "This calculation is applicable only when " << symbol(epsilon) << Sub("x") << " is negative." << rptNewLine;
   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR( RPT_ES, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,2) << COLHDR( RPT_AS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,3) << COLHDR( RPT_EP, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,4) << COLHDR( RPT_APS, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,5) << COLHDR( RPT_EC, rptStressUnitTag, pDisplayUnits->GetModEUnit() );
   (*table)(0,6) << COLHDR( RPT_AC, rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,7) << "F" << Sub(symbol(epsilon));

   INIT_UV_PROTOTYPE( rptPointOfInterest,  location, pDisplayUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,    pDisplayUnits->GetModEUnit(),            false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,     pDisplayUnits->GetAreaUnit(),            false );

   rptRcScalar scalar;
   scalar.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   scalar.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   scalar.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );

      if ( scd.Fe < 0 )
      {
         (*table)(row,1) << "-";
         (*table)(row,2) << "-";
         (*table)(row,3) << "-"; 
         (*table)(row,4) << "-"; 
         (*table)(row,5) << "-"; 
         (*table)(row,6) << "-"; 
         (*table)(row,7) << "-"; 
      }
      else
      {
         (*table)(row,1) << mod_e.SetValue( scd.Es );
         (*table)(row,2) << area.SetValue( scd.As );
         (*table)(row,3) << mod_e.SetValue( scd.Ep );
         (*table)(row,4) << area.SetValue( scd.Aps );
         (*table)(row,5) << mod_e.SetValue( scd.Ec );
         (*table)(row,6) << area.SetValue( scd.Ac );
         (*table)(row,7) << scalar.SetValue(scd.Fe);
      }

      row++;
   }
}

void write_ex_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
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
   pParagraph->SetName("Longitudinal Strain");
   *pChapter << pParagraph;
   *pParagraph << "Longitudinal Strain " << Sub2(symbol(epsilon),"x");
   if ( !bAfter1999 )
      *pParagraph << " [Eqn 5.8.3.4.2-2] ";

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << "- " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;

   if ( bAfter2007 )
   {
      if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
      {
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2008.gif") << rptNewLine;
      }
      else if ( shear_capacity_method == scmWSDOT2001 )
      {
         // tables with WSDOT modifications
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation WSDOT 2003.gif") << rptNewLine;
      }
      else
      {
         // tables
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2005.gif") << rptNewLine;
      }
   }
   else if ( bAfter2004 )
   {
      if ( shear_capacity_method == scmWSDOT2007 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2008.gif") << rptNewLine;
      else if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation WSDOT 2003.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2005.gif") << rptNewLine;
   }
   else if ( bAfter2003 )
   {
      ATLASSERT(shear_capacity_method != scmWSDOT2007);

      if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation WSDOT 2003.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2003.gif") << rptNewLine;
   }
   else if ( bAfter1999 )
   {
      if ( shear_capacity_method == scmWSDOT2001 )
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation WSDOT 2000.gif") << rptNewLine;
      else
         *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation 2000.gif") << rptNewLine;
   }
   else
   {
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ex Equation.jpg") << rptNewLine; // 1999 and earlier
   }

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   if ( !bAfter1999 )
   {
      *pParagraph << Sub2(symbol(epsilon),"x") << " includes " << Sub2("F",symbol(epsilon)) << " when applicable" << rptNewLine;
      *pParagraph << rptNewLine;
   }

   Int16 nCol = (bAfter1999 && shear_capacity_method == scmBTTables ? 11 : 9);
   if ( shear_capacity_method == scmWSDOT2001 || 
        shear_capacity_method == scmWSDOT2007 || 
        shear_capacity_method == scmBTEquations 
      )
      nCol--;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol,"");
   Int16 col = 1;

   *pParagraph << table << rptNewLine;

   col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( bAfter1999  && shear_capacity_method == scmBTTables )
   {
      (*table)(0,col++) << "Min. Reinf." << rptNewLine << "per 5.8.2.5" ;
      (*table)(0,col++) << "Eqn" << rptNewLine << "5.8.3.4.2-";
   }

   (*table)(0,col++) << COLHDR( Sub2("M","u"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   if ( !bAfter1999 )
      (*table)(0,col++) << COLHDR( Sub2("V","u"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,col++) << COLHDR( "|" << Sub2("V","u") << " - " << Sub2("V","p") << "|", rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,col++) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( Sub2("A","s"), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( Sub2("A","ps"), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,col++) << COLHDR( Sub2("A","c"), rptLength2UnitTag, pDisplayUnits->GetAreaUnit() );
   
   if ( shear_capacity_method != scmWSDOT2001 && 
        shear_capacity_method != scmWSDOT2007 &&
        shear_capacity_method != scmBTEquations 
      )
      (*table)(0,col++) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   (*table)(0,col++) << Sub2(symbol(epsilon),"x") << rptNewLine << "x 1000";

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptLength2UnitValue, area, pDisplayUnits->GetAreaUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool print_footnote1 = false;
   bool print_footnote2 = false;

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      
      ColumnIndexType col = 0;
      (*table)(row,col++) << location.SetValue( poi, end_size );

      if ( bAfter1999  && shear_capacity_method == scmBTTables )
      {
         (*table)(row,col++) << (scd.Equation == 1 || scd.Equation == 31 ? "Yes" : "No");
         (*table)(row,col++) << (scd.Equation == 1 ? "1" : scd.Equation == 2 ? "2" : "3");
      }

      (*table)(row,col) << moment.SetValue( scd.Mu );
      if ( bAfter1999 )
      {
         if ( scd.MuLimitUsed)
         {
            print_footnote1 = true;
            (*table)(row,col) << " $";
         }
      }
      col++;

      if ( !bAfter1999 )
         (*table)(row,col++) << shear.SetValue( scd.Vu );
      else
         (*table)(row,col++) << shear.SetValue( fabs(scd.Vu - scd.Vp) );

      (*table)(row,col++) << dim.SetValue( scd.dv );
      (*table)(row,col++) << area.SetValue( scd.As );
      (*table)(row,col++) << area.SetValue( scd.Aps );
      (*table)(row,col++) << area.SetValue( scd.Ac );
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
            (*table)(row,col) << " " << symbol(LTE) << " " << scalar.SetValue( scd.ex_tbl*1000.0 );
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
         (*table)(row,col++) << "*";
         (*table)(row,col++) << "*";
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote1 || print_footnote2)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;

      *pParagraph << Sub2("N","u") << " = 0" << rptNewLine;

      if ( print_footnote1 )
      {
         if ( shear_capacity_method == scmWSDOT2007 || shear_capacity_method == scmBTEquations )
            *pParagraph << "$ - Taken as |" << Sub2("V","u") << " - " << Sub2("V","p") << "|" << Sub2("d","v") << " per definitions given in 5.8.3.4.2" << rptNewLine;
         else
            *pParagraph << "$ - Taken as " << Sub2("V","u") << Sub2("d","v") << " per definitions given in 5.8.3.4.2" << rptNewLine;
      }

      if ( print_footnote2 )
         *pParagraph << "* - Value could not be calculated. Shear crushing capacity of section exceeded"<< rptNewLine<<rptNewLine;
   }

// To be removed from WSDOT BDM... 7/25/2006 RAB
//   if ( !bLrfdMethod )
//      *pParagraph << Sub2(symbol(theta),"min") << " = 25" << symbol(DEGREES) << " beyond end region (1.5H). [WSDOT BDM 5.2.4F.2]" << rptNewLine;

}

void write_btsummary_table(IBroker* pBroker,
                       IDisplayUnits* pDisplayUnits,
                       SpanIndexType span,
                       GirderIndexType gdr,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       pgsTypes::Stage stage,
                       const std::string& strStageName,pgsTypes::LimitState ls)
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
   pParagraph->SetName("Shear Parameters");
   *pChapter << pParagraph;


   if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
   {
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "BetaEquation.gif") << rptNewLine;
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "ThetaEquation.gif") << rptNewLine;
   }


   ColumnIndexType nCol = (shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007) ? 4 : 6;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   CString strTitle;
   strTitle.Format("Shear Parameters Summary - %s",OLE2A(pStageMap->GetLimitStateName(ls)));
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol,std::string(strTitle));

   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
   {
      (*table)(0,1) << Sub2(symbol(epsilon),"s") << rptNewLine << "x 1000";
      (*table)(0,2) << symbol(beta);
      (*table)(0,3) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   }
   else
   {
      (*table)(0,1) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*table)(0,2) << "v" << "/" << RPT_FC;
      (*table)(0,3) << symbol(epsilon) << Sub("x") << " x 1000";
      (*table)(0,4) << symbol(beta);
      (*table)(0,5) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,   pDisplayUnits->GetStressUnit(),          false );


   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      int col = 0;

      (*table)(row,col++) << location.SetValue( poi, end_size );

      if ( shear_capacity_method != scmBTEquations && shear_capacity_method != scmWSDOT2007 )
         (*table)(row,col++) << stress.SetValue( scd.fc );


      if ( bAfter1999 && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001) )
      {
         (*table)(row,col) << scalar.SetValue( scd.vfc );
         (*table)(row,col++) << " " << symbol(LTE) << " " << scalar.SetValue(scd.vfc_tbl);
      }
      else if ( shear_capacity_method != scmBTEquations && shear_capacity_method != scmWSDOT2007 )
      {
         (*table)(row,col++) << scalar.SetValue( scd.vfc );
      }

      if (scd.ShearInRange)
      {
         if( bAfter1999  && (shear_capacity_method == scmBTTables || shear_capacity_method == scmWSDOT2001) )
         {
            (*table)(row,col) << scalar.SetValue( scd.ex * 1000.0);
            (*table)(row,col++) << " " << symbol(LTE) << " " << scalar.SetValue( scd.ex_tbl * 1000.0 );
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
         (*table)(row,col++) << "*";
         (*table)(row,col++) << "*";
         (*table)(row,col++) << "*";
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << "* - Value could not be calculated. Shear crushing capacity of section exceeded"<< rptNewLine<<rptNewLine;
   }
}

void write_Vs_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName("Vs");

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << "Shear Resistance Provided By Shear Reinforcement - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;

   *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vs Equation.jpg") << rptNewLine;
   *pParagraph << rptNewLine;
   

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(8,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR( Sub2("A","v"), rptAreaUnitTag, pDisplayUnits->GetAreaUnit() );
   (*table)(0,2) << COLHDR( RPT_FY, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR( "s", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR( symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   (*table)(0,6) << COLHDR( symbol(alpha), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );
   (*table)(0,7) << COLHDR( Sub2("V","s"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area, pDisplayUnits->GetAreaUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << area.SetValue( scd.Av );
      (*table)(row,2) << stress.SetValue( scd.fy );
      (*table)(row,3) << dim.SetValue( scd.S );
      (*table)(row,4) << dim.SetValue( scd.dv );
      if (scd.ShearInRange)
      {
         (*table)(row,5) << angle.SetValue( scd.Theta );
         (*table)(row,6) << angle.SetValue( scd.Alpha );
         (*table)(row,7) << shear.SetValue( scd.Vs );
      }
      else
      {
         print_footnote=true;
         (*table)(row,5) << "*";
         (*table)(row,6) << "*";
         (*table)(row,7) << "*";
      }

      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << "* - Value could not be calculated. Shear crushing capacity of section exceeded"<< rptNewLine<<rptNewLine;
   }

}

void write_Vc_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName("Vc");

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << "Shear Resistance Provided By Tensile Stress in the Concrete - " << OLE2A(pStageMap->GetLimitStateName(ls)) << rptNewLine;

      if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vc Equation -SI.jpg") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vc Equation -US.jpg") << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << symbol(beta);
   (*table)(0,2) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR( Sub2("b","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,5) << COLHDR( Sub2("V","c"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,2) << stress.SetValue( scd.fc );
      (*table)(row,3) << dim.SetValue( scd.bv );
      (*table)(row,4) << dim.SetValue( scd.dv );
      if (scd.ShearInRange)
      {
         (*table)(row,1) << scalar.SetValue( scd.Beta );
         (*table)(row,5) << shear.SetValue( scd.Vc );
      }
      else
      {
         print_footnote=true;
         (*table)(row,1) << "*";
         (*table)(row,5) << "*";
      }
      row++;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
      *pChapter << pParagraph;
      *pParagraph << "* - Value could not be calculated. Shear crushing capacity of section exceeded"<< rptNewLine<<rptNewLine;
   }
}


void write_Vci_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName("Vci");

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << OLE2A(pStageMap->GetLimitStateName(ls)) << " - ";

   *pParagraph << "Shear Resistance Provided by Concrete when inclined cracking results from combined shear and moment" << rptNewLine;

      if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vci_SI.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vci_US.gif") << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR( Sub2("b","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,2) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR( Sub2("V","d"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,4) << COLHDR( Sub2("V","i"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << COLHDR( Sub2("M","max"), rptMomentUnitTag,  pDisplayUnits->GetMomentUnit() );
   (*table)(0,6) << COLHDR( Sub2("M","cre"), rptMomentUnitTag,  pDisplayUnits->GetMomentUnit() );

      if ( IS_SI_UNITS(pDisplayUnits) )
      (*table)(0,7) << COLHDR( Sub2("V","ci min") << rptNewLine << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vcimin_SI.gif"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,7) << COLHDR( Sub2("V","ci min") << rptNewLine << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vcimin_US.gif"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   (*table)(0,8) << COLHDR( Sub2("V","ci"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
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
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName("Vcw");

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << OLE2A(pStageMap->GetLimitStateName(ls)) << " - ";

   *pParagraph << "Shear Resistance Provided by Concrete when inclined cracking results from excessive principal tension in the web." << rptNewLine;

      if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vcw_SI.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "Vcw_US.gif") << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR(Sub2("f","pc"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(Sub2("b","v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,3) << COLHDR(Sub2("d","v"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,4) << COLHDR(Sub2("V","p"),  rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,5) << COLHDR(Sub2("V","cw"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << stress.SetValue( scd.fpc );
      (*table)(row,2) << dim.SetValue( scd.bv );
      (*table)(row,3) << dim.SetValue( scd.dv );
      (*table)(row,4) << shear.SetValue( scd.Vp );
      (*table)(row,5) << shear.SetValue( scd.Vcw );
      row++;
   }
}

void write_theta_table(IBroker* pBroker,
                       IDisplayUnits* pDisplayUnits,
                       SpanIndexType span,
                       GirderIndexType gdr,
                       const std::vector<pgsPointOfInterest>& pois,
                       rptChapter* pChapter,
                       pgsTypes::Stage stage,
                       const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   pParagraph->SetName("Angle of inclination of diagonal compressive stress");

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   *pParagraph << OLE2A(pStageMap->GetLimitStateName(ls)) << " - ";

   *pParagraph << "Angle of inclination of diagonal compressive stress [LRFD 5.8.3.3 and 5.8.3.4.3]" << rptNewLine;

      if ( IS_SI_UNITS(pDisplayUnits) )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "cotan_Theta_SI.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "cotan_Theta_US.gif") << rptNewLine;

   *pParagraph << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(6,"");
   *pParagraph << table << rptNewLine;

   if ( stage == pgsTypes::CastingYard )
      (*table)(0,0)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,0)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,1) << COLHDR(Sub2("V","ci"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,2) << COLHDR(Sub2("V","cw"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,3) << COLHDR(Sub2("f","pc"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,4) << "cot " << symbol(theta);
   (*table)(0,5) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetAngleUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue, shear, pDisplayUnits->GetShearUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, angle, pDisplayUnits->GetAngleUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,0) << location.SetValue( poi, end_size );
      (*table)(row,1) << shear.SetValue( scd.Vci );
      (*table)(row,2) << shear.SetValue( scd.Vcw );
      (*table)(row,3) << stress.SetValue( scd.fpc );
      (*table)(row,4) << scalar.SetValue( 1/tan(scd.Theta) );
      (*table)(row,5) << angle.SetValue(scd.Theta);
      row++;
   }
}

void write_Vn_table(IBroker* pBroker,
                    IDisplayUnits* pDisplayUnits,
                    SpanIndexType span,
                    GirderIndexType gdr,
                    const std::vector<pgsPointOfInterest>& pois,
                    rptChapter* pChapter,
                    pgsTypes::Stage stage,
                    const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   CString strName;
   strName.Format("Nominal Shear Resistance - %s",OLE2A(pStageMap->GetLimitStateName(ls)));
   pParagraph->SetName(strName);

   *pChapter << pParagraph;

   int nCol = (shear_capacity_method == scmVciVcw ? 10 : 11);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol,pParagraph->GetName());

   *pParagraph << table << rptNewLine;

   int col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,col++) << COLHDR( RPT_FC, rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( Sub2("b","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( shear_capacity_method != scmVciVcw )
      (*table)(0,col++) << COLHDR( Sub2("V","p"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   if ( shear_capacity_method == scmVciVcw )
      (*table)(0,col++) << COLHDR( Sub2("V","c") << Super("&"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   else
      (*table)(0,col++) << COLHDR( Sub2("V","c"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   (*table)(0,col++) << COLHDR( Sub2("V","s"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2("V","n1") << Super("$"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2("V","n2") << Super("#"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2("V","n"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( symbol(phi) << Sub2("V","n"), rptForceUnitTag, pDisplayUnits->GetShearUnit() );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   bool print_footnote=false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,col++) << location.SetValue( poi, end_size );
      (*table)(row,col++) << stress.SetValue( scd.fc );
      (*table)(row,col++) << dim.SetValue( scd.bv );
      (*table)(row,col++) << dim.SetValue( scd.dv );

      if ( shear_capacity_method != scmVciVcw )
         (*table)(row,col++) << shear.SetValue( scd.Vp );

      if (scd.ShearInRange)
      {
         (*table)(row,col++) << shear.SetValue( scd.Vc );
         (*table)(row,col++) << shear.SetValue( scd.Vs );
         (*table)(row,col++) << shear.SetValue( scd.Vn1 );
      }
      else
      {
         print_footnote=true;
         (*table)(row,col++) << "*";
         (*table)(row,col++) << "*";
         (*table)(row,col++) << "*";
      }

      (*table)(row,col++) << shear.SetValue( scd.Vn2 );
      (*table)(row,col++) << shear.SetValue( scd.Vn );
      (*table)(row,col++) << shear.SetValue( scd.pVn );

      row++;
   }

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetFootnoteStyle());
   *pChapter << pParagraph;

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << Super("&") << Sub2("V","c") << " = " << "min(" << Sub2("V","ci") << "," << Sub2("V","cw") << ")" << rptNewLine;
   }

   *pParagraph << Super("$") << Sub2("V","n1") << " = " << Sub2("V","c") << " + " << Sub2("V","s");

   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << " [Eqn 5.8.3.3-1 with " << Sub2("V","p") << " taken to be 0]" << rptNewLine;
   }
   else
   {
      *pParagraph << " + " << Sub2("V","p") << " [Eqn 5.8.3.3-1]"<< rptNewLine;
   }


   *pParagraph << Super("#") << Sub2("V","n2") << " = " << "0.25" << RPT_FC << Sub2("b","v") << Sub2("d","v");
   if ( shear_capacity_method == scmVciVcw )
   {
      *pParagraph << " [Eqn 5.8.3.3-2 with " << Sub2("V","p") << " taken to be 0]" << rptNewLine;
   }
   else
   {
      *pParagraph  << " + " << Sub2("V","p") << " [Eqn 5.8.3.3-2]"<< rptNewLine;
   }

   // print footnote if any values could not be calculated
   if (print_footnote)
      *pParagraph << "* - Value could not be calculated. Shear crushing capacity of section exceeded"<< rptNewLine<<rptNewLine;
}

void write_Avs_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                     SpanIndexType span,
                     GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   CString strLabel;
   strLabel.Format("Required Shear Reinforcement - %s",OLE2A(pStageMap->GetLimitStateName(ls)));

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName(strLabel);
   *pParagraph << pParagraph->GetName() << rptNewLine;
   *pChapter << pParagraph;

   if ( shear_capacity_method != scmVciVcw )
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "RequiredShearReinforcement1.gif");
   else
      *pParagraph << rptRcImage(pgsReportStyleHolder::GetImagePath() + "RequiredShearReinforcement2.gif");

   int nCol = (shear_capacity_method == scmVciVcw ? 8 : 9);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCol,std::string(strLabel));
   *pParagraph << table << rptNewLine;

   int col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,col++) << COLHDR( Sub2("V","u") << "/" << symbol(phi), rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2("V","c"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   if ( shear_capacity_method != scmVciVcw )
      (*table)(0,col++) << COLHDR( Sub2("V","p"), rptForceUnitTag,  pDisplayUnits->GetShearUnit() );

   (*table)(0,col++) << COLHDR( Sub2("V","s") << " *", rptForceUnitTag,  pDisplayUnits->GetShearUnit() );
   (*table)(0,col++) << COLHDR( Sub2("f","y"), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR( Sub2("d","v"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR( symbol(theta), rptAngleUnitTag,  pDisplayUnits->GetAngleUnit() );
   (*table)(0,col++) << COLHDR( Sub2("A","v") << "/S", rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );


   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),       false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,  shear,    pDisplayUnits->GetShearUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim,      pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,  angle,    pDisplayUnits->GetAngleUnit(),        false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs,      pDisplayUnits->GetAvOverSUnit(),      false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,col++) << location.SetValue( poi, end_size );
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
      *pParagraph << "* - Transverse reinforcement not required if " << Sub2("V","u") << " < 0.5" << symbol(phi) << Sub2("V","c");
      *pParagraph << " [Eqn 5.8.2.4-1 with " << Sub2("V","p") << " taken to be 0]" << rptNewLine;
   }
   else
   {
      *pParagraph << "* - Transverse reinforcement not required if " << Sub2("V","u") << " < 0.5" << symbol(phi) << "(" << Sub2("V","c");
      *pParagraph  << " + " << Sub2("V","p") << ") [Eqn 5.8.2.4-1]"<< rptNewLine;
   }
}

void write_bar_spacing_table(IBroker* pBroker,
                     IDisplayUnits* pDisplayUnits,
                      SpanIndexType span,
                      GirderIndexType gdr,
                     const std::vector<pgsPointOfInterest>& pois,
                     rptChapter* pChapter,
                     pgsTypes::Stage stage,
                     const std::string& strStageName,pgsTypes::LimitState ls)
{
   USES_CONVERSION;
   GET_IFACE2(pBroker,IGirder,pGirder);
   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   rptParagraph* pParagraph;

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   CString strLabel;
   strLabel.Format("Required Stirrup Spacing - %s",OLE2A(pStageMap->GetLimitStateName(ls)));

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   pParagraph->SetName(strLabel);
   *pParagraph << pParagraph->GetName() << rptNewLine;
   *pChapter << pParagraph;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,"Required Stirrup Spacing");
   *pParagraph << table << rptNewLine;

   int col = 0;
   if ( stage == pgsTypes::CastingYard )
      (*table)(0,col++)  << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
   else
      (*table)(0,col++)  << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

   (*table)(0,col++) << COLHDR( Sub2("A","v") << "/S", rptLengthUnitTag, pDisplayUnits->GetAvOverSUnit() );

   long nLegs = 2;

   GET_IFACE2(pBroker,IShear,pShear);
   CShearData shearData = pShear->GetShearData(span,gdr);

   if ( 0 < shearData.ShearZones.size() )
   {
      nLegs = shearData.ShearZones[0].nVertBars;
   }

   std::ostringstream os3;
   os3 << nLegs << "-#3";

   (*table)(0,col++) << COLHDR( os3.str() << rptNewLine << "S", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   std::ostringstream os4;
   os4 << nLegs << "-#4";

   (*table)(0,col++) << COLHDR( os4.str() << rptNewLine << "S", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   std::ostringstream os5;
   os5 << nLegs << "-#5";

   (*table)(0,col++) << COLHDR( os5.str() << rptNewLine << "S", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );


   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spacing,  pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, avs,      pDisplayUnits->GetAvOverSUnit(),      false );

   lrfdRebarPool* pRebarPool = lrfdRebarPool::GetInstance();

   double Ab3 = pRebarPool->GetRebar(3)->GetNominalArea();
   double Ab4 = pRebarPool->GetRebar(4)->GetNominalArea();
   double Ab5 = pRebarPool->GetRebar(5)->GetNominalArea();

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   Int16 row = 1;
   GET_IFACE2(pBroker,IShearCapacity,pShearCap);
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = pois.begin(); i != pois.end(); i++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *i;
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails(ls,stage,poi,&scd);

      (*table)(row,col++) << location.SetValue( poi, end_size );
      (*table)(row,col++) << avs.SetValue( scd.AvOverS_Reqd );

      if ( !IsZero(scd.AvOverS_Reqd) )
      {
         double S3 = nLegs*Ab3/scd.AvOverS_Reqd;
         double S4 = nLegs*Ab4/scd.AvOverS_Reqd;
         double S5 = nLegs*Ab5/scd.AvOverS_Reqd;

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
