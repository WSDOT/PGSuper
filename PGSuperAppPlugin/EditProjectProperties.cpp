///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "stdafx.h"
#include "EditProjectProperties.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditProjectProperties::txnEditProjectProperties(
      const std::_tstring& oldBridgeName, const std::_tstring& newBridgeName,
      const std::_tstring& oldBridgeID,   const std::_tstring& newBridgeID,
      const std::_tstring& oldJobNumber,  const std::_tstring& newJobNumber,
      const std::_tstring& oldEngineer,   const std::_tstring& newEngineer,
      const std::_tstring& oldCompany,    const std::_tstring& newCompany,
      const std::_tstring& oldComment,    const std::_tstring& newComment)
{
   m_BridgeName[0] = oldBridgeName;
   m_BridgeName[1] = newBridgeName;

   m_BridgeID[0] = oldBridgeID;
   m_BridgeID[1] = newBridgeID;

   m_JobNumber[0] = oldJobNumber;
   m_JobNumber[1] = newJobNumber;

   m_Engineer[0] = oldEngineer;
   m_Engineer[1] = newEngineer;

   m_Company[0] = oldCompany;
   m_Company[1] = newCompany;

   m_Comment[0] = oldComment;
   m_Comment[1] = newComment;
}

txnEditProjectProperties::~txnEditProjectProperties()
{
}

bool txnEditProjectProperties::Execute()
{
   Execute(1);
   return true;
}

void txnEditProjectProperties::Undo()
{
   Execute(0);
}

void txnEditProjectProperties::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IProjectProperties,pProjProp);
   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   pProjProp->SetBridgeName( m_BridgeName[i].c_str() );
   pProjProp->SetBridgeID(   m_BridgeID[i].c_str()   );
   pProjProp->SetJobNumber(  m_JobNumber[i].c_str()  );
   pProjProp->SetEngineer(   m_Engineer[i].c_str()   );
   pProjProp->SetCompany(    m_Company[i].c_str()    );
   pProjProp->SetComments(   m_Comment[i].c_str()    );
}

txnTransaction* txnEditProjectProperties::CreateClone() const
{
   return new txnEditProjectProperties(m_BridgeName[0],
                                       m_BridgeName[1],
                                       m_BridgeID[0],
                                       m_BridgeID[1],
                                       m_JobNumber[0],
                                       m_JobNumber[1],
                                       m_Engineer[0],
                                       m_Engineer[1],
                                       m_Company[0],
                                       m_Company[1],
                                       m_Comment[0],
                                       m_Comment[1]);
}

std::_tstring txnEditProjectProperties::Name() const
{
   return _T("Edit Project Properties");
}

bool txnEditProjectProperties::IsUndoable()
{
   return true;
}

bool txnEditProjectProperties::IsRepeatable()
{
   return false;
}
