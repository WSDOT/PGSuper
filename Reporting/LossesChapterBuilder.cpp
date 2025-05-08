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

#include "StdAfx.h"
#include <Reporting\LossesChapterBuilder.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\Bridge.h>


CLossesChapterBuilder::CLossesChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CLossesChapterBuilder::GetName() const
{
   return TEXT("Prestress Loss Details");
}

rptChapter* CLossesChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec  = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pBroker = pGdrRptSpec->GetBroker();
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(pBroker, ILosses,          pILosses     );
   GET_IFACE2(pBroker, IBridge,          pBridge      );

   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      pILosses->ReportLosses(thisGirderKey,pChapter,pDisplayUnits);
   }

   return pChapter;
}
