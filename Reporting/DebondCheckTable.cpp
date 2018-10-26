///////////////////////////////////////////////////////////////////////
// PGSplice - Precast Post-tensioned Spliced Girder Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <Reporting\DebondCheckTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PointOfInterest.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\Allowables.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDebondCheckTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDebondCheckTable::CDebondCheckTable()
{
}


CDebondCheckTable::~CDebondCheckTable()
{
}

//======================== OPERATORS  =======================================

//======================== OPERATIONS =======================================
void CDebondCheckTable::Build(rptChapter* pChapter, IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IDebondLimits,debond_limits);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsDebondArtifact* pDebondArtifact = gdrArtifact->GetDebondArtifact(strandType);

   rptParagraph* p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p <<_T("Debonding Limits")<<rptNewLine;

   p = new rptParagraph;
   *pChapter << p;

   Float64 total_fra = pDebondArtifact->GetFraDebondedStrands()*100.0;
   Float64 limit_fra = pDebondArtifact->GetMaxFraDebondedStrands()*100;

   StrandIndexType ndb = pDebondArtifact->GetNumDebondedStrands();

   if (total_fra>limit_fra)
   {
      *p <<Bold(_T("Warning: "));
   }

   *p << ndb <<_T(", or ")<< total_fra << _T("% of total strands are debonded. Debonded strands should not exceed ") << limit_fra << _T("% of the total.") << rptNewLine;

   // check debond lengths 
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    loc, pDisplayUnits->GetSpanLengthUnit(),   true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    loc2, pDisplayUnits->GetSpanLengthUnit(),   true );

   Float64 dbl_limit;
   pgsTypes::DebondLengthControl control;
   pDebondArtifact->GetDebondLengthLimit(&dbl_limit, &control);
   Float64 maxdbl = pDebondArtifact->GetMaxDebondLength();

   *p << _T("The longest debond length = ")<< loc.SetValue(maxdbl) <<_T(", and the allowable length is ")<<loc2.SetValue(dbl_limit)<<_T(" controlled by ");
   if (control==pgsTypes::mdbDefault)
   {
      *p<<_T("development length from mid-girder  ");
   }
   else if (control==pgsTypes::mbdFractional)
   {
      *p<<_T("user input fraction of girder length  ");
   }
   else
   {
      *p<<_T("user-input length  ");
   }

   if (maxdbl - 1.0e-5 >dbl_limit)
      *p<<RPT_FAIL<<rptNewLine;
   else
      *p<<RPT_PASS<<rptNewLine;

   // debond section length
   Float64 mndbs = pDebondArtifact->GetMinDebondSectionSpacing();
   Float64 mndbsl = pDebondArtifact->GetDebondSectionSpacingLimit();
   *p << _T("The shortest distance between debond sections  = ")<< loc.SetValue(mndbs) <<_T(", and the minimum allowable = ")<<loc2.SetValue(mndbsl);
   // need a tolerance here
   if (mndbs + 1.0e-5 < mndbsl)
      *p<<_T("  ")<<RPT_FAIL<<rptNewLine;
   else
      *p<<_T("  ")<<RPT_PASS<<rptNewLine;

   // tables
   Float64 Lg = pBridge->GetGirderLength(span,girder);

   *p << CDebondCheckTable().Build1(pDebondArtifact,span,girder,pgsTypes::Straight, pDisplayUnits);
   *p << Super(_T("*")) << _T("Exterior strands shall not be debonded") << rptNewLine << rptNewLine;

   *p << CDebondCheckTable().Build2(pDebondArtifact,span,girder,Lg, pgsTypes::Straight, pDisplayUnits);
   *p << Super(_T("*")) << _T("Not more than ") << debond_limits->GetMaxDebondedStrandsPerSection(span,girder)*100 << _T("% of the debonded strands, or ") << debond_limits->GetMaxNumDebondedStrandsPerSection(span,girder) << _T(" strands, whichever is greatest, shall have debonding terminated at any section") << rptNewLine;
}

