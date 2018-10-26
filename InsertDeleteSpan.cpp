///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "InsertDeleteSpan.h"
#include "PGSuperDocBase.h"

#include <PgsExt\ClosureJointData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnInsertSpan::txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,IndexType pierErectionEventIdx)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;
   m_SpanLength = spanLength;
   m_bCreateNewGroup = bCreateNewGroup;
   m_PierErectionEventIndex = pierErectionEventIdx;

   Init();
}

std::_tstring txnInsertSpan::Name() const
{
   return _T("Insert Span");
}

txnTransaction* txnInsertSpan::CreateClone() const
{
   return new txnInsertSpan(m_RefPierIdx, m_PierFace,m_SpanLength,m_bCreateNewGroup,m_PierErectionEventIndex);
}

bool txnInsertSpan::IsUndoable()
{
   return true;
}

bool txnInsertSpan::IsRepeatable()
{
   return false;
}

bool txnInsertSpan::Execute()
{
   DoExecute(1);
   return true;
}

void txnInsertSpan::Undo()
{
   DoExecute(0);
}

void txnInsertSpan::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   m_BridgeDescription[0] = *(pIBridgeDesc->GetBridgeDescription());
   m_BridgeDescription[1] = m_BridgeDescription[0];

   m_BridgeDescription[1].InsertSpan(m_RefPierIdx,m_PierFace,m_SpanLength,NULL,NULL,m_bCreateNewGroup,m_PierErectionEventIndex);
}

void txnInsertSpan::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->SetBridgeDescription(m_BridgeDescription[i]);

   pEvents->FirePendingEvents();
}

///////////////////////////////////////////////

txnDeleteSpan::txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace)
{
   m_RefPierIdx = refPierIdx;
   m_PierFace   = pierFace;

   Init();
}

txnDeleteSpan::~txnDeleteSpan()
{
}

std::_tstring txnDeleteSpan::Name() const
{
   return _T("Delete Span");
}

txnTransaction* txnDeleteSpan::CreateClone() const
{
   return new txnDeleteSpan(m_RefPierIdx, m_PierFace);
}

bool txnDeleteSpan::IsUndoable()
{
   return true;
}

bool txnDeleteSpan::IsRepeatable()
{
   return false;
}

bool txnDeleteSpan::Execute()
{
   DoExecute(1);
   return true;
}

void txnDeleteSpan::Undo()
{
   DoExecute(0);
}

void txnDeleteSpan::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   m_BridgeDescription[0] = *(pIBridgeDesc->GetBridgeDescription());
   m_BridgeDescription[1] = m_BridgeDescription[0];

   SpanIndexType spanIdx;
   pgsTypes::RemovePierType removePierType;

   if (m_PierFace == pgsTypes::Back )
   {
      spanIdx = m_RefPierIdx - 1;
      removePierType = pgsTypes::NextPier;
   }
   else
   {
      spanIdx = m_RefPierIdx;
      removePierType = pgsTypes::PrevPier;
   }

   m_BridgeDescription[1].RemoveSpan(spanIdx,removePierType);
}

void txnDeleteSpan::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents,pEvents);
   pEvents->HoldEvents();

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   pIBridgeDesc->SetBridgeDescription(m_BridgeDescription[i]);

   pEvents->FirePendingEvents();
}
