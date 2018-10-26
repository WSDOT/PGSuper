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
#include "EditProjectCriteria.h"
#include <IFace\Project.h> // for IEvents and ISpecification
#include "PGSuperDoc.h" // for EAFGetBroker

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditProjectCriteria::txnEditProjectCriteria(LPCTSTR strOldCriteria,LPCTSTR strNewCriteria,pgsTypes::AnalysisType oldAnalysisType,pgsTypes::AnalysisType newAnalysisType)
{
   m_strProjectCriteria[0] = strOldCriteria;
   m_strProjectCriteria[1] = strNewCriteria;

   m_AnalysisType[0] = oldAnalysisType;
   m_AnalysisType[1] = newAnalysisType;
}

txnEditProjectCriteria::~txnEditProjectCriteria()
{
}

bool txnEditProjectCriteria::Execute()
{
   Execute(1);
   return true;
}

void txnEditProjectCriteria::Undo()
{
   Execute(0);
}

void txnEditProjectCriteria::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   GET_IFACE2(pBroker, ISpecification, pSpec );

   pEvents->HoldEvents(); // don't fire any changed events until all changes are done
   pSpec->SetSpecification( m_strProjectCriteria[i] );
   pSpec->SetAnalysisType(m_AnalysisType[i]);
   pEvents->FirePendingEvents();
}

txnTransaction* txnEditProjectCriteria::CreateClone() const
{
   return new txnEditProjectCriteria(m_strProjectCriteria[0].c_str(),m_strProjectCriteria[1].c_str(),m_AnalysisType[0],m_AnalysisType[1]);
}

std::_tstring txnEditProjectCriteria::Name() const
{
   return _T("Edit Design Criteria");
}

bool txnEditProjectCriteria::IsUndoable()
{
   return true;
}

bool txnEditProjectCriteria::IsRepeatable()
{
   return false;
}
