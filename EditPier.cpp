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
#include "EditPier.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditPier::txnEditPier(PierIndexType pierIdx,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc)
{
   m_PierIdx = pierIdx;
}

txnEditPier::~txnEditPier()
{
}

txnTransaction* txnEditPier::CreateClone() const
{
   return new txnEditPier(m_PierIdx,m_BridgeDescription[0],m_BridgeDescription[1]);
}

std::_tstring txnEditPier::Name() const
{
   std::_tostringstream os;
   os << "Edit Pier " << LABEL_PIER(m_PierIdx);
   return os.str();
}
