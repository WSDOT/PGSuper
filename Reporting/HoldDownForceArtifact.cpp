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

#include "StdAfx.h"
#include <Reporting\HoldDownForceCheck.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>

#include <psgLib/HoldDownCriteria.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHoldDownForceCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CHoldDownForceCheck::CHoldDownForceCheck()
{
}

CHoldDownForceCheck::CHoldDownForceCheck(const CHoldDownForceCheck& rOther)
{
   MakeCopy(rOther);
}

CHoldDownForceCheck::~CHoldDownForceCheck()
{
}

//======================== OPERATORS  =======================================
CHoldDownForceCheck& CHoldDownForceCheck::operator= (const CHoldDownForceCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CHoldDownForceCheck::Build(rptChapter* pChapter,IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                                       IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   bool bIsApplicable = false;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsHoldDownForceArtifact* pArtifact = pSegmentArtifact->GetHoldDownForceArtifact();
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

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& hold_down_criteria = pSpecEntry->GetHoldDownCriteria();

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Hold Down Force");

   INIT_UV_PROTOTYPE( rptForceUnitValue, force, pDisplayUnits->GetGeneralForceUnit(), false );

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsHoldDownForceArtifact* pArtifact = pSegmentArtifact->GetHoldDownForceArtifact();
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
      
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(3,nullptr);
         *pBody << pTable;

         if (hold_down_criteria.type == HoldDownCriteria::Type::Total)
         {
            (*pTable)(0, 0) << COLHDR(_T("Hold Down Force"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
            (*pTable)(0, 1) << COLHDR(_T("Hold Down Force Limit"), rptForceUnitTag, pDisplayUnits->GetGeneralForceUnit());
         }
         else
         {
            CHECK(hold_down_criteria.type == HoldDownCriteria::Type::PerStrand);
            (*pTable)(0, 0) << _T("Hold Down Force") << rptNewLine << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T("/strand)");
            (*pTable)(0, 1) << _T("Hold Down Force Limit") << rptNewLine << _T("(") << rptForceUnitTag(&pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure) << _T("/strand)");
         }
         (*pTable)(0, 2) << _T("Status");

         (*pTable)(1,0) << force.SetValue(pArtifact->GetDemand());
         (*pTable)(1,1) << force.SetValue(pArtifact->GetCapacity());

         if ( pArtifact->Passed() )
         {
            (*pTable)(1,2) << RPT_PASS;
         }
         else
         {
            (*pTable)(1,2) << RPT_FAIL;
         }
      } // is applicable
   } // next segment
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CHoldDownForceCheck::MakeCopy(const CHoldDownForceCheck& rOther)
{
   // Add copy code here...
}

void CHoldDownForceCheck::MakeAssignment(const CHoldDownForceCheck& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
