///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\CustomDDX.h>
#include <LRFD\StrandPool.h>

void DDX_Strand(CDataExchange* pDX, UINT nIDC, const WBFL::Materials::PsStrand** ppStrand)
{
   const auto* pPool = WBFL::LRFD::StrandPool::GetInstance();
   CComboBox* pList = (CComboBox*)pDX->m_pDlgWnd->GetDlgItem(nIDC);

   if (pDX->m_bSaveAndValidate)
   {
      // strand material
      int curSel = pList->GetCurSel();
      auto key = (Int64)pList->GetItemData(curSel);
      *ppStrand = pPool->GetStrand(key);
   }
   else
   {
      auto target_key = pPool->GetStrandKey(*ppStrand);
      int cStrands = pList->GetCount();
      for (int i = 0; i < cStrands; i++)
      {
         auto key = (Int64)pList->GetItemData(i);
         if (key == target_key)
         {
            pList->SetCurSel(i);
            break;
         }
      }
   }
}
