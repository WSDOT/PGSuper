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
#include "InsertDeleteTemporarySupport.h"
#include "PGSpliceDoc.h"

#include <EAF\EAFDisplayUnits.h>


txnInsertTemporarySupport::txnInsertTemporarySupport(SupportIndexType tsIdx,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc),
m_tsIndex(tsIdx)
{
}

std::_tstring txnInsertTemporarySupport::Name() const
{
   return _T("Insert Temporary Support");
}

std::unique_ptr<WBFL::EAF::Transaction> txnInsertTemporarySupport::CreateClone() const
{
   return std::make_unique<txnInsertTemporarySupport>(m_tsIndex,m_BridgeDescription[0],m_BridgeDescription[1]);
}

///////////////////////////////////////////////

txnDeleteTemporarySupport::txnDeleteTemporarySupport(SupportIndexType tsIdx,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc),
m_tsIndex(tsIdx)
{
}

std::_tstring txnDeleteTemporarySupport::Name() const
{
   return _T("Delete Temporary Support");
}

std::unique_ptr<WBFL::EAF::Transaction> txnDeleteTemporarySupport::CreateClone() const
{
   return std::make_unique<txnDeleteTemporarySupport>(m_tsIndex,m_BridgeDescription[0],m_BridgeDescription[1]);
}
