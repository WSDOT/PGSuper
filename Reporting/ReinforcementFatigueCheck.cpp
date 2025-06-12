///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <Reporting\ReinforcementFatigueCheck.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\ReinforcementFatigueArtifact.h>


CReinforcementFatigueCheck::CReinforcementFatigueCheck()
{
}

CReinforcementFatigueCheck::~CReinforcementFatigueCheck()
{
}

void CReinforcementFatigueCheck::Build(rptChapter* pChapter,std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsGirderArtifact* pGirderArtifact,
                                       std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   bool bIsApplicable = false;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetReinforcementFatigueArtifact();

      // no report if not applicable
      if ( pArtifact->IsApplicable() )
      {
         bIsApplicable = true;
         break;
      }
   }

   if ( !bIsApplicable )
   {
      return;
   }

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Reinforcement Fatigue (GS 1.5.3)");
   pTitle->SetName(_T("Reinforcement Fatigue"));

   INIT_UV_PROTOTYPE(rptStressUnitValue, modE, pDisplayUnits->GetModEUnit(), false);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptLengthUnitValue, length, pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE(rptLength4UnitValue, inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false);
   INIT_UV_PROTOTYPE(rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), false);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("ReinforcementFatigue.png")) << rptNewLine;

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto* pArtifact = pSegmentArtifact->GetReinforcementFatigueArtifact();
      // no report if not applicable
      if ( pArtifact->IsApplicable() )
      {
         if ( 1 < nSegments )
         {
            pBody = new rptParagraph(rptStyleManager::GetSubheadingStyle());
            *pChapter << pBody;
            *pBody << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
         }
      
         pBody = new rptParagraph;
         *pChapter << pBody;
      
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10,nullptr);
         *pBody << pTable;

         rptParagraph* pFootnote = new rptParagraph(rptStyleManager::GetFootnoteStyle());
         *pChapter << pFootnote;
         *pFootnote << Sub2(_T("M"),_T("f")) << _T(" = Fatigue Live Load Moment per Girder") << rptNewLine;
         *pFootnote << _T("e = eccentricity of strand furthest from composite section centroid") << rptNewLine;

         RowIndexType row = 0;
         ColumnIndexType col = 0;
         (*pTable)(row, col++) << symbol(gamma);
         (*pTable)(row, col++) << COLHDR(RPT_EPS, rptStressUnitTag, pDisplayUnits->GetModEUnit());
         (*pTable)(row, col++) << COLHDR(RPT_EC, rptStressUnitTag, pDisplayUnits->GetModEUnit());
         (*pTable)(row, col++) << COLHDR(Sub2(_T("M"),_T("f")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit());
         (*pTable)(row, col++) << COLHDR(_T("e"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(row, col++) << COLHDR(_T("I"), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit());
         (*pTable)(row, col++) << COLHDR(symbol(DELTA) << _T("f"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(row, col++) << COLHDR(symbol(gamma) << _T("(") << symbol(DELTA) << _T("f") << _T(")"), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(row, col++) << COLHDR(_T("(") << symbol(DELTA) << _T("F") << _T(")") << Sub(_T("TH")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(row, col++) << _T("Status");

         row = 1;
         col = 0;
         (*pTable)(row, col++) << pArtifact->GetLoadFactor();
         (*pTable)(row, col++) << modE.SetValue(pArtifact->GetEps());
         (*pTable)(row, col++) << modE.SetValue(pArtifact->GetEc());
         (*pTable)(row, col++) << moment.SetValue(pArtifact->GetFatigueLiveLoadMoment());
         (*pTable)(row, col++) << length.SetValue(pArtifact->GetStrandEccentricity());
         (*pTable)(row, col++) << inertia.SetValue(pArtifact->GetMomentOfInertia());
         (*pTable)(row, col++) << stress.SetValue(pArtifact->GetLiveLoadStressRange());
         (*pTable)(row, col++) << stress.SetValue(pArtifact->GetLoadFactor()*pArtifact->GetLiveLoadStressRange());
         (*pTable)(row, col++) << stress.SetValue(pArtifact->GetFatigueThreshold());
         if ( pArtifact->Passed() )
         {
            (*pTable)(row, col++) << RPT_PASS;
         }
         else
         {
            (*pTable)(row, col++) << RPT_FAIL;
         }
      } // is applicable
   } // next segment
}
