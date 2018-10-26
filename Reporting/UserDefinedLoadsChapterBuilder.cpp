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
#include <Reporting\UserDefinedLoadsChapterBuilder.h>

#include <IFace\DisplayUnits.h>
#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   CUserDefinedLoadsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserDefinedLoadsChapterBuilder::CUserDefinedLoadsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CUserDefinedLoadsChapterBuilder::GetName() const
{
   return TEXT("User Defined Loads");
}

rptChapter* CUserDefinedLoadsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSpec->GetBroker(&pBroker);
   SpanIndexType span = pSpec->GetSpan();
   GirderIndexType girder = pSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pParagraph;
   pParagraph = new rptParagraph();
   *pChapter << pParagraph;
   *pParagraph <<"Span "<<LABEL_SPAN(span)<<" Girder "<<LABEL_GIRDER(girder)<<rptNewLine;
   *pParagraph <<"Locations are measured from left support."<<rptNewLine;

   // tables of details - point loads first
   rptParagraph* ppar1 = CreatePointLoadTable(pBroker, span, girder, pDisplayUnits, level);
   *pChapter <<ppar1;

   // distributed loads
   ppar1 = CreateDistributedLoadTable(pBroker, span, girder, pDisplayUnits, level);
   *pChapter <<ppar1;

   // moments loads
   ppar1 = CreateMomentLoadTable(pBroker, span, girder, pDisplayUnits, level);
   *pChapter <<ppar1;

   return pChapter;
}


