///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "resource.h"
#include <PgsExt\Prompts.h>
#include "SelectGirderDlg.h"
#include "SelectSegmentDlg.h"
#include "SelectPoiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT_PTR pgsSelectGirder(CGirderKey& girderKey)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CSelectGirderDlg dlg;
   dlg.m_GirderKey = girderKey;
   UINT_PTR result = dlg.DoModal();
   if ( result == IDOK )
   {
      girderKey = dlg.m_GirderKey;
   }

   return result;
}

UINT_PTR PGSEXTFUNC pgsSelectSegment(CSegmentKey& segmentKey)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CSelectSegmentDlg dlg;
   dlg.m_SegmentKey = segmentKey;
   UINT_PTR result = dlg.DoModal();
   if ( result == IDOK )
   {
      segmentKey = dlg.m_SegmentKey;
   }

   return result;
}

UINT_PTR PGSEXTFUNC pgsSelectPointOfInterest(pgsPointOfInterest& poi,IntervalIndexType& intervalIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CSelectPOIDlg dlg;
   dlg.m_InitialPOI = poi;
   dlg.m_IntervalIdx = intervalIdx;
   UINT_PTR result = dlg.DoModal();
   if ( result == IDOK )
   {
      poi = dlg.GetPOI();
      intervalIdx = dlg.m_IntervalIdx;
   }

   return result;
}
