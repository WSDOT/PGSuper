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

#include "StdAfx.h"
#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\PrestressRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>

#include <PgsExt\PierData2.h>

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
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey( pGirderRptSpec->GetGirderKey() );

   // we want the final configuration... that would be in the last interval
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(girderKey);

   GET_IFACE2(pBroker,IBearingDesign,pBearingDesign);

   bool bIncludeImpact = pBearingDesign->BearingLiveLoadReactionsIncludeImpact();

   // Don't create much of report if no simple span ends
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(intervalIdx,girderKey);
   bool doFinalLoads = (0 < vPiers.size() ? true : false);

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   GET_IFACE2(pBroker,ISpecification,pSpec);

   // Product Reactions
   rptParagraph* p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,girderKey,pSpec->GetAnalysisType(),BearingReactionsTable, bIncludeImpact,true,false,true,pDisplayUnits) << rptNewLine;

   if( doFinalLoads )
   {
      *p << (bIncludeImpact ? LIVELOAD_PER_LANE : LIVELOAD_PER_LANE_NO_IMPACT) << rptNewLine;

      if (bPedestrian)
      {
         *p << _T("$ Pedestrian values are per girder") << rptNewLine;
      }
   }
   else
   {
      rptParagraph* p = new rptParagraph;
      *p << _T("Final Bearing Reactions are not available if neither end of girder is simply supported") << rptNewLine;
      *pChapter << p;
      return pChapter;
   }

   *p << rptNewLine;

   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girderKey);
   std::vector<std::_tstring>::iterator iter;
   long j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << _T("(D") << j << _T(") ") << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girderKey);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(P") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girderKey);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(F") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,girderKey,pSpec->GetAnalysisType(),BearingReactionsTable,intervalIdx,pDisplayUnits) << rptNewLine;
   }

   // Combined reactions
   CCombinedReactionTable().BuildForBearingDesign(pBroker,pChapter,girderKey,pDisplayUnits,intervalIdx,pSpec->GetAnalysisType(),bIncludeImpact);

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,girderKey,pSpec->GetAnalysisType(), bIncludeImpact,true,true,true,true,pDisplayUnits) << rptNewLine;
   *p << (bIncludeImpact ? LIVELOAD_PER_GIRDER : LIVELOAD_PER_GIRDER_NO_IMPACT) << rptNewLine;

   if (bPedestrian)
   {
      *p << _T("$ Pedestrian values are per girder") << rptNewLine;
   }

   *p << rptNewLine;

   strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girderKey);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << _T("(D") << j << _T(") ") << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girderKey);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(P") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girderKey);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << _T("(F") << j << _T(") ") << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << rptNewLine;
      *p << CUserRotationTable().Build(pBroker,girderKey,pSpec->GetAnalysisType(),intervalIdx,pDisplayUnits) << rptNewLine;
   }

   *p << rptNewLine;
   *p << CPrestressRotationTable().Build(pBroker, girderKey, pSpec->GetAnalysisType(), intervalIdx, pDisplayUnits) << rptNewLine;

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPierIdx,endPierIdx;
   pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   // TRICKY: use adapter class to get correct reaction interfaces
   std::unique_ptr<IProductReactionAdapter> pForces(std::make_unique<BearingDesignProductReactionAdapter>(pBearingDesign, intervalIdx, girderKey) );
   
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(),    false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,    rotation, pDisplayUnits->GetRadAngleUnit(), false );

   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(9,_T("Corresponding Live Load Bearing Reactions and Rotations"));
   *p << pTable << rptNewLine;
   *p << (bIncludeImpact ? LIVELOAD_PER_GIRDER : LIVELOAD_PER_GIRDER_NO_IMPACT) << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;

   pTable->SetRowSpan(0,col,2);
   (*pTable)(0, col++) << _T("");

   pTable->SetColumnSpan(0,col,4);
   (*pTable)(0, col) << _T("* Reactions");
   (*pTable)(1, col++) << COLHDR(Sub2(_T("R"), _T("Max")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1, col++) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(_T("R"), _T("Min")), rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1, col++) << COLHDR(symbol(theta), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());

   pTable->SetColumnSpan(0,col,4);
   (*pTable)(0, col) << _T("* Rotations");
   (*pTable)(1, col++) << COLHDR(Sub2(symbol(theta),_T("Max")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1, col++) << COLHDR(_T("R"),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1, col++) << COLHDR(Sub2(symbol(theta),_T("Min")),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1, col++) << COLHDR(_T("R"),rptForceUnitTag, pDisplayUnits->GetShearUnit());


   // First get primary reaction with corresp rotations and primary rotations with corresp reactions
   std::vector<Float64> vMaxReaction, vMaxReaction_Rotation; // max reaction and rotation that corresponds to max reaction
   std::vector<Float64> vMinReaction, vMinReaction_Rotation; // min reaction and rotation that corresponds to min reaction
   std::vector<Float64> vMaxRotation, vMaxRotation_Reaction; // max rotation and reaction that corresponds to max rotation
   std::vector<Float64> vMinRotation, vMinRotation_Reaction; // min rotation and reaction that corresponds to min rotation

   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      if (!pForces->DoReportAtPier(pierIdx, girderKey))
      {
         // Don't report pier if information is not available
         continue;
      }

      ReactionLocation location;
      location.PierIdx = pierIdx;
      location.GirderKey = girderKey;
      if ( pierIdx == startPierIdx )
      {
         location.Face = rftAhead;
      }
      else if ( pierIdx == endPierIdx )
      {
         location.Face = rftBack;
      }
      else
      {
         location.Face = rftMid;
      }

      if ( analysisType == pgsTypes::Envelope )
      {
         // reactions and corresponding rotations
         Float64 Rmin, Rmax, Tmin, Tmax;
         pBearingDesign->GetBearingLiveLoadReaction( intervalIdx, location, pgsTypes::lltDesign, pgsTypes::MaxSimpleContinuousEnvelope, bIncludeImpact, true, &Rmin, &Rmax, &Tmin, &Tmax);
         vMaxReaction.push_back(Rmax);
         vMaxReaction_Rotation.push_back(Tmax);

         pBearingDesign->GetBearingLiveLoadReaction( intervalIdx, location, pgsTypes::lltDesign, pgsTypes::MinSimpleContinuousEnvelope, bIncludeImpact, true, &Rmin, &Rmax, &Tmin, &Tmax);
         vMinReaction.push_back(Rmin);
         vMinReaction_Rotation.push_back(Tmin);

         // rotations and corresponding reactions
         pBearingDesign->GetBearingLiveLoadRotation( intervalIdx, location, pgsTypes::lltDesign, pgsTypes::MaxSimpleContinuousEnvelope, bIncludeImpact, true, &Tmin, &Tmax, &Rmin, &Rmax);
         vMaxRotation.push_back(Tmax);
         vMaxRotation_Reaction.push_back(Rmax);

         pBearingDesign->GetBearingLiveLoadRotation( intervalIdx, location, pgsTypes::lltDesign, pgsTypes::MinSimpleContinuousEnvelope, bIncludeImpact, true, &Tmin, &Tmax, &Rmin, &Rmax);
         vMinRotation.push_back(Tmin);
         vMinRotation_Reaction.push_back(Rmin);
      }
      else
      {
         pgsTypes::BridgeAnalysisType bat = analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan;

         Float64 Rmin, Rmax, Tmin, Tmax;
         pBearingDesign->GetBearingLiveLoadReaction( intervalIdx, location, pgsTypes::lltDesign, bat, bIncludeImpact, true, &Rmin, &Rmax, &Tmin, &Tmax);
         vMinReaction.push_back(Rmin);
         vMinReaction_Rotation.push_back(Tmin);
         vMaxReaction.push_back(Rmax);
         vMaxReaction_Rotation.push_back(Tmax);

         pBearingDesign->GetBearingLiveLoadRotation( intervalIdx, location, pgsTypes::lltDesign, bat, bIncludeImpact, true, &Tmin, &Tmax, &Rmin, &Rmax);
         vMinRotation.push_back(Tmin);
         vMinRotation_Reaction.push_back(Rmin);
         vMaxRotation.push_back(Tmax);
         vMaxRotation_Reaction.push_back(Rmax);
      }
   }

   // Write table values
   RowIndexType row = pTable->GetNumberOfHeaderRows();

   int i = 0;
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      col = 0;

      if (!pForces->DoReportAtPier(pierIdx, girderKey))
      {
         // Don't report pier if information is not available
         continue;
      }

      (*pTable)(row,col++) << LABEL_PIER_EX(pBridge->IsAbutment(pierIdx),pierIdx);

      (*pTable)(row,col++) << reaction.SetValue( vMaxReaction[i] );
      (*pTable)(row,col++) << rotation.SetValue( vMaxReaction_Rotation[i] );

      (*pTable)(row,col++) << reaction.SetValue( vMinReaction[i] );
      (*pTable)(row,col++) << rotation.SetValue( vMinReaction_Rotation[i] );

      (*pTable)(row,col++) << rotation.SetValue( vMaxRotation[i] );
      (*pTable)(row,col++) << reaction.SetValue( vMaxRotation_Reaction[i] );

      (*pTable)(row,col++) << rotation.SetValue( vMinRotation[i] );
      (*pTable)(row,col++) << reaction.SetValue( vMinRotation_Reaction[i] );

      row++;
      i++;
   }

   ///////////////////////////////////////

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   GET_IFACE2(pBroker, IGirder, pGirder);

   p = new rptParagraph;
   *pChapter << p;

   pTable = rptStyleManager::CreateDefaultTable(9,_T("Bearing Recess Geometry"));

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( recess_dimension, IS_US_UNITS(pDisplayUnits), 8, RoundOff, pDisplayUnits->GetComponentDimUnit(), false, true );

   col = 0;

   (*pTable)(0,col++) << _T("");
   (*pTable)(0,col++) << _T("Girder") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,col++) << _T("Excess") << rptNewLine << _T("Camber") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,col++) << _T("Bearing") << rptNewLine << _T("Recess") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,col++) << _T("* Transverse") << rptNewLine << _T("Bearing") << rptNewLine << _T("Slope") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,col++) << _T("** W") << rptNewLine << _T("Recess") << rptNewLine << _T("Length");
   (*pTable)(0,col++) << _T("D") << rptNewLine << _T("Recess") << rptNewLine << _T("Height");
   (*pTable)(0,col++) << Sub2(_T("D"),_T("1"));
   (*pTable)(0,col++) << Sub2(_T("D"),_T("2"));

   row = pTable->GetNumberOfHeaderRows();

   bool bCheckTaperedSolePlate;
   Float64 taperedSolePlateThreshold;
   pSpec->GetTaperedSolePlateRequirements(&bCheckTaperedSolePlate, &taperedSolePlateThreshold);
   bool bNeedTaperedSolePlateTransverse = false; // if bearing recess slope exceeds "taperedSolePlateThreshold", a tapered bearing plate is required per LRFD 14.8.2
   bool bNeedTaperedSolePlateLongitudinal = false; // if bearing recess slope exceeds "taperedSolePlateThreshold", a tapered bearing plate is required per LRFD 14.8.2

   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      col = 0;

      if (!pForces->DoReportAtPier(pierIdx, girderKey))
      {
         // Don't report pier if information is not available
         continue;
      }      

      (*pTable)(row,col++) << LABEL_PIER_EX(pBridge->IsAbutment(pierIdx),pierIdx);

      CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);

      Float64 slope1 = pBridge->GetSegmentSlope( segmentKey );
      (*pTable)(row,col++) << scalar.SetValue(slope1);

      pgsPointOfInterest poi;
      pgsTypes::PierFaceType pierFace(pgsTypes::Back);
      if ( pierIdx == startPierIdx )
      {
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
         poi = vPoi.front();

         pierFace = pgsTypes::Ahead;
      }
      else if ( pierIdx == endPierIdx )
      {
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
         poi = vPoi.front();
      }
      else
      {
         poi = pPoi->GetPierPointOfInterest(girderKey,pierIdx);
      }

      Float64 slope2 = pCamber->GetExcessCamberRotation(poi,CREEP_MAXTIME);
      (*pTable)(row,col++) << scalar.SetValue(slope2);

      const CBearingData2* pbd = pIBridgeDesc->GetBearingData(pierIdx, pierFace, girderKey.girderIndex);

      Float64 slope3 = slope1 + slope2;
      (*pTable)(row,col++) << scalar.SetValue(slope3);

      Float64 transverse_slope = pGirder->GetOrientation(segmentKey);
      (*pTable)(row, col++) << scalar.SetValue(transverse_slope);

      bNeedTaperedSolePlateLongitudinal = taperedSolePlateThreshold < fabs(slope3); // see lrfd 14.8.2
      bNeedTaperedSolePlateTransverse = taperedSolePlateThreshold < fabs(transverse_slope); // see lrfd 14.8.2

      Float64 W = max(pbd->RecessLength, pbd->Length); // don't allow recess to be shorter than bearing
      Float64 D = pbd->RecessHeight;
      Float64 D1 = D + W*slope3/2;
      Float64 D2 = D - W*slope3/2;

      (*pTable)(row,col++) << recess_dimension.SetValue(W);
      (*pTable)(row,col++) << recess_dimension.SetValue(D);
      (*pTable)(row,col++) << recess_dimension.SetValue(D1);
      (*pTable)(row,col++) << recess_dimension.SetValue(D2);

      row++;
   }

   *p << pTable << rptNewLine;
   *p << _T("* Orientation of the girder cross section with respect to vertical, zero indicates plumb and positive values indicate girder is rotated clockwise") << rptNewLine;
   *p << _T("** W is maximum of input bearing length and recess length") << rptNewLine;

   if (bCheckTaperedSolePlate && (bNeedTaperedSolePlateLongitudinal || bNeedTaperedSolePlateTransverse))
   {
      *p << bgcolor(rptRiStyle::Yellow);
      *p << _T("The inclination of the underside of the girder to the horizontal exceeds ") << taperedSolePlateThreshold << _T(" rad.");
      if (bNeedTaperedSolePlateLongitudinal && !bNeedTaperedSolePlateTransverse)
      {
         *p << _T(" in the longitudinal direction.");
      }
      else if (!bNeedTaperedSolePlateLongitudinal && bNeedTaperedSolePlateTransverse)
      {
         *p << _T(" in the transverse direction.");
      }
      else
      {
         ATLASSERT(bNeedTaperedSolePlateLongitudinal && bNeedTaperedSolePlateTransverse);
         *p << _T(" in the longitudinal and transverse directions.");
      }
      *p << _T(" Per LRFD 14.8.2 a tapered sole plate shall be used in order to provide a level surface.") << rptNewLine;
      *p << bgcolor(rptRiStyle::White);
   }
   *p << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("BearingRecessSlope.png")) << rptNewLine;

   return pChapter;
}

CChapterBuilder* CBearingDesignParametersChapterBuilder::Clone() const
{
   return new CBearingDesignParametersChapterBuilder;
}
