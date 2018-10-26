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
#include <Reporting\StrandLocations.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <PgsExt\GirderData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandLocations
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandLocations::CStrandLocations()
{
}

CStrandLocations::CStrandLocations(const CStrandLocations& rOther)
{
   MakeCopy(rOther);
}

CStrandLocations::~CStrandLocations()
{
}

//======================== OPERATORS  =======================================
CStrandLocations& CStrandLocations::operator= (const CStrandLocations& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

//======================== OPERATIONS =======================================
void CStrandLocations::Build(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();
   CHECK(span<nspans);

   Float64 Lg = pBridge->GetGirderLength(span,girder);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len, pDisplayUnits->GetSpanLengthUnit(),  false );

   rptParagraph* pHead;
   pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pHead;
   *pHead <<_T("Prestressing Strand Locations")<<rptNewLine;

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,girder);
   if (pgirderData->PrestressData.GetNumPermStrandsType() == NPS_DIRECT_SELECTION)
   {
      rptParagraph* p = new rptParagraph;
      *pChapter << p;
      *p << _T("Note: Strands were defined using Non-Sequential, Direct Fill.") << rptNewLine;
   }

   // Straight strands
   pgsPointOfInterest end_poi(span,girder,0.0);
   StrandIndexType nss = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Straight);
   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Straight);
   StrandIndexType nExtendedLeft  = pStrandGeometry->GetNumExtendedStrands(span,girder,pgsTypes::metStart,pgsTypes::Straight);
   StrandIndexType nExtendedRight = pStrandGeometry->GetNumExtendedStrands(span,girder,pgsTypes::metEnd,pgsTypes::Straight);
   if (0 < nss)
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      ColumnIndexType nColumns = 3;
      if ( 0 < nDebonded )
         nColumns += 2;

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
         nColumns += 2;

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Straight Strand Locations"));
      *pPara << p_table;

      ColumnIndexType col = 0;
      (*p_table)(0,col++) << _T("Strand");
      (*p_table)(0,col++) << COLHDR(_T("X Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(0,col++) << COLHDR(_T("Y Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if ( 0 < nDebonded )
      {
         (*p_table)(0,col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
      {
         p_table->SetColumnStyle(col,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0,col++) << _T("Extended") << rptNewLine << _T("Left End");

         p_table->SetColumnStyle(col,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0,col++) << _T("Extended") << rptNewLine << _T("Right End");
      }

      CComPtr<IPoint2dCollection> spts;
      pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Straight, &spts);
      RowIndexType row = 1;
      for (StrandIndexType is = 0; is < nss; is++)
      {
         col = 0;
         (*p_table)(row,col++) << row;
         CComPtr<IPoint2d> spt;
         spts->get_Item(is, &spt);
         Float64 x,y;
         spt->get_X(&x);
         spt->get_Y(&y);
         (*p_table)(row,col++) << dim.SetValue(x);
         (*p_table)(row,col++) << dim.SetValue(y);

         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(span,girder,is,pgsTypes::Straight,&start,&end) )
            {
               (*p_table)(row,col++) << len.SetValue(start);
               (*p_table)(row,col++) << len.SetValue(Lg - end);
            }
            else
            {
               (*p_table)(row,col++) << _T("-");
               (*p_table)(row,col++) << _T("-");
            }
         }

         if ( 0 < nExtendedLeft || 0 < nExtendedRight )
         {
            if ( pStrandGeometry->IsExtendedStrand(span,girder,pgsTypes::metStart,is,pgsTypes::Straight) )
               (*p_table)(row,col++) << symbol(DOT);
            else
               (*p_table)(row,col++) << _T("");

            if ( pStrandGeometry->IsExtendedStrand(span,girder,pgsTypes::metEnd,is,pgsTypes::Straight) )
               (*p_table)(row,col++) << symbol(DOT);
            else
               (*p_table)(row,col++) << _T("");
         }

         row++;
      }
   }
   else
   {
      rptParagraph* pPara;
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara <<_T("Straight Strand Locations")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<_T("No Straight Strands in Girder")<<rptNewLine;
   }

   // Temporary strands
   if ( 0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary) )
   {
      StrandIndexType nts = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Temporary);
      nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Temporary);
      if (nts >0)
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3 + (nDebonded > 0 ? 2 : 0),_T("Temporary Strand Locations"));
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         if ( 0 < nDebonded )
         {
            (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }

         CComPtr<IPoint2dCollection> spts;
         pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Temporary, &spts);

         RowIndexType row=1;
         for (StrandIndexType is = 0; is < nts; is++)
         {
            (*p_table)(row,0) << row;
            CComPtr<IPoint2d> spt;
            spts->get_Item(is, &spt);
            Float64 x,y;
            spt->get_X(&x);
            spt->get_Y(&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);

            if ( 0 < nDebonded ) 
            {
               Float64 start,end;
               if ( pStrandGeometry->IsStrandDebonded(span,girder,is,pgsTypes::Temporary,&start,&end) )
               {
                  (*p_table)(row,3) << len.SetValue(start);
                  (*p_table)(row,4) << len.SetValue(end);
               }
               else
               {
                  (*p_table)(row,3) << _T("-");
                  (*p_table)(row,4) << _T("-");
               }
            }

            row++;
         }
      }
      else
      {
         rptParagraph* pPara;
         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara <<_T("Temporary Strand Locations")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara <<_T("No Temporary Strands in Girder")<<rptNewLine;
      }
   }

   // Harped strands
   StrandIndexType nhs = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Harped);
   nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Harped);
   if (0 < nhs)
   {
      bool areHarpedStraight = pStrandGeometry->GetAreHarpedStrandsForcedStraight(span,girder);

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      std::_tstring label( (areHarpedStraight ? _T("Straight-Web Strand Locations") : _T("Harped Strand Locations at Ends of Girder")));

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3 + (nDebonded > 0 ? 2 : 0),label);
      *pPara << p_table;

      (*p_table)(0,0) << _T("Strand");
      (*p_table)(0,1) << COLHDR(_T("X Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(0,2) << COLHDR(_T("Y Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if ( 0 < nDebonded )
      {
         (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      CComPtr<IPoint2dCollection> spts;
      pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Harped, &spts);

      int row=1;
      StrandIndexType is;
      for (is = 0; is < nhs; is++)
      {
         (*p_table)(row,0) << (Uint16)row;
         CComPtr<IPoint2d> spt;
         spts->get_Item(is, &spt);
         Float64 x,y;
         spt->get_X(&x);
         spt->get_Y(&y);
         (*p_table)(row,1) << dim.SetValue(x);
         (*p_table)(row,2) << dim.SetValue(y);

         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(span,girder,is,pgsTypes::Harped,&start,&end) )
            {
               (*p_table)(row,3) << len.SetValue(start);
               (*p_table)(row,4) << len.SetValue(end);
            }
            else
            {
               (*p_table)(row,3) << _T("-");
               (*p_table)(row,4) << _T("-");
            }
         }

         row++;
      }

      if (!areHarpedStraight)
      {
         // harped strands at harping point
         Float64 mid = pBridge->GetGirderLength(span,girder)/2.;
         pgsPointOfInterest harp_poi(span,girder,mid);

         pPara = new rptParagraph;
         *pChapter << pPara;

         p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Harped Strand Locations at Harping Points"));
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         CComPtr<IPoint2dCollection> hspts;
         pStrandGeometry->GetStrandPositions(harp_poi, pgsTypes::Harped, &hspts);

         row=1;
         for (is=0; is<nhs; is++)
         {
            (*p_table)(row,0) << (Uint16)row;
            CComPtr<IPoint2d> spt;
            hspts->get_Item(is, &spt);
            Float64 x,y;
            spt->get_X(&x);
            spt->get_Y(&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);
            row++;
         }
      }
   }
   else
   {
      rptParagraph* pPara;
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara <<_T("Harped Strand Locations")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara <<_T("No Harped Strands in Girder")<<rptNewLine;
   }

   // Temporary strands
   if ( 0 < pStrandGeometry->GetMaxStrands(span,girder,pgsTypes::Temporary) )
   {
      StrandIndexType nts = pStrandGeometry->GetNumStrands(span,girder,pgsTypes::Temporary);
      nDebonded = pStrandGeometry->GetNumDebondedStrands(span,girder,pgsTypes::Temporary);
      if (nts >0)
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3 + (nDebonded > 0 ? 2 : 0),_T("Temporary Strand Locations"));
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y Location"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         if ( 0 < nDebonded )
         {
            (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }

         CComPtr<IPoint2dCollection> spts;
         pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Temporary, &spts);

         RowIndexType row=1;
         for (StrandIndexType is = 0; is < nts; is++)
         {
            (*p_table)(row,0) << row;
            CComPtr<IPoint2d> spt;
            spts->get_Item(is, &spt);
            Float64 x,y;
            spt->get_X(&x);
            spt->get_Y(&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);

            if ( 0 < nDebonded ) 
            {
               Float64 start,end;
               if ( pStrandGeometry->IsStrandDebonded(span,girder,is,pgsTypes::Temporary,&start,&end) )
               {
                  (*p_table)(row,3) << len.SetValue(start);
                  (*p_table)(row,4) << len.SetValue(end);
               }
               else
               {
                  (*p_table)(row,3) << _T("-");
                  (*p_table)(row,4) << _T("-");
               }
            }

            row++;
         }
      }
      else
      {
         rptParagraph* pPara;
         pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << pPara;
         *pPara <<_T("Temporary Strand Locations")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara <<_T("No Temporary Strands in Girder")<<rptNewLine;
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CStrandLocations::MakeCopy(const CStrandLocations& rOther)
{
   // Add copy code here...
}

void CStrandLocations::MakeAssignment(const CStrandLocations& rOther)
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
bool CStrandLocations::AssertValid() const
{
   return true;
}

void CStrandLocations::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CStrandLocations") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStrandLocations::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStrandLocations");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStrandLocations");

   TESTME_EPILOG("CStrandLocations");
}
#endif // _UNITTEST
