///////////////////////////////////////////////////////////////////////
// IEPluginExample
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

// PGSpliceProjectImporter.cpp : Implementation of CPGSpliceProjectImporter
#include "stdafx.h"
#include "IEPluginExample.h"
#include "PGSpliceProjectImporter.h"

HRESULT CPGSpliceProjectImporter::FinalConstruct()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   VERIFY(m_Bitmap.LoadBitmap(IDB_IEPLUGIN));
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CPGSpliceProjectImporter

STDMETHODIMP CPGSpliceProjectImporter::GetCLSID(CLSID* pCLSID) const
{
   *pCLSID = CLSID_PGSuperProjectImporter;
   return S_OK;
}

STDMETHODIMP CPGSpliceProjectImporter::GetItemText(BSTR*  bstrText) const
{
   CComBSTR bstrItemText("Example Importer");
   *bstrText = bstrItemText.Copy();
   return S_OK;
}

STDMETHODIMP CPGSpliceProjectImporter::GetIcon(HICON* phIcon) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   *phIcon = AfxGetApp()->LoadIcon(IDI_IMPORTER);
   return S_OK;
}

STDMETHODIMP CPGSpliceProjectImporter::Import(IBroker* pBroker)
{
//   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int st = AfxMessageBox(_T("Simulate importing data from an external source by creating a default bridge. Click Yes to do it!"),MB_YESNO);

   if (st==IDYES)
   {
      return S_OK;
   }
   else
   {
      return E_FAIL;
   }
}

