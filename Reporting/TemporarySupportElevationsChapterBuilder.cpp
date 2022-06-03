///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "Reporting\TemporarySupportElevationsChapterBuilder.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <PgsExt\TemporarySupportData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTemporarySupportElevationsChapterBuilder::CTemporarySupportElevationsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTemporarySupportElevationsChapterBuilder::GetName() const
{
   return _T("Temporary Support Elevations");
}

CChapterBuilder* CTemporarySupportElevationsChapterBuilder::Clone() const
{
   return new CTemporarySupportElevationsChapterBuilder();
}

rptChapter* CTemporarySupportElevationsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CBrokerReportSpecification* pGirderRptSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, dim, pDisplayUnits->GetSpanLengthUnit(), false);

   GET_IFACE2(pBroker, IBridge, pBridge);
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   if (nTS == 0)
   {
      *pPara << _T("No temporary supports modeled") << rptNewLine;
   }

   GET_IFACE2_NOCHECK(pBroker, ITempSupport, pTempSupport); // not used if no temp supports

   std::array<std::_tstring, 2> strMemberEnd{ _T("Start"),_T("End") };

   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> vElevDetails = pTempSupport->GetElevationDetails(tsIdx);

      CString strTitle;
      strTitle.Format(_T("Temporary Support %d - %s"), LABEL_TEMPORARY_SUPPORT(tsIdx), CTemporarySupportData::AsString(pBridge->GetTemporarySupportType(tsIdx)));
      rptRcTable* pTable = rptStyleManager::CreateDefaultTable(4, strTitle);

      *pPara << pTable << rptNewLine;

      ColumnIndexType col = 0;
      (*pTable)(0, col++) << _T("Girder");
      (*pTable)(0, col++) << _T("Segment");
      (*pTable)(0, col++) << _T("CL Brg") << rptNewLine << _T("Location");
      (*pTable)(0, col++) << COLHDR(_T("Bottom") << rptNewLine << _T("Girder") << rptNewLine << _T("Elevation"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());

      RowIndexType row = pTable->GetNumberOfHeaderRows();
      for (const auto& details : vElevDetails)
      {
         col = 0;
         (*pTable)(row, col++) << LABEL_GIRDER(details.girderIdx);
         (*pTable)(row, col++) << LABEL_SEGMENT(details.segmentIdx);
         if (details.bContinuous)
         {
            (*pTable)(row, col++) << _T("CL TS");
         }
         else
         {
            (*pTable)(row, col++) << strMemberEnd[details.endType];
         }
         (*pTable)(row, col++) << dim.SetValue(details.Elevation);

         row++;
      }
      *pPara << rptNewLine;
   }

   return pChapter;
}
