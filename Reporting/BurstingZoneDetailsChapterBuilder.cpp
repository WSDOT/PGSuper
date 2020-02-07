///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\BurstingZoneDetailsChapterBuilder.h>

#include <Reporting\SpanGirderReportSpecification.h>

#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSplittingZoneDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSplittingZoneDetailsChapterBuilder::CSplittingZoneDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSplittingZoneDetailsChapterBuilder::GetKey() const
{
   return TEXT("Splitting Resistance Details");
}

LPCTSTR CSplittingZoneDetailsChapterBuilder::GetName() const
{
   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
   {
      return TEXT("Splitting Resistance Details");
   }
   else
   {
      return TEXT("Bursting Resistance Details");
   }
}

rptChapter* CSplittingZoneDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   GET_IFACE2_NOCHECK(pBroker, ILossParameters, pLossParams);

   bool bInitialRelaxation = ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 || 
                               pLossParams->GetLossMethod() == pgsTypes::WSDOT_REFINED                 ||
                               pLossParams->GetLossMethod() == pgsTypes::TXDOT_REFINED_2004            ||
                               pLossParams->GetLossMethod() == pgsTypes::WSDOT_LUMPSUM                 ||
                               pLossParams->GetLossMethod() == pgsTypes::TIME_STEP               ? true : false );


   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(),   true );
   INIT_UV_PROTOTYPE(rptLengthUnitValue, short_length, pDisplayUnits->GetComponentDimUnit(), true);
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,   pDisplayUnits->GetAreaUnit(),         true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,     force,  pDisplayUnits->GetGeneralForceUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(4);
   scalar.SetPrecision(1);

   GET_IFACE2_NOCHECK(pBroker, IMaterials, pMaterials);

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      bool bUHPC = pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC ? true : false;

      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsSegmentArtifact* segArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
      const pgsSplittingZoneArtifact* pArtifact = segArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();

      rptParagraph* pPara;

      if ( 1 < nSegments )
      {
         pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }
      
      pPara = new rptParagraph;
      *pChapter << pPara;

      std::_tstring strName;
      if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      {
         strName = _T("Splitting");
      }
      else
      {
         strName = _T("Bursting");
      }

      if (pArtifact->GetIsApplicable())
      {
         (*pPara) << _T("LRFD ") << LrfdCw8th(_T("5.10.10.1"),_T("5.9.4.4.1")) << rptNewLine;

         for (int i = 0; i < 2; i++)
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

            bool bTempStrands = IsZero(pArtifact->GetAps(endType, pgsTypes::Temporary)) ? false : true;

            if (endType == pgsTypes::metStart)
            {
               (*pPara) << Bold(_T("Left End:")) << rptNewLine;
            }
            else
            {
               (*pPara) << Bold(_T("Right End:")) << rptNewLine;
            }
            (*pPara) << strName << _T(" Dimension: h = ") << length.SetValue(pArtifact->GetH(endType)) << _T(" = ") << short_length.SetValue(pArtifact->GetH(endType)) << rptNewLine;
            (*pPara) << strName << _T(" Length: h/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(" = ") << length.SetValue(pArtifact->GetSplittingZoneLength(endType)) << _T(" = ") << short_length.SetValue(pArtifact->GetSplittingZoneLength(endType)) << rptNewLine;
            (*pPara) << strName << _T(" Direction: ") << (pArtifact->GetSplittingDirection() == pgsTypes::sdVertical ? _T("Vertical") : _T("Horizontal")) << rptNewLine;
            
            if (bTempStrands)
            {
               (*pPara) << strName << _T(" Force at PSXFR: P = ") << Sub2(_T("P"), _T("perm")) << _T(" + ") << Sub2(_T("P"), _T("temp")) << rptNewLine;
               (*pPara) << Sub2(_T("P"), _T("perm")) << _T(" = 0.04A") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

               if (bInitialRelaxation)
               {
                  if (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004)
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
                  }
                  else
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
                  }
               }

               (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
               if (bInitialRelaxation)
               {
                  (*pPara) << _T(" 0.04(") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
               }

               (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Permanent)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Permanent)) << _T(" - ");
               (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Permanent)) << _T(" ) = ") << force.SetValue(pArtifact->GetSplittingForce(endType, pgsTypes::Permanent)) << rptNewLine;

               (*pPara) << Sub2(_T("P"), _T("temp")) << _T(" = 0.04(") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

               if (bInitialRelaxation)
               {
                  if (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004)
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
                  }
                  else
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
                  }
               }

               (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
               if (bInitialRelaxation)
               {
                  (*pPara) << _T(" 0.04(") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
               }

               (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Temporary)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Temporary)) << _T(" - ");
               (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Temporary)) << _T(" ) = ") << force.SetValue(pArtifact->GetSplittingForce(endType,pgsTypes::Temporary)) << rptNewLine;

               (*pPara) << _T("P = ") << force.SetValue(pArtifact->GetTotalSplittingForce(endType)) << rptNewLine;
            }
            else
            {
               (*pPara) << strName << _T(" Force at PSXFR: P = 0.04(") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ");

               if (bInitialRelaxation)
               {
                  if (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004)
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
                  }
                  else
                  {
                     (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
                  }
               }

               (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES")) << _T(") = ");
               if (bInitialRelaxation)
               {
                  (*pPara) << _T(" 0.04(") << Sub2(_T("A"),_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") << symbol(DELTA) << RPT_STRESS(_T("pT")) << _T(") = ");
               }

               (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps(endType, pgsTypes::Permanent)) << _T(")(") << stress.SetValue(pArtifact->GetFpj(endType, pgsTypes::Permanent)) << _T(" - ");
               (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer(endType, pgsTypes::Permanent)) << _T(" ) = ") << force.SetValue(pArtifact->GetTotalSplittingForce(endType)) << rptNewLine;
            }

            (*pPara) << strName << _T(" Resistance: P") << Sub(_T("r")) << _T(" = ") << RPT_STRESS(_T("s")) << Sub2(_T("A"), _T("s"));
            if (bUHPC)
            {
               (*pPara) << _T(" + (f1 / 2)(h /") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(")") << Sub2(_T("b"), _T("v"));
            }
            (*pPara) << _T(" = ");
            (*pPara) << _T("(") << stress.SetValue(pArtifact->GetFs(endType)) << _T(")(") << area.SetValue(pArtifact->GetAvs(endType)) << _T(")");
            if (bUHPC)
            {
               (*pPara) << _T(" + (") << stress.SetValue(pArtifact->GetUHPCStrengthAtFirstCrack()) << _T(" /2)(") << short_length.SetValue(pArtifact->GetH(endType)) << _T("/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(")");
               (*pPara) << _T("(") << short_length.SetValue(pArtifact->GetShearWidth(endType)) << _T(")");
            }
            (*pPara) << _T(" = ") << force.SetValue(pArtifact->GetSplittingResistance(endType)) << rptNewLine << rptNewLine;
         }
      }
      else
      {
         (*pPara) << _T("Check for ") << strName << _T(" resistance (LRFD ") << LrfdCw8th(_T("5.10.10.1"), _T("5.9.4.4.1")) << _T(") is disabled in Project Criteria library.") << rptNewLine;
      }
   } // next segment

   return pChapter;
}

CChapterBuilder* CSplittingZoneDetailsChapterBuilder::Clone() const
{
   return new CSplittingZoneDetailsChapterBuilder;
}
