///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include <Reporting\StrandSlopeCheck.h>

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\StrandSlopeArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandSlopeCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandSlopeCheck::CStrandSlopeCheck()
{
}

CStrandSlopeCheck::CStrandSlopeCheck(const CStrandSlopeCheck& rOther)
{
   MakeCopy(rOther);
}

CStrandSlopeCheck::~CStrandSlopeCheck()
{
}

//======================== OPERATORS  =======================================
CStrandSlopeCheck& CStrandSlopeCheck::operator= (const CStrandSlopeCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CStrandSlopeCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   // scan all the segments to see if there is an applicable strand slope check
   bool bCheck = false;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStrandSlopeArtifact* pArtifact = pSegmentArtifact->GetStrandSlopeArtifact();
      if ( pArtifact->IsApplicable() )
      {
         bCheck = true;
         break; // there is one... so we can report
      }
   }

   if ( bCheck == false )
   {
      // nothing to check
      return;
   }

   rptParagraph* pTitle = new rptParagraph( rptStyleManager::GetHeadingStyle() );
   *pChapter << pTitle;
   *pTitle << _T("Strand Slope");

   INIT_SCALAR_PROTOTYPE(rptRcScalar, slope, pDisplayUnits->GetScalarFormat());

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;
   *pBody << rptRcImage(std::_tstring(rptStyleManager::GetImagePath()) + _T("Strand Slope.jpg") ) << rptNewLine;

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsStrandSlopeArtifact* pArtifact = pSegmentArtifact->GetStrandSlopeArtifact();
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
      
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(2);
         *pBody << pTable;

         pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle( CB_NONE | CJ_LEFT) );
         pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

         (*pTable)(0,0) << _T("");
         (*pTable)(0,1) << _T("1 : n");
         (*pTable)(1,0) << _T("Allowable Slope");
         (*pTable)(2,0) << _T("Strand Slope");
         (*pTable)(3,0) << _T("Status");

         (*pTable)(1,1) << slope.SetValue(pArtifact->GetCapacity());
         (*pTable)(2,1) << slope.SetValue(pArtifact->GetDemand());

         if ( pArtifact->Passed() )
         {
            (*pTable)(3,1) << RPT_PASS;
         }
         else
         {
            (*pTable)(3,1) << RPT_FAIL;
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
void CStrandSlopeCheck::MakeCopy(const CStrandSlopeCheck& rOther)
{
   // Add copy code here...
}

void CStrandSlopeCheck::MakeAssignment(const CStrandSlopeCheck& rOther)
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
