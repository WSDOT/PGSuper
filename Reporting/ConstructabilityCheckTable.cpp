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
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CConstructabilityCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CConstructabilityCheckTable::CConstructabilityCheckTable()
{
}

CConstructabilityCheckTable::CConstructabilityCheckTable(const CConstructabilityCheckTable& rOther)
{
   MakeCopy(rOther);
}

CConstructabilityCheckTable::~CConstructabilityCheckTable()
{
}

//======================== OPERATORS  =======================================
CConstructabilityCheckTable& CConstructabilityCheckTable::operator= (const CConstructabilityCheckTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CConstructabilityCheckTable::BuildSlabOffsetTable(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsConstructabilityArtifact* pArtifact = pGdrArtifact->GetConstructabilityArtifact();
   
   if (pArtifact->SlabOffsetStatus() != pgsConstructabilityArtifact::NA)
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4,"Slab Offset (\"A\" Dimension)");

      pTable->SetColumnStyle(3,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(3,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      (*pTable)(0,0) << COLHDR("Minimum" << rptNewLine << "Provided", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,1) << COLHDR("Required", rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,2) << "Status";
      (*pTable)(0,3) << "Notes";

      (*pTable)(1,0) << dim.SetValue(pArtifact->GetProvidedSlabOffset());
      (*pTable)(1,1) << dim.SetValue(pArtifact->GetRequiredSlabOffset());

      switch( pArtifact->SlabOffsetStatus() )
      {
         case pgsConstructabilityArtifact::Passed:
            (*pTable)(1,2) << RPT_PASS;
            break;

         case pgsConstructabilityArtifact::Failed:
            (*pTable)(1,2) << RPT_FAIL;
            break;

         case pgsConstructabilityArtifact::Excessive:
            (*pTable)(1,2) << color(Blue) << "Excessive" << color(Black);
            break;

         default:
            ATLASSERT(0);
            break;
      }

      if ( pArtifact->CheckStirrupLength() )
      {
         (*pTable)(1,3) << color(Red) << "There is a large variation in the slab haunch thickness. Check stirrup length to ensure they engage the deck at all locations." << color(Black) << rptNewLine;
      }
      else
      {
         (*pTable)(1,3) << "";
      }

      return pTable;
   }
   else
   {
      return NULL;
   }
}

void CConstructabilityCheckTable::BuildGlobalGirderStabilityCheck(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsConstructabilityArtifact* pArtifact = pGdrArtifact->GetConstructabilityArtifact();
   
   if ( !pArtifact->IsGlobalGirderStabilityApplicable() )
   {
      return;
   }

   rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << "Global Stability of Girder";

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(pgsReportStyleHolder::GetImagePath() + "GlobalGirderStability.gif");

   rptRcScalar slope;
   slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,"");
   std::string strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   (*pTable)(0,0) << COLHDR(Sub2("W","b"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,1) << COLHDR(Sub2("Y","b"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,2) << "Incline from Vertical (" << Sub2(symbol(theta),"max") << ")" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,3) << "Max Incline" << rptNewLine << "(" << strSlopeTag << "/" << strSlopeTag << ")";
   (*pTable)(0,4) << "Status";

   double Wb, Yb, Orientation;
   pArtifact->GetGlobalGirderStabilityParameters(&Wb,&Yb,&Orientation);
   double maxIncline = pArtifact->GetMaxGirderIncline();

   (*pTable)(1,0) << dim.SetValue(Wb);
   (*pTable)(1,1) << dim.SetValue(Yb);
   (*pTable)(1,2) << slope.SetValue(Orientation);
   (*pTable)(1,3) << slope.SetValue(maxIncline);

   if ( pArtifact->GlobalGirderStabilityPassed() )
      (*pTable)(1,4) << RPT_PASS;
   else
      (*pTable)(1,4) << RPT_FAIL << rptNewLine << "Reaction falls outside of middle third of bottom width of girder";
   
   *pBody << pTable;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CConstructabilityCheckTable::MakeCopy(const CConstructabilityCheckTable& rOther)
{
   // Add copy code here...
}

void CConstructabilityCheckTable::MakeAssignment(const CConstructabilityCheckTable& rOther)
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
bool CConstructabilityCheckTable::AssertValid() const
{
   return true;
}

void CConstructabilityCheckTable::Dump(dbgDumpContext& os) const
{
   os << "Dump for CConstructabilityCheckTable" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CConstructabilityCheckTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CConstructabilityCheckTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CConstructabilityCheckTable");

   TESTME_EPILOG("CConstructabilityCheckTable");
}
#endif // _UNITTEST
