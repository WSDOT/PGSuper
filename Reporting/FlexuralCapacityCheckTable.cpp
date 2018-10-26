///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CFlexuralCapacityCheckp_table
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CFlexuralCapacityCheckTable::CFlexuralCapacityCheckTable()
{
}

CFlexuralCapacityCheckTable::CFlexuralCapacityCheckTable(const CFlexuralCapacityCheckTable& rOther)
{
   MakeCopy(rOther);
}

CFlexuralCapacityCheckTable::~CFlexuralCapacityCheckTable()
{
}

//======================== OPERATORS  =======================================
CFlexuralCapacityCheckTable& CFlexuralCapacityCheckTable::operator= (const CFlexuralCapacityCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CFlexuralCapacityCheckTable::Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               pgsTypes::Stage stage,
                                               pgsTypes::LimitState ls,bool bPositiveMoment,bool* pbOverReinforced) const
{
   USES_CONVERSION;
   bool bOverReinforced = false;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims );
   Uint16 nCols = c_over_de ? 9 : 6;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T(""));


   if ( span == ALL_SPANS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   GET_IFACE2(pBroker,IStageMap,pStageMap);
   std::_tstring strLimitState = OLE2T(pStageMap->GetLimitStateName(ls));

   if ( bPositiveMoment )
      p_table->TableLabel() << _T("Positive Moment Capacity for ") << strLimitState << _T(" Limit State");
   else
      p_table->TableLabel() << _T("Negative Moment Capacity for ") << strLimitState << _T(" Limit State");

   if ( stage == pgsTypes::BridgeSite1 )
      p_table->TableLabel() << _T(" for Deck and Diaphragm Stage (Bridge Site 1) [5.7]");
   else
      p_table->TableLabel() << _T(" for Final with Live Load Stage (Bridge Site 3) [5.7]");

   ColumnIndexType col = 0;
   CHECK( stage != pgsTypes::CastingYard ); // need to revise location label

   p_table->SetNumberOfHeaderRows(2);

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   if ( c_over_de )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << Sub2(_T("c/d"),_T("e"));

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << Sub2(_T("c/d"),_T("e")) << _T(" Max");

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,SKIP_CELL);
      (*p_table)(0,col++) << _T("Over") << rptNewLine << _T("Reinforced") << rptNewLine << _T("Status");
   }

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << COLHDR(Sub2(_T("M"),_T("u")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );


   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << COLHDR(symbol(phi) << Sub2(_T("M"),_T("n")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,SKIP_CELL);
   (*p_table)(0,col++) << COLHDR(symbol(phi) << _T("M") << Sub(_T("n")) << _T(" Min"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );

   p_table->SetColumnSpan(0,col,2);
   p_table->SetColumnSpan(0,col+1,SKIP_CELL);
   (*p_table)(0,col) << _T("Status");

   (*p_table)(1,col++) << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min ") << symbol(LTE) << _T(" ") << symbol(phi) << Sub2(_T("M"),_T("n")) << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("M"),_T("n")) << _T("/") << symbol(phi) << Sub2(_T("M"),_T("n")) << _T(" Min)");
   (*p_table)(1,col++) << Sub2(_T("M"),_T("u")) << _T(" ") << symbol(LTE) << _T(" ") << symbol(phi) << Sub2(_T("M"),_T("n")) << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("M"),_T("n")) << _T("/") << Sub2(_T("M"),_T("u")) << _T(")");

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);

   rptRcScalar scalar;
   scalar.SetFormat( pDisplayUnits->GetScalarFormat().Format );
   scalar.SetWidth( pDisplayUnits->GetScalarFormat().Width );
   scalar.SetPrecision( pDisplayUnits->GetScalarFormat().Precision );

   rptCapacityToDemand cap_demand;

   // Fill up the p_table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
      ///////////////////////////////////////////////////////////////////
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);
      ///////////////////////////////////////////////////////////////////

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsFlexuralCapacityArtifactKey> keys = gdrArtifact->GetFlexuralCapacityArtifactKeys();
   std::vector<pgsFlexuralCapacityArtifactKey>::iterator iter;
   for ( iter = keys.begin(); iter != keys.end(); iter++ )
   {
      col = 0;
      pgsFlexuralCapacityArtifactKey key = *iter;

      if ( key.GetStage() != stage || key.GetLimitState() != ls )
         continue;

      const pgsPointOfInterest& poi = pIPoi->GetPointOfInterest( key.GetStage(), span, girder, key.GetDistFromStart() );

      // Skip POI at critical section for shear if we are reporting on BridgeSite1
      if ( stage == pgsTypes::BridgeSite1 && poi.HasAttribute(pgsTypes::BridgeSite3, ls == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2) )
         continue;

      (*p_table)(row,col++) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

      const pgsFlexuralCapacityArtifact* pArtifact;
      if ( bPositiveMoment )
         pArtifact = gdrArtifact->GetPositiveMomentFlexuralCapacityArtifact( key );
      else
         pArtifact = gdrArtifact->GetNegativeMomentFlexuralCapacityArtifact( key );

      if ( c_over_de )
      {
         (*p_table)(row,col++) << scalar.SetValue( pArtifact->GetMaxReinforcementRatio() );
         (*p_table)(row,col++) << scalar.SetValue( pArtifact->GetMaxReinforcementRatioLimit() );
         if ( pArtifact->IsOverReinforced() )
         {
            (*p_table)(row,col++) << RPT_FAIL;
         }
         else
         {
            (*p_table)(row,col++) << RPT_PASS;
         }
      }

      Float64 Mu    = pArtifact->GetDemand();
      Float64 Mr    = pArtifact->GetCapacity();
      Float64 MrMin = pArtifact->GetMinCapacity() ;

      (*p_table)(row,col++) << moment.SetValue( Mu );
      (*p_table)(row,col++) << moment.SetValue( Mr );
      (*p_table)(row,col++) << moment.SetValue( MrMin );

      bool bPassed = !pArtifact->IsUnderReinforced();
      if ( bPassed )
         (*p_table)(row,col) << RPT_PASS;
      else
         (*p_table)(row,col) << RPT_FAIL;

      (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(Mr,MrMin,bPassed) << _T(")");

      bPassed = pArtifact->CapacityPassed();
      if ( bPassed )
         (*p_table)(row,col) << RPT_PASS;
      else
         (*p_table)(row,col) << RPT_FAIL;

      (*p_table)(row,col++) << rptNewLine << _T("(") << cap_demand.SetValue(Mr,Mu,bPassed) << _T(")");

      if ( c_over_de )
      {
         if ( pArtifact->GetMaxReinforcementRatio() > pArtifact->GetMaxReinforcementRatioLimit() )
         {
            bOverReinforced = true;
            (*p_table)(row,8) << _T(" *");

            // Show limiting capacity of over reinforced section
            MOMENTCAPACITYDETAILS mcd;
            pMomentCap->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);
            (*p_table)(row,5) << rptNewLine << _T("(") << moment.SetValue( mcd.Phi * mcd.MnMin ) << _T(")");
         }
      }

      row++;
   }

   *pbOverReinforced = bOverReinforced;
   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CFlexuralCapacityCheckTable::MakeCopy(const CFlexuralCapacityCheckTable& rOther)
{
   // Add copy code here...
}

void CFlexuralCapacityCheckTable::MakeAssignment(const CFlexuralCapacityCheckTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CFlexuralCapacityCheckTable::AssertValid() const
{
   return true;
}

void CFlexuralCapacityCheckTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CFlexuralCapacityCheckTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CFlexuralCapacityCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CFlexuralCapacityCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CFlexuralCapacityCheckTable");

   TESTME_EPILOG("CFlexuralCapacityCheckTable");
}
#endif // _UNITTEST
