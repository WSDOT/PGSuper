///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "EditProjectCriteria.h"
#include <IFace\Project.h> // for IEvents and ISpecification
#include "PGSuperDoc.h" // for EAFGetBroker

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditProjectCriteria::txnEditProjectCriteria(LPCTSTR strOldCriteria,LPCTSTR strNewCriteria,pgsTypes::AnalysisType oldAnalysisType,pgsTypes::AnalysisType newAnalysisType,pgsTypes::WearingSurfaceType oldWearingSurfaceType,pgsTypes::WearingSurfaceType newWearingSurfaceType)
{
   m_strProjectCriteria[0] = strOldCriteria;
   m_strProjectCriteria[1] = strNewCriteria;

   m_AnalysisType[0] = oldAnalysisType;
   m_AnalysisType[1] = newAnalysisType;

   m_WearingSurfaceType[0] = oldWearingSurfaceType;
   m_WearingSurfaceType[1] = newWearingSurfaceType;
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
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   pSpec->SetAnalysisType(m_AnalysisType[i]);
   pSpec->SetSpecification( m_strProjectCriteria[i] );

   GET_IFACE2(pBroker,IBridgeDescription,pBridgeDesc);
   pBridgeDesc->SetWearingSurfaceType(m_WearingSurfaceType[i]);
}

std::unique_ptr<CEAFTransaction> txnEditProjectCriteria::CreateClone() const
{
   return std::make_unique<txnEditProjectCriteria>(m_strProjectCriteria[0].c_str(),m_strProjectCriteria[1].c_str(),m_AnalysisType[0],m_AnalysisType[1],m_WearingSurfaceType[0],m_WearingSurfaceType[1]);
}

std::_tstring txnEditProjectCriteria::Name() const
{
   return _T("Edit Design Criteria");
}

bool txnEditProjectCriteria::IsUndoable() const
{
   return true;
}

bool txnEditProjectCriteria::IsRepeatable() const
{
   return false;
}
