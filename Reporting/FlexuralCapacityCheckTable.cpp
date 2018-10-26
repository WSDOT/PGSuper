///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include <IFace\Bridge.h>
#include <IFace\DisplayUnits.h>
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
                                               IDisplayUnits* pDispUnit,
                                               pgsTypes::Stage stage,
                                               pgsTypes::LimitState ls,bool bPositiveMoment,bool* pbOverReinforced) const
{
   bool bOverReinforced = false;

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims );
   Uint16 nCols = c_over_de ? 9 : 6;

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nCols,"");

   std::string strLimitState( ls == pgsTypes::StrengthI ? "Strength I" : "Strength II" );

   if ( bPositiveMoment )
      p_table->TableLabel() << "Positive Moment Capacity for " << strLimitState << " Limit State";
   else
      p_table->TableLabel() << "Negative Moment Capacity for " << strLimitState << " Limit State";

   if ( stage == pgsTypes::BridgeSite1 )
      p_table->TableLabel() << " for Deck and Diaphragm Stage (Bridge Site 1) [5.7]";
   else
      p_table->TableLabel() << " for Final with Live Load Stage (Bridge Site 3) [5.7]";

   ColumnIndexType col = 0;
   CHECK( stage != pgsTypes::CastingYard ); // need to revise location label

   p_table->SetNumberOfHeaderRows(2);

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,-1);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDispUnit->GetSpanLengthUnit() );

   if ( c_over_de )
   {
      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,-1);
      (*p_table)(0,col++) << Sub2("c/d","e");

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,-1);
      (*p_table)(0,col++) << Sub2("c/d","e") << " Max";

      p_table->SetRowSpan(0,col,2);
      p_table->SetRowSpan(1,col,-1);
      (*p_table)(0,col++) << "Over" << rptNewLine << "Reinforced" << rptNewLine << "Status";
   }

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,-1);
   (*p_table)(0,col++) << COLHDR(Sub2("M","u"), rptMomentUnitTag, pDispUnit->GetMomentUnit() );


   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,-1);
   (*p_table)(0,col++) << COLHDR(symbol(phi) << Sub2("M","n"), rptMomentUnitTag, pDispUnit->GetMomentUnit() );

   p_table->SetRowSpan(0,col,2);
   p_table->SetRowSpan(1,col,-1);
   (*p_table)(0,col++) << COLHDR(symbol(phi) << "M" << Sub("n") << " Min", rptMomentUnitTag, pDispUnit->GetMomentUnit() );

   p_table->SetColumnSpan(0,col,2);
   p_table->SetColumnSpan(0,col+1,-1);
   (*p_table)(0,col) << "Status";

   (*p_table)(1,col++) << symbol(phi) << Sub2("M","n") << " Min " << symbol(LTE) << " " << symbol(phi) << Sub2("M","n") << rptNewLine << "(" << symbol(phi) << Sub2("M","n") << "/" << symbol(phi) << Sub2("M","n") << " Min)";
   (*p_table)(1,col++) << Sub2("M","u") << " " << symbol(LTE) << " " << symbol(phi) << Sub2("M","n") << rptNewLine << "(" << symbol(phi) << Sub2("M","n") << "/" << Sub2("M","u") << ")";

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnit->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDispUnit->GetMomentUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( pDispUnit->GetScalarFormat().Format );
   scalar.SetWidth( pDispUnit->GetScalarFormat().Width );
   scalar.SetPrecision( pDispUnit->GetScalarFormat().Precision );


   // Fill up the p_table
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);
      ///////////////////////////////////////////////////////////////////
   GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);
      ///////////////////////////////////////////////////////////////////

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);

   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest( stage, span, girder, POI_FLEXURECAPACITY | POI_TABULAR);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   if ( stage == pgsTypes::CastingYard )
      end_size = 0; // don't adjust if CY stage

   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      col = 0;
      const pgsPointOfInterest& poi = *i;

      // Skip POI at critical section for shear if we are reporting on BridgeSite1
      if ( stage == pgsTypes::BridgeSite1 && poi.HasAttribute(ls == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2) )
         continue;

      (*p_table)(row,col++) << location.SetValue( poi, end_size );

      const pgsFlexuralCapacityArtifact* pArtifact;
      if ( bPositiveMoment )
         pArtifact = gdrArtifact->GetPositiveMomentFlexuralCapacityArtifact( pgsFlexuralCapacityArtifactKey(stage,ls,poi.GetDistFromStart()) );
      else
         pArtifact = gdrArtifact->GetNegativeMomentFlexuralCapacityArtifact( pgsFlexuralCapacityArtifactKey(stage,ls,poi.GetDistFromStart()) );

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

      double Mu    = pArtifact->GetDemand();
      double Mr    = pArtifact->GetCapacity();
      double MrMin = pArtifact->GetMinCapacity() ;

      (*p_table)(row,col++) << moment.SetValue( Mu );
      (*p_table)(row,col++) << moment.SetValue( Mr );
      (*p_table)(row,col++) << moment.SetValue( MrMin );

      if ( !pArtifact->IsUnderReinforced() )
         (*p_table)(row,col) << RPT_PASS;
      else
         (*p_table)(row,col) << RPT_FAIL;

      if ( IsZero( MrMin ) )
      {
         (*p_table)(row,col++) << rptNewLine << "(" << symbol(INFINITY) << ")";
      }
      else
      {
         (*p_table)(row,col++) << rptNewLine << "(" << scalar.SetValue(Mr/MrMin) << ")";
      }

      if ( pArtifact->CapacityPassed() )
         (*p_table)(row,col) << RPT_PASS;
      else
         (*p_table)(row,col) << RPT_FAIL;

      if ( IsZero( Mu ) )
      {
         (*p_table)(row,col++) << rptNewLine << "(" << symbol(INFINITY) << ")";
      }
      else
      {
         (*p_table)(row,col++) << rptNewLine << "(" << scalar.SetValue(Mr/Mu) << ")";
      }

      if ( c_over_de )
      {
         if ( pArtifact->GetMaxReinforcementRatio() > pArtifact->GetMaxReinforcementRatioLimit() )
         {
            bOverReinforced = true;
            (*p_table)(row,8) << " *";

            // Show limiting capacity of over reinforced section
            MOMENTCAPACITYDETAILS mcd;
            pMomentCap->GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);
            (*p_table)(row,5) << rptNewLine << "(" << moment.SetValue( mcd.Phi * mcd.MnMin ) << ")";
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
   os << "Dump for CFlexuralCapacityCheckTable" << endl;
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
