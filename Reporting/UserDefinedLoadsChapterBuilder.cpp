///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>


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
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : Min(girderKey.girderIndex,nGirders-1));
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
               pParagraph = new rptParagraph(rptStyleManager::GetHeadingStyle());
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

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location,   pDisplayUnits->GetSpanLengthUnit(),  true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,   pDisplayUnits->GetGeneralForceUnit(), false );
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   ASSERT_SPAN_KEY(spanKey);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);
   std::vector<CPointLoadData> vLoads = pUdl->GetPointLoads(spanKey);
   if (0 < vLoads.size())
   {
      GET_IFACE2(pBroker, IEventMap, pEventMap);

      rptRcTable* table = rptStyleManager::CreateDefaultTable(5, _T("Point Loads"));

      table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pParagraph << table;

      ColumnIndexType col = 0;
      (*table)(0, col++) << _T("Event");
      (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
      (*table)(0, col++) << _T("Location");
      (*table)(0, col++) << COLHDR(_T("Magnitude"), rptForceUnitTag, pDisplayUnits->GetShearUnit());
      (*table)(0, col++) << _T("Description");

      RowIndexType row = table->GetNumberOfHeaderRows();

      for ( const auto& load : vLoads)
      {
         col = 0;
         EventIndexType eventIdx = pUdl->GetPointLoadEventIndex(load.m_ID);

         std::_tstring strEventName(pEventMap->GetEventName(eventIdx));
         std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(load.m_LoadCase));

         (*table)(row, col++) << strEventName;
         (*table)(row, col++) << strLoadCaseName;

         if (load.m_Fractional)
         {
            (*table)(row, col++) << percentage.SetValue(load.m_Location);
         }
         else
         {
            (*table)(row, col++) << location.SetValue(load.m_Location);
         }

         (*table)(row, col++) << force.SetValue(load.m_Magnitude);
         (*table)(row, col++) << load.m_Description;

         row++;
      }
   }
   else
   {
      if (!bSimplifiedVersion)
      {
         *pParagraph << _T("Point loads were not defined for this girder") << rptNewLine;
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

   INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE(rptForcePerLengthUnitValue, force, pDisplayUnits->GetForcePerLengthUnit(), false);
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey.spanIndex, spanKey.girderIndex);

   ASSERT_SPAN_KEY(spanKey);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);
   std::vector<CDistributedLoadData> vLoads = pUdl->GetDistributedLoads(spanKey);
   if (0 < vLoads.size() )
   {
      GET_IFACE2(pBroker, IEventMap, pEventMap);

      rptRcTable* table = rptStyleManager::CreateDefaultTable(7, _T("Distributed Loads"));

      table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table->SetColumnStyle(6, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(6, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      *pParagraph << table;

      ColumnIndexType col = 0;
      (*table)(0, col++) << _T("Event");
      (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
      (*table)(0, col++) << _T("Start") << rptNewLine << _T("Location");
      (*table)(0, col++) << _T("End") << rptNewLine << _T("Location");
      (*table)(0, col++) << COLHDR(_T("Start") << rptNewLine << _T("Magnitude"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      (*table)(0, col++) << COLHDR(_T("End") << rptNewLine << _T("Magnitude"), rptForcePerLengthUnitTag, pDisplayUnits->GetForcePerLengthUnit());
      (*table)(0, col++) << _T("Description");

      RowIndexType row = table->GetNumberOfHeaderRows();

      for ( const auto& load : vLoads)
      {
         col = 0;

         EventIndexType eventIdx = pUdl->GetDistributedLoadEventIndex(load.m_ID);

         std::_tstring strEventName(pEventMap->GetEventName(eventIdx));
         std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(load.m_LoadCase));

         (*table)(row, col++) << strEventName;
         (*table)(row, col++) << strLoadCaseName;

         if (load.m_Fractional)
         {
            if (load.m_Type == UserLoads::Uniform)
            {
               (*table)(row, col++) << percentage.SetValue(0.0);
               (*table)(row, col++) << percentage.SetValue(1.0);
            }
            else
            {
               (*table)(row, col++) << percentage.SetValue(load.m_StartLocation);
               (*table)(row, col++) << percentage.SetValue(load.m_EndLocation);
            }
         }
         else
         {
            if (load.m_Type == UserLoads::Uniform)
            {
               (*table)(row, col++) << location.SetValue(0.0);
               (*table)(row, col++) << location.SetValue(span_length);
            }
            else
            {
               (*table)(row, col++) << location.SetValue(load.m_StartLocation);
               (*table)(row, col++) << location.SetValue(load.m_EndLocation);
            }
         }

         if (load.m_Type == UserLoads::Uniform)
         {
            (*table)(row, col++) << force.SetValue(load.m_WStart);
            (*table)(row, col++) << force.SetValue(load.m_WStart);
         }
         else
         {
            (*table)(row, col++) << force.SetValue(load.m_WStart);
            (*table)(row, col++) << force.SetValue(load.m_WEnd);
         }
         (*table)(row, col++) << load.m_Description;

         row++;
      }
   }
   else
   {
      if (!bSimplifiedVersion)
      {
         *pParagraph << _T("Distributed loads were not defined for this girder") << rptNewLine;
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

   INIT_UV_PROTOTYPE(rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), true);
   INIT_UV_PROTOTYPE( rptMomentUnitValue,   moment,   pDisplayUnits->GetMomentUnit(), false );
   INIT_SCALAR_PROTOTYPE(rptRcPercentage, percentage, pDisplayUnits->GetPercentageFormat());

   ASSERT_SPAN_KEY(spanKey);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanKey.spanIndex);
   CGirderKey girderKey(grpIdx,spanKey.girderIndex);
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(pBroker, IUserDefinedLoadData, pUdl);
   std::vector<CMomentLoadData> vLoads = pUdl->GetMomentLoads(spanKey);
   if (0 < vLoads.size())
   {
      GET_IFACE2(pBroker, IEventMap, pEventMap);

      rptRcTable* table = rptStyleManager::CreateDefaultTable(5, _T("End Moments"));

      *pParagraph << table;

      table->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      table->SetColumnStyle(4, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      table->SetStripeRowColumnStyle(4, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      ColumnIndexType col = 0;
      (*table)(0, col++) << _T("Event");
      (*table)(0, col++) << _T("Load") << rptNewLine << _T("Case");
      (*table)(0, col++) << _T("Location");
      (*table)(0, col++) << COLHDR(_T("Magnitude"), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
      (*table)(0, col++) << _T("Description");

      RowIndexType row = table->GetNumberOfHeaderRows();

      for ( const auto& load : vLoads)
      {
         col = 0;
         EventIndexType eventIdx = pUdl->GetMomentLoadEventIndex(load.m_ID);

         std::_tstring strEventName(pEventMap->GetEventName(eventIdx));
         std::_tstring strLoadCaseName(UserLoads::GetLoadCaseName(load.m_LoadCase));

         (*table)(row, col++) << strEventName;
         (*table)(row, col++) << strLoadCaseName;

         if (IsZero(load.m_Location))
         {
            (*table)(row, col++) << _T("Start of span");
         }
         else
         {
            (*table)(row, col++) << _T("End of span");
         }

         (*table)(row, col++) << moment.SetValue(load.m_Magnitude);
         (*table)(row, col++) << load.m_Description;

         row++;
      }
   }
   else
   {
      if (!bSimplifiedVersion)
      {
         *pParagraph << _T("Moment loads were not defined for this girder") << rptNewLine;
      }
   }

   return pParagraph;
}

