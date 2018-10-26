///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\ClosureJointData.h>

#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>


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
rptRcTable* CFlexuralCapacityCheckTable::Build(IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                                               IEAFDisplayUnits* pDisplayUnits,
                                               IntervalIndexType intervalIdx,
                                               pgsTypes::LimitState ls,bool bPositiveMoment,bool* pbOverReinforced) const
{
   bool bOverReinforced = false;

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims );
   Uint16 nCols = c_over_de ? 9 : 6;

   rptRcTable* p_table = rptStyleManager::CreateDefaultTable(nCols,_T(""));

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   std::_tstring strLimitState = GetLimitStateString(ls);
   std::_tstring strDirection  = (bPositiveMoment ? _T("Positive") : _T("Negative"));

   std::_tostringstream os;
   os << strDirection << _T(" Moment Capacity for ") << strLimitState << _T(" Limit State [5.7]");
   p_table->TableLabel() << os.str().c_str();

   ColumnIndexType col = 0;

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
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   rptCapacityToDemand cdRatio;

   // Fill up the p_table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   // report all the artifacts there were created.
   CollectionIndexType nArtifacts = pGirderArtifact->GetFlexuralCapacityArtifactCount(intervalIdx,ls);
   ATLASSERT(0 < nArtifacts); // why aren't there any capacity artifacts?
   for (CollectionIndexType artifactIdx = 0; artifactIdx < nArtifacts; artifactIdx++ )
   {
      col = 0;

      const pgsFlexuralCapacityArtifact* pArtifact;
      if ( bPositiveMoment )
      {
         pArtifact = pGirderArtifact->GetPositiveMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx );
      }
      else
      {
         pArtifact = pGirderArtifact->GetNegativeMomentFlexuralCapacityArtifact( intervalIdx, ls, artifactIdx );
      }

      const pgsPointOfInterest& poi(pArtifact->GetPointOfInterest());

      (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );


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
      {
         (*p_table)(row,col) << RPT_PASS;
      }
      else
      {
         (*p_table)(row,col) << RPT_FAIL;
      }

      (*p_table)(row,col++) << rptNewLine << _T("(") << cdRatio.SetValue(Mr,MrMin,bPassed) << _T(")");

      bPassed = pArtifact->CapacityPassed();
      if ( bPassed )
      {
         (*p_table)(row,col) << RPT_PASS;
      }
      else
      {
         (*p_table)(row,col) << RPT_FAIL;
      }

      (*p_table)(row,col++) << rptNewLine << _T("(") << cdRatio.SetValue(Mr,Mu,bPassed) << _T(")");

      if ( c_over_de )
      {
         if ( pArtifact->GetMaxReinforcementRatioLimit() < pArtifact->GetMaxReinforcementRatio() )
         {
            bOverReinforced = true;
            (*p_table)(row,8) << _T(" *");

            // Show limiting capacity of over reinforced section
         
            GET_IFACE2(pBroker,IMomentCapacity,pMomentCap); 
            // it may seem wasteful to get this interface in this scope, inside a loop
            // however, the c_over_de method isn't used in the current LRFD so the reality is
            // that this interface wont be requested very often

            const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment);
            (*p_table)(row,5) << rptNewLine << _T("(") << moment.SetValue( pmcd->Phi * pmcd->MnMin ) << _T(")");
         }
      }

      row++;
   } // next artifact

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
