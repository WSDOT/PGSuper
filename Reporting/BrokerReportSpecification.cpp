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

#include "stdafx.h"
#include <Reporting\BrokerReportSpecification.h>

#pragma Reminder("WORKING HERE - Removing COM - holding pointer to broker can create circular reference")
// These specification needs to be deleted in Agent::Reset() to break the circular reference
CBrokerReportSpecification::CBrokerReportSpecification(const std::_tstring& strReportName,std::shared_ptr<WBFL::EAF::Broker> pBroker) :
WBFL::Reporting::ReportSpecification(strReportName)
{
   SetBroker(pBroker);
}

CBrokerReportSpecification::~CBrokerReportSpecification(void)
{
}

void CBrokerReportSpecification::SetBroker(std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
   m_pBroker = pBroker;
}

std::shared_ptr<WBFL::EAF::Broker> CBrokerReportSpecification::GetBroker() const
{
   return m_pBroker;
}

bool CBrokerReportSpecification::IsValid() const
{
   if ( !m_pBroker )
      return false;

   return WBFL::Reporting::ReportSpecification::IsValid();
}