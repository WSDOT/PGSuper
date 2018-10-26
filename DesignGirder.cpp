///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "DesignGirder.h"
#include "PGSuperDoc.h"
#include <DesignConfigUtil.h>
#include <PgsExt\BridgeDescription.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\GirderHandling.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnDesignGirder::txnDesignGirder( std::vector<const pgsDesignArtifact*>& artifacts, pgsTypes::SlabOffsetType slabOffsetType)
{
   for (std::vector<const pgsDesignArtifact*>::iterator it= artifacts.begin(); it != artifacts.end(); it++)
   {
      DesignData data;
      data.m_DesignArtifact = *(*it);
      data.m_SlabOffsetType[1] = slabOffsetType;

      m_DesignDataColl.push_back(data);
   }

   m_bInit = false;
}

txnDesignGirder::~txnDesignGirder()
{
}

bool txnDesignGirder::Execute()
{
   if ( !m_bInit )
      Init();

   DoExecute(1);
   return true;
}

void txnDesignGirder::Undo()
{
   DoExecute(0);
}

txnTransaction* txnDesignGirder::CreateClone() const
{
   pgsTypes::SlabOffsetType slabtype;
   std::vector<const pgsDesignArtifact*> artifacts;
   bool first = true;
   for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      artifacts.push_back(&(iter->m_DesignArtifact));

      if (first)
         slabtype = iter->m_SlabOffsetType[1];

      first = false;
   }

   return new txnDesignGirder(artifacts, slabtype);
}

std::_tstring txnDesignGirder::Name() const
{
   std::_tostringstream os;
   if ( m_DesignDataColl.size() == 1 )
   {
      DesignDataConstIter iter = m_DesignDataColl.begin();
      os << _T("Design for Span ") << LABEL_SPAN(iter->m_DesignArtifact.GetSpan()) << _T(", Girder ") << LABEL_GIRDER(iter->m_DesignArtifact.GetGirder());
   }
   else
   {
      os << _T("Design for (Span, Girder) =");
      for (DesignDataConstIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
      {
         os << _T(" (") << LABEL_SPAN(iter->m_DesignArtifact.GetSpan()) << _T(", ") << LABEL_GIRDER(iter->m_DesignArtifact.GetGirder())<< _T(")");
      }
   }

   return os.str();
}

bool txnDesignGirder::IsUndoable()
{
   return true;
}

bool txnDesignGirder::IsRepeatable()
{
   return false;
}

void txnDesignGirder::Init()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

      SpanIndexType   span  = rdata.m_DesignArtifact.GetSpan();
      GirderIndexType gdr   = rdata.m_DesignArtifact.GetGirder();

      // old (existing) girder data
      rdata.m_GirderData[0] = *(pGirderData->GetGirderData(span, gdr));

      // new girder data. Artifact does the work
      rdata.m_GirderData[1] = rdata.m_DesignArtifact.GetGirderData();

      const CGirderTypes* pGirderTypes = pBridgeDesc->GetSpan(span)->GetGirderTypes();

      // deck offset
      rdata.m_SlabOffset[pgsTypes::metStart][0] = pGirderTypes->GetSlabOffset(gdr,pgsTypes::metStart);
      rdata.m_SlabOffset[pgsTypes::metEnd][0]   = pGirderTypes->GetSlabOffset(gdr,pgsTypes::metEnd);
      rdata.m_SlabOffset[pgsTypes::metStart][1] = rdata.m_DesignArtifact.GetSlabOffset(pgsTypes::metStart);
      rdata.m_SlabOffset[pgsTypes::metEnd][1]   = rdata.m_DesignArtifact.GetSlabOffset(pgsTypes::metEnd);

      // get old deck offset type... new type was set in constructor
      rdata.m_SlabOffsetType[0] = pBridgeDesc->GetSlabOffsetType();
   }

   m_bInit = true; // initialization is complete, don't do it again
}

void txnDesignGirder::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   // Loop over all girder designs
   for (DesignDataIter iter = m_DesignDataColl.begin(); iter!=m_DesignDataColl.end(); iter++)
   {
      DesignData& rdata = *iter;

      SpanIndexType span  = rdata.m_DesignArtifact.GetSpan();
      GirderIndexType gdr = rdata.m_DesignArtifact.GetGirder();
      arDesignOptions design_options = rdata.m_DesignArtifact.GetDesignOptions();

      // Set girder data for either flexural or shear design
      if (design_options.doDesignForFlexure != dtNoDesign || design_options.doDesignForShear)
      {
         pGirderData->SetGirderData(rdata.m_GirderData[i], span, gdr);

         // Slab offset set only for flexural design
         if ( design_options.doDesignForFlexure!=dtNoDesign && design_options.doDesignSlabOffset )
         {
            if ( rdata.m_SlabOffsetType[i] == pgsTypes::sotBridge )
            {
               pIBridgeDesc->SetSlabOffset(rdata.m_SlabOffset[pgsTypes::metStart][i]);
            }
            else
            {
               pIBridgeDesc->SetSlabOffset( span, gdr,rdata. m_SlabOffset[pgsTypes::metStart][i], rdata.m_SlabOffset[pgsTypes::metEnd][i]);
            }
         }
      }
   }

  pEvents->FirePendingEvents();
}