rptRcTable* CDebondCheckTable::Build1(const pgsDebondArtifact* pDebondArtifact,SpanIndexType span,GirderIndexType girder,pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,_T(" "));
   table->TableCaption().SetStyleName(pgsReportStyleHolder::GetHeadingStyle());
   table->TableCaption() << _T("Requirements for Partially Debonded Strands [5.11.4.3]");

   (*table)(0,0) << _T("Row");
   (*table)(0,1) << _T("Number") << rptNewLine << _T("Strands");
   (*table)(0,2) << _T("Number") << rptNewLine << _T("Debonded") << rptNewLine << _T("Strands");
   (*table)(0,3) << _T("% Debonded");
   (*table)(0,4) << _T("Maximum") << rptNewLine << _T("% Debonded");
   (*table)(0,5) << _T("Exterior") << rptNewLine << _T("Strands") << rptNewLine << _T("Debonded") << Super(_T("*"));
   (*table)(0,6) << _T("Status");

   // Fill up the table
   table->TableLabel().SetStyleName(pgsReportStyleHolder::GetFootnoteStyle());
   RowIndexType table_row = table->GetNumberOfHeaderRows();


   std::vector<StrandIndexType> nStrandsInRow = pDebondArtifact->GetNumStrandsInRow();
   std::vector<StrandIndexType> nDebondedStrandsInRow = pDebondArtifact->GetNumDebondedStrandsInRow();
   std::vector<Float64> vFra = pDebondArtifact->GetFraDebondedStrandsInRow();
   std::vector<Float64> vMaxFra = pDebondArtifact->GetMaxFraDebondedStrandsInRow();
   std::vector<bool>    bExteriorDebonded = pDebondArtifact->GetIsExteriorStrandDebondedInRow();

   std::vector<Float64>::iterator iter;
   Uint16 row = 1;
   Uint16 index = 0;
   for ( iter = vFra.begin(); iter != vFra.end(); iter++, index++ )
   {
      (*table)(table_row,0) << row;
      (*table)(table_row,1) << nStrandsInRow[index];
      (*table)(table_row,2) << nDebondedStrandsInRow[index];
      (*table)(table_row,3) << vFra[index]*100. << _T("%");
      (*table)(table_row,4) << vMaxFra[index]*100. << _T("%");
      (*table)(table_row,5) << (bExteriorDebonded[index] == true ? _T("Yes") : _T("No"));

      if ( pDebondArtifact->RowPassed(index) )
         (*table)(table_row,6) << RPT_PASS;
      else
         (*table)(table_row,6) << RPT_FAIL;

      row++;
      table_row++;
   }

   table_row++;

   return table;
}

rptRcTable* CDebondCheckTable::Build2(const pgsDebondArtifact* pDebondArtifact,SpanIndexType span,GirderIndexType girder,Float64 Lg, pgsTypes::StrandType strandType,IEAFDisplayUnits* pDisplayUnits) const
{
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(4,_T(" "));


   if ( span == ALL_SPANS )
   {
      table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   (*table)(0,0) << COLHDR(RPT_GDR_END_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << _T("Number") << rptNewLine << _T("Debonded") << rptNewLine << _T("Strands");
   (*table)(0,2) << _T("Maximum") << rptNewLine << _T("Debonded") << rptNewLine << _T("Strands") << Super(_T("*"));
   (*table)(0,3) << _T("Status");

   // Fill up the table
   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),   false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);

   StrandIndexType nMaxStrands1;
   Float64 fraMaxStrands;
   pDebondArtifact->GetMaxDebondStrandsAtSection(&nMaxStrands1,&fraMaxStrands);

   StrandIndexType nDebondedStrands = pDebondArtifact->GetNumDebondedStrands();

   // allow int to floor
   StrandIndexType nMaxStrands2 = Uint16(floor(fraMaxStrands * nDebondedStrands));
   StrandIndexType nMaxStrands = _cpp_max(nMaxStrands1,nMaxStrands2);

   SectionIndexType nSections = pDebondArtifact->GetNumDebondSections();
   for ( SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
   {
      Float64 loc;
      StrandIndexType nStrands;
      Float64 fraStrands;

      pDebondArtifact->GetDebondSection(sectionIdx,&loc,&nStrands,&fraStrands);

      if ( loc < 0 || Lg < loc )
         continue; // skip of debond point is off of girder

      pgsPointOfInterest poi(span,girder,loc);

      (*table)(sectionIdx+1,0) << location.SetValue(pgsTypes::BridgeSite3,poi);
      (*table)(sectionIdx+1,1) << nStrands;

      (*table)(sectionIdx+1,2) << nMaxStrands;

      if ( pDebondArtifact->SectionPassed(sectionIdx) )
         (*table)(sectionIdx+1,3) << RPT_PASS;
      else
         (*table)(sectionIdx+1,3) << RPT_FAIL;
  }
   
   return table;
}