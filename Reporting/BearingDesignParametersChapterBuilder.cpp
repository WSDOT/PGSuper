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
#include <Reporting\BearingDesignParametersChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DisplayUnits.h>

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
CBearingDesignParametersChapterBuilder::CBearingDesignParametersChapterBuilder()
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

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
   
   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);

   GET_IFACE2(pBroker,ISpecification,pSpec);


   rptParagraph* p = new rptParagraph;
   *pChapter << p;

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),false,true,true,true,true,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;
   *p << rptNewLine;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   std::vector<std::string> strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
   std::vector<std::string>::iterator iter;
   long j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDisplayUnits) << rptNewLine;
   }

   // Product Rotations
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),false,true,true,true,true,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;
   *p << rptNewLine;

   strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltDesign,girder);
   j = 0;
   for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
   {
      *p << "(D" << j << ") " << *iter << rptNewLine;
   }

   if ( bPermit )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltPermit,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(P" << j << ") " << *iter << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      strLLNames = pProductLoads->GetVehicleNames(pgsTypes::lltFatigue,girder);
      j = 0;
      for (iter = strLLNames.begin(); iter != strLLNames.end(); iter++, j++ )
      {
         *p << "(F" << j << ") " << *iter << rptNewLine;
      }
   }

   if (are_user_loads)
   {
      *p << rptNewLine;
      *p << CUserRotationTable().Build(pBroker,span,girder,pSpec->GetAnalysisType(),pDisplayUnits) << rptNewLine;
   }

   p = new rptParagraph;
   *pChapter << p;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,"Rotation due to Camber");
   *p << pTable << rptNewLine;


   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(7);
   scalar.SetPrecision(4);
   scalar.SetTolerance(1.0e-6);

   (*pTable)(0,0) << "";
   (*pTable)(0,1) << "Camber" << rptNewLine << "Rotation" << rptNewLine << "(rad)";

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPier = (span == ALL_SPANS ? 0 : span);
   PierIndexType endPier   = (span == ALL_SPANS ? nPiers : startPier+2 );

   GET_IFACE2(pBroker,ICamber,pCamber);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest(pgsTypes::BridgeSite3,span,girder,POI_SECTCHANGE);
   ATLASSERT( 2 <= vPoi.size() );

   PierIndexType pier = 0;
   for ( pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      pgsTypes::PierFaceType pierFace = (pier == startPier ? pgsTypes::Ahead : pgsTypes::Back );

      if ( pier == 0 || pier == nPiers-1 )
         (*pTable)(row,col++) << "Abutment " << LABEL_PIER(pier);
      else
         (*pTable)(row,col++) << "Pier " << LABEL_PIER(pier);

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
   GET_IFACE2(pBroker,IProductForces,pForces);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   INIT_UV_PROTOTYPE( rptForceSectionValue, reaction, pDisplayUnits->GetShearUnit(),    false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,    rotation, pDisplayUnits->GetRadAngleUnit(), false );


   p = new rptParagraph;
   *pChapter << p;

   pTable = pgsReportStyleHolder::CreateDefaultTable(9,"Corresponding Live Load Reactions and Rotations");
   *p << pTable << rptNewLine;
   *p << LIVELOAD_PER_GIRDER_NO_IMPACT << rptNewLine;

   pTable->SetNumberOfHeaderRows(2);

   pTable->SetRowSpan(0,0,2);
   pTable->SetRowSpan(1,0,-1);
   (*pTable)(0,0) << "";

   pTable->SetColumnSpan(0,1,4);
   (*pTable)(0,1) << "* Reactions";

   pTable->SetColumnSpan(0,2,4);
   (*pTable)(0,2) << "* Rotations";

   pTable->SetColumnSpan(0,3,-1);
   pTable->SetColumnSpan(0,4,-1);
   pTable->SetColumnSpan(0,5,-1);
   pTable->SetColumnSpan(0,6,-1);
   pTable->SetColumnSpan(0,7,-1);
   pTable->SetColumnSpan(0,8,-1);

   (*pTable)(1,1) << COLHDR(Sub2("R","Max"),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,2) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,3) << COLHDR(Sub2("R","Min"),rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,4) << COLHDR(symbol(theta),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());

   (*pTable)(1,5) << COLHDR(Sub2(symbol(theta),"Max"),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,6) << COLHDR("R",rptForceUnitTag, pDisplayUnits->GetShearUnit());
   (*pTable)(1,7) << COLHDR(Sub2(symbol(theta),"Min"),rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   (*pTable)(1,8) << COLHDR("R",rptForceUnitTag, pDisplayUnits->GetShearUnit());

   row = pTable->GetNumberOfHeaderRows();

   for ( pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      pgsTypes::PierFaceType pierFace = (pier == startPier ? pgsTypes::Ahead : pgsTypes::Back );

      if ( pier == 0 || pier == nPiers-1 )
         (*pTable)(row,col++) << "Abutment " << LABEL_PIER(pier);
      else
         (*pTable)(row,col++) << "Pier " << LABEL_PIER(pier);

      pgsPointOfInterest poi(startPier,girder,pBridge->GetGirderStartConnectionLength(span,girder));
      if ( pier != startPier )
         poi.SetDistFromStart( poi.GetDistFromStart() + pBridge->GetSpanLength(span,girder) );
   
      double Rmax,Rmin,Tmin,Tmax;
      long minConfig, maxConfig;
      if ( analysisType == pgsTypes::Envelope )
      {
         // reactions and corresponding rotations
         pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MaxSimpleContinuousEnvelope, false, true, &Rmin, &Rmax, &Tmin, &Tmax, &minConfig, &maxConfig );
         (*pTable)(row,col++) << reaction.SetValue( Rmax );
         (*pTable)(row,col++) << rotation.SetValue( Tmax );

         pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, MinSimpleContinuousEnvelope, false, true, &Rmin, &Rmax, &Tmin, &Tmax, &minConfig, &maxConfig  );
         (*pTable)(row,col++) << reaction.SetValue( Rmin );
         (*pTable)(row,col++) << rotation.SetValue( Tmin );

         // rotations and corresponding reaactions
         pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, pierFace, MaxSimpleContinuousEnvelope, false, true, &Tmin, &Tmax, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*pTable)(row,col++) << rotation.SetValue( Tmax );
         (*pTable)(row,col++) << reaction.SetValue( Rmax );

         pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, pierFace, MinSimpleContinuousEnvelope, false, true, &Tmin, &Tmax, &Rmin, &Rmax, &minConfig, &maxConfig  );
         (*pTable)(row,col++) << rotation.SetValue( Tmin );
         (*pTable)(row,col++) << reaction.SetValue( Rmin );
      }
      else
      {
         // reactions and corresponding rotations
         pForces->GetLiveLoadReaction( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, false, true, &Rmin, &Rmax, &Tmin, &Tmax, &minConfig, &maxConfig );
         (*pTable)(row,col++) << reaction.SetValue( Rmax );
         (*pTable)(row,col++) << rotation.SetValue( Tmax );

         (*pTable)(row,col++) << reaction.SetValue( Rmin );
         (*pTable)(row,col++) << rotation.SetValue( Tmin );

         // rotations and corresponding reactions
         pForces->GetLiveLoadRotation( pgsTypes::lltDesign, pgsTypes::BridgeSite3, pier, girder, pierFace, analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, false, true, &Tmin, &Tmax, &Rmin, &Rmax, &minConfig, &maxConfig );
         (*pTable)(row,col++) << rotation.SetValue( Tmax );
         (*pTable)(row,col++) << reaction.SetValue( Rmax );

         (*pTable)(row,col++) << rotation.SetValue( Tmin );
         (*pTable)(row,col++) << reaction.SetValue( Rmin );
      }

      row++;
   }

   ///////////////////////////////////////

   p = new rptParagraph;
   *pChapter << p;

   pTable = pgsReportStyleHolder::CreateDefaultTable(8,"Bearing Geometry (based on assumed values)");
   *p << pTable << rptNewLine;
   *p << "W and D are assumed typical values" << rptNewLine;
   *p << rptRcImage( pgsReportStyleHolder::GetImagePath() + "BearingRecessSlope.gif") << rptNewLine;

   std::string strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( recess_dimension, IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), false, true );

   (*pTable)(0,0) << "";
   (*pTable)(0,1) << "Girder" << rptNewLine << "Slope" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,2) << "Excess" << rptNewLine << "Camber" << rptNewLine << "Slope" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,3) << "Bearing" << rptNewLine << "Recess" << rptNewLine << "Slope" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,4) << "W";
   (*pTable)(0,5) << "D";
   (*pTable)(0,6) << Sub2("D","1");
   (*pTable)(0,7) << Sub2("D","2");

   row = pTable->GetNumberOfHeaderRows();

   for ( pier = startPier; pier < endPier; pier++ )
   {
      ColumnIndexType col = 0;

      pgsTypes::PierFaceType pierFace = (pier == startPier ? pgsTypes::Ahead : pgsTypes::Back );

      if ( pier == 0 || pier == nPiers-1 )
         (*pTable)(row,col++) << "Abutment " << LABEL_PIER(pier);
      else
         (*pTable)(row,col++) << "Pier " << LABEL_PIER(pier);


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
