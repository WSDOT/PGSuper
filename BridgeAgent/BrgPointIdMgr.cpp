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
#include "BrgPointIdMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CBrgPointIdMgr
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CBrgPointIdMgr::CBrgPointIdMgr()
{
}

CBrgPointIdMgr::~CBrgPointIdMgr()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CBrgPointIdMgr::Reset()
{
   m_Ids.clear();
}

//======================== ACCESS     =======================================

Uint32 get_index( SpanIndexType spanIdx, CBrgPointIdMgr::Location loc )
{
   ATLASSERT(spanIdx != ALL_SPANS);
   Uint32 idx = Uint32(2*spanIdx - 2);
   if ( loc == CBrgPointIdMgr::End )
   {
      idx++;
   }

   return idx;
}

Int32 CBrgPointIdMgr::GetId( SpanIndexType spanIdx, Location loc )
{
   return m_Ids[get_index(spanIdx,loc)];
}

void CBrgPointIdMgr::SetId( SpanIndexType spanIdx, Location loc, Int32 id)
{
   Uint32 idx = get_index(spanIdx,loc);
   if ( m_Ids.size() <= idx )
   {
      m_Ids.resize( 2*idx );
   }

   m_Ids[idx] = id;
}

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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CBrgPointIdMgr::AssertValid() const
{
   return true;
}

void CBrgPointIdMgr::Dump(dbgDumpContext& os) const
{
   os << "Dump for CBrgPointIdMgr" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CBrgPointIdMgr::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CBrgPointIdMgr");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CBrgPointIdMgr");

   TESTME_EPILOG("BrgPointIdMgr");
}
#endif // _UNITTEST
