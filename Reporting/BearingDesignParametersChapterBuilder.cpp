///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBearingDesignParametersChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBearingDesignParametersChapterBuilder::CBearingDesignParametersChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CBearingDesignParametersChapterBuilder::GetName() const
{
   return TEXT("Bearing Design Parameters");
}

rptChapter* CBearingDesignParametersChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   ATLASSERT(span!=ALL_SPANS); // This report is not capable if reporting girderline results

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(span, girder);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);

   GET_IFACE2(pBroker,IBearingDesign,pBearing);

   // Don't create report if no simple span ends
   bool doStartPier, doEndPier;
   pBearing->AreBearingReactionsAvailable(span, girder, &doStartPier, &doEndPier);
   if( !(doStartPier || doEndPier) )
   {
      rptParagraph* p = new rptParagraph;
      *p << _T("Bearing Reactions are not available if neither end of girder is simply supported") << rptNewLine;
      *pChapter << p;
      return pChapter;
   }

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   GET_IFACE2(pBroker,ISpecification,pSpec);

   // Product Reactions
   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),CProductReactionTable::BearingReactionsTable,false,true,true,false,true,pDisplayUnits) << rptNewLine;

   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;

   if (bPedestrian)
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << rptNewLine;

   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
   std::vector<std::_tstring>::iterator iter;
   long j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << _T("(D") << j << _T(") ") << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(P") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(F") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),CUserReactionTable::BearingReactionsTable,pDisplayUnits) << rptNewLine;
   }

   // Combined reactions
   CCombinedReactionTable().BuildForBearingDesign(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3,pSpec->GetAnalysisType());

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),false,true,true,true,true,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;

   if (bPedestrian)
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;

   *p << rptNewLine;

   strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << _T("(D") << j << _T(") ") << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(P") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(F") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << rptNewLine;
      *p << CUserRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDisplayUnits) << rptNewLine;
   }

   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,_T("Rotation due to Camber"));
   *p << pTable << rptNewLine;


   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(7);
   scalar.SetPrecision(4);
   scalar.SetTolerance(1.0e-6);

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << _T("Camber") << rptNewLine << _T("Rotation") << rptNewLine << _T("(rad)");

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPier = (span == ALL_SPANS ? 0 : span); // Make ALL_SPAN not crash. Assert above should stop this
   PierIndexType endPier   = startPier+2;

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_SECTCHANGE,POIFIND_OR);
   ATLASSERT( 2 <= vPoi.size() );

   PierIndexType pier = 0;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      pgsTypes::PierFaceType pierFace = (pier == startPier ? pgsTypes::Ahead : pgsTypes::Back );

      if ( pier == 0 || pier == nPiers-1 )
         (*pTable)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*pTable)(row,col++) << _T("Pier ") << LABEL_PIER(pier);

      pgsPointOfInterest poi;
      if ( pier == startPier )
      {
         poi = vPoi.front();
      }
      else
      {
         poi = vPoi.back();
      }

      double rotation = pCamber->GetExcessCamberRotation(poi,CREEP_MAXTIME);
      (*pTable)(row,col++) << scalar.SetValue(rotation);

      row++;
   }


   //////////////////////
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(),    false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,    rotation, pDisplayUnits->GetRadAngleUnit(), false );

   p = new rptParagraph;
   *pChapter << p;

   pTable = pgsReportStyleHolder::CreateDefaultTable(9,_T("Corresponding Live Load Bearing Reactions and Rotations"));
   *p << pTable << rptNewLine;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);

   pTable->SetRowSpan(0,0,2);
   pTable->SetRowSpan(1,0,SKIP_CELL);
   (*pTable)(0,0) << _T("");

   pTable->SetColumnSpan(0,1,4);
   (*pTable)(0,1) << _T("* Reactions");

   pTable->SetColumnSpan(0,2,4);
   (*pTable)(0,2) << _T("* Rotations");

   pTable->SetColumnSpan(0,3,SKIP_CELL);
   pTable->SetColumnSpan(0,4,SKIP_CELL);
   pTable->SetColumnSpan(0,5,SKIP_CELL);
   pTable->SetColumnSpan(0,6,SKIP_CELL);
   pTable->SetColumnSpan(0,7,SKIP_CELL);
   pTable->SetColumnSpan(0,8,SKIP_CELL);

   (*pTable)(1,1) << COLHDR(Sub2(_T("R"),_T("Max")),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,2) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,3) << COLHDR(Sub2(_T("R"),_T("Min")),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,4) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());

   (*pTable)(1,5) << COLHDR(Sub2(symbol(theta),_T("Max")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,6) << COLHDR(_T("R"),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,7) << COLHDR(Sub2(symbol(theta),_T("Min")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,8) << COLHDR(_T("R"),rptForceUnitTag, pDisplayUnits->GetShearUnit());


   // First get primary reaction with corresp rotations and primary rotations with corresp reactions
   Float64 startRPmax,startRPmin,startTCmin,startTCmax;
   Float64 startRCmax,startRCmin,startTPmin,startTPmax;
   Float64 endRPmax,endRPmin,endTCmin,endTCmax;
   Float64 endRCmax,endRCmin,endTPmin,endTPmax;
   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 fdummy;

      // reactions and corresponding rotations
      pBearing->GetBearingLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, MaxSimpleContinuousEnvelope, false, true,
                                           &fdummy, &startRPmax, &fdummy, &startTCmax, &fdummy, &endRPmax, &fdummy, &endTCmax,
                                           NULL, NULL, NULL, NULL);

      pBearing->GetBearingLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, MinSimpleContinuousEnvelope, false, true,
                                           &startRPmin, &fdummy, &startTCmin, &fdummy, &endRPmin, &fdummy, &endTCmin, &fdummy,
                                           NULL, NULL, NULL, NULL);

      // rotations and corresponding reactions
      pBearing->GetBearingLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, MaxSimpleContinuousEnvelope, false, true,
                                           &fdummy, &startTPmax, &fdummy, &startRCmax, &fdummy, &endTPmax, &fdummy, &endRCmax,
                                           NULL, NULL, NULL, NULL);

      pBearing->GetBearingLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, MinSimpleContinuousEnvelope, false, true,
                                           &startTPmin, &fdummy, &startRCmin, &fdummy, &endTPmin, &fdummy, &endRCmin, &fdummy,
                                           NULL, NULL, NULL, NULL);
   }
   else
   {
      BridgeAnalysisType batype = analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan;

      pBearing->GetBearingLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, batype, false, true,
                                           &startRPmin, &startRPmax, &startTCmin, &startTCmax, &endRPmin, &endRPmax, &endTCmin, &endTCmax,
                                           NULL, NULL, NULL, NULL);

      pBearing->GetBearingLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, girder, batype, false, true,
                                           &startTPmin, &startTPmax, &startRCmin, &startRCmax, &endTPmin, &endTPmax, &endRCmin, &endRCmax,
                                           NULL, NULL, NULL, NULL);
   }

   // Write table values
   row = pTable->GetNumberOfHeaderRows();
   ColumnIndexType col = 0;

   if(doStartPier)
   {
      if ( startPier == 0 || startPier == nPiers-1 )
         (*pTable)(row,col++) << _T("Abutment ") << LABEL_PIER(startPier);
      else
         (*pTable)(row,col++) << _T("Pier ") << LABEL_PIER(startPier);

      (*pTable)(row,col++) << reaction.SetValue( startRPmax );
      (*pTable)(row,col++) << rotation.SetValue( startTCmax );

      (*pTable)(row,col++) << reaction.SetValue( startRPmin );
      (*pTable)(row,col++) << rotation.SetValue( startTCmin );

      (*pTable)(row,col++) << rotation.SetValue( startTPmax );
      (*pTable)(row,col++) << reaction.SetValue( startRCmax );

      (*pTable)(row,col++) << rotation.SetValue( startTPmin );
      (*pTable)(row,col++) << reaction.SetValue( startRCmin );

      row++;
   }

   if(doEndPier)
   {
      col = 0;

      if ( endPier == 0 || endPier == nPiers )
         (*pTable)(row,col++) << _T("Abutment ") << LABEL_PIER(endPier-1);
      else
         (*pTable)(row,col++) << _T("Pier ") << LABEL_PIER(endPier-1);

      (*pTable)(row,col++) << reaction.SetValue( endRPmax );
      (*pTable)(row,col++) << rotation.SetValue( endTCmax );

      (*pTable)(row,col++) << reaction.SetValue( endRPmin );
      (*pTable)(row,col++) << rotation.SetValue( endTCmin );

      (*pTable)(row,col++) << rotation.SetValue( endTPmax );
      (*pTable)(row,col++) << reaction.SetValue( endRCmax );

      (*pTable)(row,col++) << rotation.SetValue( endTPmin );
      (*pTable)(row,col++) << reaction.SetValue( endRCmin );
   }

   ///////////////////////////////////////

   p = new rptParagraph;
   *pChapter << p;

   pTable = pgsReportStyleHolder::CreateDefaultTable(8,_T("Bearing Geometry (based on assumed values)"));
   *p << pTable << rptNewLine;
   *p << _T("W and D are assumed typical values") << rptNewLine;
   *p << rptRcImage( pgsReportStyleHolder::GetImagePath() + _T("BearingRecessSlope.gif")) << rptNewLine;

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( recess_dimension, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), false, true );

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << _T("Girder") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,2) << _T("Excess") << rptNewLine << _T("Camber") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,3) << _T("Bearing") << rptNewLine << _T("Recess") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,4) << _T("W");
   (*pTable)(0,5) << _T("D");
   (*pTable)(0,6) << Sub2(_T("D"),_T("1"));
   (*pTable)(0,7) << Sub2(_T("D"),_T("2"));

   row = pTable->GetNumberOfHeaderRows();

   for ( pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      pgsTypes::PierFaceType pierFace = (pier == startPier ? pgsTypes::Ahead : pgsTypes::Back );

      if ( pier == 0 || pier == nPiers-1 )
         (*pTable)(row,col++) << _T("Abutment ") << LABEL_PIER(pier);
      else
         (*pTable)(row,col++) << _T("Pier ") << LABEL_PIER(pier);


      double slope1 = pBridge->GetGirderSlope(span,girder);
      (*pTable)(row,col++) << scalar.SetValue(slope1);

      
      pgsPointOfInterest poi;
      if ( pier == startPier )
      {
         poi = vPoi.front();
      }
      else
      {
         poi = vPoi.back();
      }

      double slope2 = pCamber->GetExcessCamberRotation(poi,CREEP_MAXTIME);
      (*pTable)(row,col++) << scalar.SetValue(slope2);

      double slope3 = slope1 + slope2;
      (*pTable)(row,col++) << scalar.SetValue(slope3);

      double W = ::ConvertToSysUnits(12.0,unitMeasure::Inch);
      double D = ::ConvertToSysUnits(0.50,unitMeasure::Inch);
      double D1 = D + W*slope3/2;
      double D2 = D - W*slope3/2;

      (*pTable)(row,col++) << recess_dimension.SetValue(W);
      (*pTable)(row,col++) << recess_dimension.SetValue(D);
      (*pTable)(row,col++) << recess_dimension.SetValue(D1);
      (*pTable)(row,col++) << recess_dimension.SetValue(D2);


      row++;
   }

   return pChapter;
}

CChapterBuilder* CBearingDesignParametersChapterBuilder::Clone() const
{
   return new CBearingDesignParametersChapterBuilder;
}
