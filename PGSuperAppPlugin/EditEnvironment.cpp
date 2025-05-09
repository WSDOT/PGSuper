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
#include "EditEnvironment.h"
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditEnvironment::txnEditEnvironment(pgsTypes::ExposureCondition oldExposureCondition, pgsTypes::ExposureCondition newExposureCondition,
                                       Float64 oldRelHumidity, Float64 newRelHumidity)
{
   m_ExposureCondition[0] = oldExposureCondition;
   m_ExposureCondition[1] = newExposureCondition;

   m_RelHumidity[0] = oldRelHumidity;
   m_RelHumidity[1] = newRelHumidity;
}

txnEditEnvironment::~txnEditEnvironment()
{
}

bool txnEditEnvironment::Execute()
{
   Execute(1);
   return true;
}

void txnEditEnvironment::Undo()
{
   Execute(0);
}

void txnEditEnvironment::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEnvironment, pEnvironment );
   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   pEnvironment->SetExposureCondition( m_ExposureCondition[i] );
   pEnvironment->SetRelHumidity( m_RelHumidity[i] );
}

std::unique_ptr<CEAFTransaction> txnEditEnvironment::CreateClone() const
{
   return std::make_unique<txnEditEnvironment>(m_ExposureCondition[0], m_ExposureCondition[1],
                                 m_RelHumidity[0],       m_RelHumidity[1]);
}

std::_tstring txnEditEnvironment::Name() const
{
   return _T("Edit Environmental Conditions");
}

bool txnEditEnvironment::IsUndoable() const
{
   return true;
}

bool txnEditEnvironment::IsRepeatable() const
{
   return false;
}
