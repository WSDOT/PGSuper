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
#pragma once;

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MultiGirderSelectGrid.h : header file
//
#include <WBFLTypes.h>
#include <Lrfd\Lrfd.h>

inline void FillRebarMaterialComboBox(CComboBox* pCB,bool bFilterBySpec = true)
{
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A615,  matRebar::Grade40).c_str()  );
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A615,  matRebar::Grade60).c_str()  );
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A615,  matRebar::Grade75).c_str()  );
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A615,  matRebar::Grade80).c_str()  );
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A706,  matRebar::Grade60).c_str()  );
   pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A706,  matRebar::Grade80).c_str()  );

   if ( bFilterBySpec )
   {
      if ( lrfdVersionMgr::SixthEditionWith2013Interims <= lrfdVersionMgr::GetVersion() )
      {
         pCB->AddString( lrfdRebarPool::GetMaterialName(matRebar::A1035, matRebar::Grade100).c_str() );
      }
   }
}

inline void GetStirrupMaterial(int idx,matRebar::Type& type,matRebar::Grade& grade)
{
   switch(idx)
   {
   case 0:  type = matRebar::A615;  grade = matRebar::Grade40;  break;
   case 1:  type = matRebar::A615;  grade = matRebar::Grade60;  break;
   case 2:  type = matRebar::A615;  grade = matRebar::Grade75;  break;
   case 3:  type = matRebar::A615;  grade = matRebar::Grade80;  break;
   case 4:  type = matRebar::A706;  grade = matRebar::Grade60;  break;
   case 5:  type = matRebar::A706;  grade = matRebar::Grade80;  break;
   case 6:  type = matRebar::A1035; grade = matRebar::Grade100; break;
   default:
      ATLASSERT(false); // should never get here
   }
}

inline int GetStirrupMaterialIndex(matRebar::Type type,matRebar::Grade grade)
{
   if ( type == matRebar::A615 )
   {
      if ( grade == matRebar::Grade40 )
         return 0;
      else if ( grade == matRebar::Grade60 )
         return 1;
      else if ( grade == matRebar::Grade75 )
         return 2;
      else if ( grade == matRebar::Grade80 )
         return 3;
   }
   else if ( type == matRebar::A706 )
   {
      if ( grade == matRebar::Grade60 )
         return 4;
      else if ( grade == matRebar::Grade80 )
         return 5;
   }
   else if ( type == matRebar::A1035 )
   {
      if ( grade == matRebar::Grade100 )
         return 6;
   }

   ATLASSERT(false); // should never get here
   return -1;
}