CChapterBuilder* CUserDefinedLoadsChapterBuilder::Clone() const
{
   return new CUserDefinedLoadsChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

rptParagraph* CUserDefinedLoadsChapterBuilder::CreatePointLoadTable(IBroker* pBroker,
                           SpanIndexType span, GirderIndexType girder,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   shear,   pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(span,girder);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,"Point Loads");

   (*table)(0,0)  << "Stage";
   (*table)(0,1)  << "Load" << rptNewLine << "Case";
   (*table)(0,2)  << COLHDR("Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,3)  << COLHDR("Magnitude",rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4)  << "Description";

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   for ( int i=0; i<3; i++ )
   {
      std::string stagenm;
      pgsTypes::Stage stage;
      if (i==0)
      {
         stage = pgsTypes::BridgeSite1;
         stagenm = "Bridge Site 1";
      }
      else if (i==1)
      {
         stage = pgsTypes::BridgeSite2;
         stagenm = "Bridge Site 2";
      }
      else
      {
         stage = pgsTypes::BridgeSite3;
         stagenm = "Bridge Site 3";
      }

      const std::vector<IUserDefinedLoads::UserPointLoad>* ppl = pUdl->GetPointLoads(stage, span, girder);
      if (ppl!=0)
      {
         Int32 npl = ppl->size();

         for (Int32 ipl=0; ipl<npl; ipl++)
         {
            loads_exist = true;

            IUserDefinedLoads::UserPointLoad upl = ppl->at(ipl);

             std::string strlcn;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
                strlcn = "DC";
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
                strlcn = "DW";
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
                strlcn = "LL+IM";
             else
                ATLASSERT(0);

            (*table)(row,0) << stagenm;
            (*table)(row,1) << strlcn;
            (*table)(row,2) << dim.SetValue( upl.m_Location );
            (*table)(row,3) << shear.SetValue( upl.m_Magnitude );
            (*table)(row,4) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      *pParagraph << "No point loads exist for this girder"<<rptNewLine;
   }

   return pParagraph;
}

rptParagraph* CUserDefinedLoadsChapterBuilder::CreateDistributedLoadTable(IBroker* pBroker,
                           SpanIndexType span, GirderIndexType girder,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue,   fplu,   pDisplayUnits->GetForcePerLengthUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(span,girder);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,"Distributed Loads");

   (*table)(0,0)  << "Stage";
   (*table)(0,1)  << "Load" << rptNewLine << "Case";
   (*table)(0,2)  << COLHDR("Start" << rptNewLine << "Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,3)  << COLHDR("End" << rptNewLine << "Location",rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,4)  << COLHDR("Start" << rptNewLine << "Magnitude",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,5)  << COLHDR("End" << rptNewLine << "Magnitude",rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,6)  << "Description";

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   for ( int i=0; i<3; i++ )
   {
      std::string stagenm;
      pgsTypes::Stage stage;
      if (i==0)
      {
         stage = pgsTypes::BridgeSite1;
         stagenm = "Bridge Site 1";
      }
      else if (i==1)
      {
         stage = pgsTypes::BridgeSite2;
         stagenm = "Bridge Site 2";
      }
      else
      {
         stage = pgsTypes::BridgeSite3;
         stagenm = "Bridge Site 3";
      }

      const std::vector<IUserDefinedLoads::UserDistributedLoad>* ppl = pUdl->GetDistributedLoads(stage, span, girder);
      if (ppl!=0)
      {
         Int32 npl = ppl->size();
         for (Int32 ipl=0; ipl<npl; ipl++)
         {
            loads_exist = true;

            IUserDefinedLoads::UserDistributedLoad upl = ppl->at(ipl);

             std::string strlcn;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
                strlcn = "DC";
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
                strlcn = "DW";
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
                strlcn = "LL+IM";
             else
                ATLASSERT(0);

            (*table)(row,0) << stagenm;
            (*table)(row,1) << strlcn;
            (*table)(row,2) << dim.SetValue( upl.m_StartLocation );
            (*table)(row,3) << dim.SetValue( upl.m_EndLocation );
            (*table)(row,4) << fplu.SetValue( upl.m_WStart );
            (*table)(row,5) << fplu.SetValue( upl.m_WEnd );
            (*table)(row,6) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      *pParagraph << "No distributed loads exist for this girder"<<rptNewLine;
   }

   return pParagraph;
}


rptParagraph* CUserDefinedLoadsChapterBuilder::CreateMomentLoadTable(IBroker* pBroker,
                           SpanIndexType span, GirderIndexType girder,
                           IDisplayUnits* pDisplayUnits,
                           Uint16 level)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,   pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(span,girder);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,"Moment Loads");

   (*table)(0,0)  << "Stage";
   (*table)(0,1)  << "Load" << rptNewLine << "Case";
   (*table)(0,2)  << "Location";
   (*table)(0,3)  << COLHDR("Magnitude",rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,4)  << "Description";

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   for ( int i=0; i<3; i++ )
   {
      std::string stagenm;
      pgsTypes::Stage stage;
      if (i==0)
      {
         stage = pgsTypes::BridgeSite1;
         stagenm = "Bridge Site 1";
      }
      else if (i==1)
      {
         stage = pgsTypes::BridgeSite2;
         stagenm = "Bridge Site 2";
      }
      else
      {
         stage = pgsTypes::BridgeSite3;
         stagenm = "Bridge Site 3";
      }

      const std::vector<IUserDefinedLoads::UserMomentLoad>* ppl = pUdl->GetMomentLoads(stage, span, girder);
      if (ppl!=0)
      {
         Int32 npl = ppl->size();

         for (Int32 ipl=0; ipl<npl; ipl++)
         {
            loads_exist = true;

            IUserDefinedLoads::UserMomentLoad upl = ppl->at(ipl);

             std::string strlcn;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
                strlcn = "DC";
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
                strlcn = "DW";
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
                strlcn = "LL+IM";
             else
                ATLASSERT(0);

            (*table)(row,0) << stagenm;
            (*table)(row,1) << strlcn;

            if ( IsZero(upl.m_Location) )
               (*table)(row,2) << "Start of span";
            else
               (*table)(row,2) << "End of span";

            (*table)(row,3) << moment.SetValue( upl.m_Magnitude );
            (*table)(row,4) << upl.m_Description;

            row++;
         }
      }
   }

   if (loads_exist)
   {
      *pParagraph << table;
   }
   else
   {
      delete table;

      *pParagraph << "No moment loads exist for this girder"<<rptNewLine;
   }

   return pParagraph;
}

