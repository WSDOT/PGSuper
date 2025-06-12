///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2025  Washington State Department of Transportation
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

// PGSuperExporter.cpp : Implementation of CPGSuperExporter
#include "stdafx.h"
#include "IEPluginExample.h"
#include "PGSuperDataExporter.h"

#include "PGSuperInterfaces.h"
#include <PsgLib\GirderLabel.h>

#include <EAF/EAFProgress.h>


CPGSuperDataExporter::CPGSuperDataExporter()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_Bitmap.LoadBitmap(IDB_IEPLUGIN));
}

STDMETHODIMP CPGSuperDataExporter::Init(UINT nCmdID)
{
   return S_OK;
}

CString CPGSuperDataExporter::GetMenuText() const
{
   return CString("Bridge Data to Text File");
}

HBITMAP CPGSuperDataExporter::GetBitmapHandle() const
{
   return m_Bitmap;
}

CString CPGSuperDataExporter::GetCommandHintText() const
{
   return CString("Status line hint text\nTool tip text");
}

HRESULT CPGSuperDataExporter::Export(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
   // write some bridge data to a text file
   CFileDialog  fildlg(FALSE, _T("txt"), _T("PGSuperExport.txt"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("PGSuper Export File (*.txt)|*.txt||"));
   if (fildlg.DoModal() == IDOK)
   {
      CString file_path = fildlg.GetPathName();

      std::_tofstream ofile(file_path);

      GET_IFACE2(pBroker, IBridge, pBridge);
      GET_IFACE2(pBroker, IEAFProgress, pProgress);

      ofile << _T("Pier 1: Station ") << pBridge->GetPierStation(0) << std::endl;

      SpanIndexType nSpans = pBridge->GetSpanCount();
      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
      {
         ofile << _T("Span ") << LABEL_SPAN(spanIdx) << std::endl;
         GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
         for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
         {
            CSegmentKey segmentKey(spanIdx, gdrIdx, 0);
            ofile << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Length: ") << pBridge->GetSegmentLength(segmentKey) << std::endl;
         }

         ofile << _T("Pier ") << LABEL_PIER(spanIdx + 1) << _T(": Station ") << pBridge->GetPierStation(spanIdx + 1) << std::endl;
      }

   }

   return S_OK;
}
