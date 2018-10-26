///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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


#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#include <PgsExt\TimelineManager.h>


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

CUserDefinedLoadsChapterBuilder::CUserDefinedLoadsChapterBuilder(bool bSelect, bool SimplifiedVersion) :
CPGSuperChapterBuilder(bSelect),
m_bSimplifiedVersion(SimplifiedVersion)
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
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CGirderLineReportSpecification* pGdrLineRptSpec = dynamic_cast<CGirderLineReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pGdrLineRptSpec->GetBroker(&pBroker);
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);

      SpanIndexType firstSpanIdx,lastSpanIdx;
      pBridge->GetGirderGroupSpans(grpIdx,&firstSpanIdx,&lastSpanIdx);

      for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx <= lastSpanIdx; spanIdx++ )
      {
         for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
         {
            rptParagraph* pParagraph;

            CSpanKey spanKey(spanIdx,gdrIdx);

            // Only print span and girder if we are in a multi span or multi girder loop
            if (lastSpanIdx != firstSpanIdx || lastGirderIdx != firstGirderIdx)
            {
               pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
               *pChapter << pParagraph;
               *pParagraph << _T("Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << rptNewLine;
            }

            pParagraph = new rptParagraph;
            *pChapter << pParagraph;
            *pParagraph << _T("Locations are measured from left support.") << rptNewLine;

            // tables of details - point loads first
            rptParagraph* ppar1 = CreatePointLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << ppar1;

            // distributed loads
            ppar1 = CreateDistributedLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << ppar1;

            // moments loads
            ppar1 = CreateMomentLoadTable(pBroker, spanKey, pDisplayUnits, level, m_bSimplifiedVersion);
            *pChapter << ppar1;
         } // gdrIdx
      } // spanIdx
   } // groupIdx

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
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion)
{
   USES_CONVERSION;
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   shear,   pDisplayUnits->GetShearUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

   ASSERT_SPAN_KEY(spanKey);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);


   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   std::_tostringstream os;
   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T("Point Loads"));

   (*table)(0,0)  << _T("Event");
   (*table)(0,1)  << _T("Load") << rptNewLine << _T("Case");
   (*table)(0,2)  << COLHDR(_T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,3)  << COLHDR(_T("Magnitude"),rptForceUnitTag, pDisplayUnits->GetShearUnit() );
   (*table)(0,4)  << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      std::_tstring strEventName(pEventMap->GetEventName(eventIdx));

      const std::vector<IUserDefinedLoads::UserPointLoad>* ppl = pUdl->GetPointLoads(intervalIdx, spanKey);
      if ( ppl != NULL )
      {
         IndexType npl = ppl->size();

         for (IndexType ipl = 0; ipl < npl; ipl++)
         {
            loads_exist = true;

            const IUserDefinedLoads::UserPointLoad& upl = ppl->at(ipl);

             std::_tstring strLoadCaseName;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
             {
                strLoadCaseName = _T("DC");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
             {
                strLoadCaseName = _T("DW");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
             {
                strLoadCaseName = _T("LL+IM");
             }
             else
             {
                ATLASSERT(false);
             }

            (*table)(row,0) << strEventName;
            (*table)(row,1) << strLoadCaseName;
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

      if (!bSimplifiedVersion)
      {
         *pParagraph << _T("Point loads were not defined for this girder")<<rptNewLine;
      }
   }

   return pParagraph;
}

rptParagraph* CUserDefinedLoadsChapterBuilder::CreateDistributedLoadTable(IBroker* pBroker,
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue,   fplu,   pDisplayUnits->GetForcePerLengthUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

   ASSERT_SPAN_KEY(spanKey);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(7,_T("Distributed Loads"));

   (*table)(0,0)  << _T("Event");
   (*table)(0,1)  << _T("Load") << rptNewLine << _T("Case");
   (*table)(0,2)  << COLHDR(_T("Start") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,3)  << COLHDR(_T("End") << rptNewLine << _T("Location"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,4)  << COLHDR(_T("Start") << rptNewLine << _T("Magnitude"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,5)  << COLHDR(_T("End") << rptNewLine << _T("Magnitude"),rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit() );
   (*table)(0,6)  << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      std::_tstring strEventName(pEventMap->GetEventName(eventIdx));

      const std::vector<IUserDefinedLoads::UserDistributedLoad>* ppl = pUdl->GetDistributedLoads(intervalIdx, spanKey);
      if (ppl != NULL)
      {
         IndexType npl = ppl->size();
         for (IndexType ipl = 0; ipl < npl; ipl++)
         {
            loads_exist = true;

            const IUserDefinedLoads::UserDistributedLoad& upl = ppl->at(ipl);

             std::_tstring strLoadCaseName;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
             {
                strLoadCaseName = _T("DC");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
             {
                strLoadCaseName = _T("DW");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
             {
                strLoadCaseName = _T("LL+IM");
             }
             else
             {
                ATLASSERT(false);
             }

            (*table)(row,0) << strEventName;
            (*table)(row,1) << strLoadCaseName;
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

      if (! bSimplifiedVersion)
      {
         *pParagraph << _T("Distributed loads were not defined for this girder")<<rptNewLine;
      }
   }

   return pParagraph;
}


rptParagraph* CUserDefinedLoadsChapterBuilder::CreateMomentLoadTable(IBroker* pBroker,
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion)
{
   rptParagraph* pParagraph = new rptParagraph();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    dim,   pDisplayUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,   pDisplayUnits->GetMomentUnit(), false );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex,spanKey.girderIndex);

   ASSERT_SPAN_KEY(spanKey);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   GET_IFACE2(pBroker,IEventMap,pEventMap);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(5,_T("End Moments"));

   (*table)(0,0)  << _T("Event");
   (*table)(0,1)  << _T("Load") << rptNewLine << _T("Case");
   (*table)(0,2)  << _T("Location");
   (*table)(0,3)  << COLHDR(_T("Magnitude"),rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,4)  << _T("Description");

   GET_IFACE2(pBroker, IUserDefinedLoads, pUdl );

   bool loads_exist = false;
   RowIndexType row = table->GetNumberOfHeaderRows();
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      EventIndexType eventIdx = pIntervals->GetStartEvent(intervalIdx);
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      std::_tstring strEventName(pEventMap->GetEventName(eventIdx));

      const std::vector<IUserDefinedLoads::UserMomentLoad>* ppl = pUdl->GetMomentLoads(intervalIdx, spanKey);
      if (ppl != NULL)
      {
         IndexType npl = ppl->size();

         for (IndexType ipl = 0; ipl < npl; ipl++)
         {
            loads_exist = true;

            IUserDefinedLoads::UserMomentLoad upl = ppl->at(ipl);

             std::_tstring strLoadCaseName;
             if (upl.m_LoadCase==IUserDefinedLoads::userDC)
             {
                strLoadCaseName = _T("DC");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userDW)
             {
                strLoadCaseName = _T("DW");
             }
             else if (upl.m_LoadCase==IUserDefinedLoads::userLL_IM)
             {
                strLoadCaseName = _T("LL+IM");
             }
             else
             {
                ATLASSERT(false);
             }

            (*table)(row,0) << strEventName;
            (*table)(row,1) << strLoadCaseName;

            if ( IsZero(upl.m_Location) )
            {
               (*table)(row,2) << _T("Start of span");
            }
            else
            {
               (*table)(row,2) << _T("End of span");
            }

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

      if (!bSimplifiedVersion)
      {
         *pParagraph << _T("Moment loads were not defined for this girder")<<rptNewLine;
      }
   }

   return pParagraph;
}

