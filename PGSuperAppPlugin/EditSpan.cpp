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
#include "EditSpan.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditSpan::txnEditSpan(SpanIndexType spanIdx,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc)
{
   m_SpanIdx = spanIdx;
}

txnEditSpan::~txnEditSpan()
{
}

std::unique_ptr<CEAFTransaction> txnEditSpan::CreateClone() const
{
   return std::make_unique<txnEditSpan>(m_SpanIdx,m_BridgeDescription[0],m_BridgeDescription[1]);
}

std::_tstring txnEditSpan::Name() const
{
   std::_tostringstream os;
   os << "Edit Span " << LABEL_SPAN(m_SpanIdx);
   return os.str();
}
