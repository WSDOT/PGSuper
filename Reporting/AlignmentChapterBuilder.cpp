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

#include "StdAfx.h"
#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


#include <WBFLCogo.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CAlignmentChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CAlignmentChapterBuilder::CAlignmentChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CAlignmentChapterBuilder::GetName() const
{
   return TEXT("Alignment");
}

rptChapter* CAlignmentChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pBrokerSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);

   // This report does not use the passd span and girder parameters
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CComPtr<IBroker> pBroker;
   pBrokerSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CBridgeDescChapterBuilder::WriteAlignmentData(pBroker,pDisplayUnits,pChapter,level);
   CBridgeDescChapterBuilder::WriteProfileData(pBroker,pDisplayUnits,pChapter,level);
   CBridgeDescChapterBuilder::WriteCrownData(pBroker,pDisplayUnits,pChapter,level);

   return pChapter;
}


CChapterBuilder* CAlignmentChapterBuilder::Clone() const
{
   return new CAlignmentChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================


