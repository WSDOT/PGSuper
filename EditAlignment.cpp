///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "StdAfx.h"
#include "EditAlignment.h"
#include "PGSuper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditAlignment::txnEditAlignment(const AlignmentData2& oldAlignmentData,   const AlignmentData2& newAlignmentData,
                                   const ProfileData2& oldProfileData,       const ProfileData2& newProfileData,
                                	  const RoadwaySectionData& oldSectionData, const RoadwaySectionData& newSectionData)

{
   m_AlignmentData[0] = oldAlignmentData;
   m_AlignmentData[1] = newAlignmentData;

   m_ProfileData[0] = oldProfileData;
   m_ProfileData[1] = newProfileData;

   m_SectionData[0] = oldSectionData;
   m_SectionData[1] = newSectionData;
}

txnEditAlignment::~txnEditAlignment()
{
}

bool txnEditAlignment::Execute()
{
   Execute(1);
   return true;
}

void txnEditAlignment::Undo()
{
   Execute(0);
}

void txnEditAlignment::Execute(int i)
{
   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   GET_IFACE2(pBroker,IRoadwayData,pAlignment);
   GET_IFACE2(pBroker,IEvents, pEvents);

   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   pAlignment->SetAlignmentData2(m_AlignmentData[i]);
   pAlignment->SetProfileData2(m_ProfileData[i]);
   pAlignment->SetRoadwaySectionData(m_SectionData[i]);

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditAlignment::CreateClone() const
{
   return new txnEditAlignment(m_AlignmentData[0], m_AlignmentData[1],
                               m_ProfileData[0],   m_ProfileData[1],
                               m_SectionData[0],   m_SectionData[1]);
}

std::string txnEditAlignment::Name() const
{
   return "Edit Alignment";
}

bool txnEditAlignment::IsUndoable()
{
   return true;
}

bool txnEditAlignment::IsRepeatable()
{
   return false;
}
