///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "EditGirderline.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditGirderline::txnEditGirderline(const CGirderKey& girderKey,bool bApplyToAllGirderlines,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc) :
txnEditBridgeDescription(oldBridgeDesc,newBridgeDesc)
{
   m_GirderKey = girderKey;
   m_bApplyToAllGirderlines = bApplyToAllGirderlines;
   m_bGirderlineCopied = false;
}

txnEditGirderline::~txnEditGirderline()
{
}

txnTransaction* txnEditGirderline::CreateClone() const
{
   return new txnEditGirderline(m_GirderKey,m_bApplyToAllGirderlines,m_BridgeDescription[0],m_BridgeDescription[1]);
}

std::_tstring txnEditGirderline::Name() const
{
   std::_tostringstream os;
   os << "Edit Girder " << LABEL_GIRDER(m_GirderKey.girderIndex);
   return os.str();
}

bool txnEditGirderline::Execute()
{
   if ( m_bApplyToAllGirderlines && !m_bGirderlineCopied )
   {
      CGirderGroupData* pGroup = m_BridgeDescription[1].GetGirderGroup(m_GirderKey.groupIndex);
      CSplicedGirderData* pGirder = pGroup->GetGirder(m_GirderKey.girderIndex);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         if ( gdrIdx == m_GirderKey.girderIndex )
            continue;

         CSplicedGirderData* pOtherGirder = pGroup->GetGirder(gdrIdx);
         pOtherGirder->CopySplicedGirderData(pGirder);

         pGroup->CopySlabOffset(m_GirderKey.groupIndex,gdrIdx);
      }
      m_bGirderlineCopied = true;
   }

   return txnEditBridgeDescription::Execute();
}
