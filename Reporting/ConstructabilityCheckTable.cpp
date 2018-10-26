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
#include <Reporting\ConstructabilityCheckTable.h>

#include <IFace\Artifact.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Constructability.h>

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
rptRcTable* CConstructabilityCheckTable::BuildSlabOffsetTable(IBroker* pBroker,const std::vector<SpanGirderHashType>& girderList,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   // Create table - delete it later if we don't need it
   bool IsSingleGirder = girderList.size()==1;

   ColumnIndexType ncols = IsSingleGirder ? 4 : 6; // put span/girder in table if multi girder
   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(ncols,_T("Slab Offset (\"A\" Dimension)"));

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim2, pDisplayUnits->GetComponentDimUnit(), true );

   pTable->SetColumnStyle(ncols-1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(ncols-1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   if (!IsSingleGirder)
   {
      (*pTable)(0,col++) << _T("Span");
      (*pTable)(0,col++) << _T("Girder");
   }

   (*pTable)(0,col++) << COLHDR(_T("Minimum") << rptNewLine << _T("Provided"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << COLHDR(_T("Required"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,col++) << _T("Status");
   (*pTable)(0,col++) << _T("Notes");

   // First thing - check if we can generate the girder schedule table at all.
   bool areAnyRows(false);
   std::vector<SpanGirderHashType>::const_iterator itsg_end(girderList.end());
   std::vector<SpanGirderHashType>::const_iterator itsg(girderList.begin());
   RowIndexType row=0;
   while(itsg != itsg_end)
   {
      SpanIndexType span;
      GirderIndexType girder;
      UnhashSpanGirder(*itsg,&span,&girder);

      const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
      const pgsConstructabilityArtifact* pArtifact = pGdrArtifact->GetConstructabilityArtifact();
      
      if (pArtifact->SlabOffsetStatus() != pgsConstructabilityArtifact::NA)
      {
         row++;
         col = 0;

         if (!IsSingleGirder)
         {
            (*pTable)(row, col++) << LABEL_SPAN(span);
            (*pTable)(row, col++) << LABEL_GIRDER(girder);
         }

         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetProvidedSlabOffset());
         (*pTable)(row, col++) << dim.SetValue(pArtifact->GetRequiredSlabOffset());

         switch( pArtifact->SlabOffsetStatus() )
         {
            case pgsConstructabilityArtifact::Passed:
               (*pTable)(row, col++) << RPT_PASS;
               break;

            case pgsConstructabilityArtifact::Failed:
               (*pTable)(row, col++) << RPT_FAIL;
               break;

            case pgsConstructabilityArtifact::Excessive:
               (*pTable)(row, col++) << color(Blue) << _T("Excessive") << color(Black);
               break;

            default:
               ATLASSERT(0);
               break;
         }

         if ( pArtifact->CheckStirrupLength() )
         {
            GET_IFACE2(pBroker,IGirderHaunch,pGdrHaunch);
            HAUNCHDETAILS haunch_details;
            pGdrHaunch->GetHaunchDetails(span,girder,&haunch_details);

            (*pTable)(row, col++) << color(Red) << _T("The haunch depth in the middle of the girder exceeds the depth at the ends by ") << dim2.SetValue(haunch_details.HaunchDiff) << _T(". Check stirrup lengths to ensure they engage the deck in all locations.") << color(Black) << rptNewLine;
         }
         else
         {
            (*pTable)(row, col++) << _T("");
         }
      }

      itsg++;
   }

   // Only return a table if it has content
   if (row>0)
   {
      return pTable;
   }
   else
   {
      delete pTable;
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
   *pTitle << _T("Global Stability of Girder");

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("GlobalGirderStability.gif"));

   rptRcScalar slope;
   slope.SetFormat(pDisplayUnits->GetScalarFormat().Format);
   slope.SetWidth(pDisplayUnits->GetScalarFormat().Width);
   slope.SetPrecision(pDisplayUnits->GetScalarFormat().Precision);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(), false );

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(5,_T(""));
   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   (*pTable)(0,0) << COLHDR(Sub2(_T("W"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,1) << COLHDR(Sub2(_T("Y"),_T("b")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*pTable)(0,2) << _T("Incline from Vertical (") << Sub2(symbol(theta),_T("max")) << _T(")") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,3) << _T("Max Incline") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");
   (*pTable)(0,4) << _T("Status");

   Float64 Wb, Yb, Orientation;
   pArtifact->GetGlobalGirderStabilityParameters(&Wb,&Yb,&Orientation);
   Float64 maxIncline = pArtifact->GetMaxGirderIncline();

   (*pTable)(1,0) << dim.SetValue(Wb);
   (*pTable)(1,1) << dim.SetValue(Yb);
   (*pTable)(1,2) << slope.SetValue(Orientation);
   (*pTable)(1,3) << slope.SetValue(maxIncline);

   if ( pArtifact->GlobalGirderStabilityPassed() )
      (*pTable)(1,4) << RPT_PASS;
   else
      (*pTable)(1,4) << RPT_FAIL << rptNewLine << _T("Reaction falls outside of middle third of bottom width of girder");
   
   *pBody << pTable;
}

void CConstructabilityCheckTable::BuildLongitudinalRebarGeometryCheck(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsConstructabilityArtifact* pArtifact = pGdrArtifact->GetConstructabilityArtifact();
   
   if ( !pArtifact->RebarGeometryCheckPassed() )
   {
      rptParagraph* pTitle = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
      *pChapter << pTitle;
      *pTitle << _T("Longitudinal rebars exist outside of the girder section - ") << RPT_FAIL;

      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      std::vector<RowIndexType> rows = pArtifact->GetRebarRowsOutsideOfSection();

      *pBody << _T("Bars are located outside of the section in the following rows: ");

       CollectionIndexType nr = rows.size();
      for (CollectionIndexType ir=0; ir<nr; ir++)
      {
         CollectionIndexType row = rows.at(ir);
         *pBody << row+1;
         if (ir!=nr-1)
            *pBody << _T(", ");
      }
   }
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
   os << _T("Dump for CConstructabilityCheckTable") << endl;
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
