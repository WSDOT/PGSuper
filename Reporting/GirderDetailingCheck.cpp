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
#include <Reporting\GirderDetailingCheck.h>
#include <Reporting\StirrupDetailingCheckTable.h>

#include <IFace\Artifact.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\PrecastIGirderDetailingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderDetailingCheck
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderDetailingCheck::CGirderDetailingCheck():
m_BasicVersion(false)
{
}

CGirderDetailingCheck::CGirderDetailingCheck(bool basic):
m_BasicVersion(basic)
{
}

CGirderDetailingCheck::CGirderDetailingCheck(const CGirderDetailingCheck& rOther)
{
   MakeCopy(rOther);
}

CGirderDetailingCheck::~CGirderDetailingCheck()
{
}

//======================== OPERATORS  =======================================
CGirderDetailingCheck& CGirderDetailingCheck::operator= (const CGirderDetailingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CGirderDetailingCheck::Build(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
#pragma Reminder("UPDATE: Use chapter levels instead of m_BasicVersion")
   if (!m_BasicVersion)
   {
      // girder dimensions check table
      CGirderDetailingCheck::BuildDimensionCheck(pChapter, pBroker, pGirderArtifact, pDisplayUnits);
   }

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   // Stirrup detailing check
   rptParagraph* p = new rptParagraph;
   bool write_note;
   *p << CStirrupDetailingCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthI,&write_note) << rptNewLine;
   *pChapter << p;

   if (write_note)
   {
      *p << _T("* - Transverse reinforcement not required if ") << Sub2(_T("V"),_T("u")) << _T(" < 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"),_T("c"));
      *p  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(") [Eqn 5.8.2.4-1]")<< rptNewLine;
   }

   GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
   if(pLimitStateForces->IsStrengthIIApplicable(girderKey))
   {
	   rptParagraph* p = new rptParagraph;
	   bool write_note;
	   *p << CStirrupDetailingCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,intervalIdx,pgsTypes::StrengthII,&write_note) << rptNewLine;
	   *pChapter << p;
	
	   if (write_note)
	   {
	      *p << _T("* - Transverse reinforcement not required if ") << Sub2(_T("V"),_T("u")) << _T(" < 0.5") << symbol(phi) << _T("(") << Sub2(_T("V"),_T("c"));
	      *p  << _T(" + ") << Sub2(_T("V"),_T("p")) << _T(") [Eqn 5.8.2.4-1]")<< rptNewLine;
	   }
   }

   // Stirrup Layout Check
   if ( !m_BasicVersion )
   {
      // Only report stirrup length/zone incompatibility if user requests it
      GET_IFACE2(pBroker,ISpecification,pSpec);
      GET_IFACE2(pBroker,ILibrary,pLib);
      std::_tstring strSpecName = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

      if ( pSpecEntry->GetDoCheckStirrupSpacingCompatibility() )
      {
         // Stirrup Layout Check
         BuildStirrupLayoutCheck(pChapter, pBroker, pGirderArtifact, pDisplayUnits);
      }
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderDetailingCheck::MakeCopy(const CGirderDetailingCheck& rOther)
{
   // Add copy code here...
}

void CGirderDetailingCheck::MakeAssignment(const CGirderDetailingCheck& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CGirderDetailingCheck::BuildDimensionCheck(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Girder Dimensions Detailing Check [5.14.1.2.2]") << rptNewLine;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsPrecastIGirderDetailingArtifact* pArtifact = pSegmentArtifact->GetPrecastIGirderDetailingArtifact();

      if ( 1 < nSegments )
      {
         rptParagraph* pSubHeading = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pSubHeading;
         *pSubHeading << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }
      
      rptParagraph* pBody = new rptParagraph;
      *pChapter << pBody;

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(4);
      *pBody << pTable;

      pTable->SetColumnStyle(0, pgsReportStyleHolder::GetTableCellStyle( CB_NONE | CJ_LEFT) );
      pTable->SetStripeRowColumnStyle(0, pgsReportStyleHolder::GetTableStripeRowCellStyle( CB_NONE | CJ_LEFT) );

      (*pTable)(0,0)  << _T("Dimension");
      (*pTable)(0,1)  << COLHDR(_T("Minimum"),  rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,2)  << COLHDR(_T("Actual"),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*pTable)(0,3)  << _T("Status");

      (*pTable)(1,0) << _T("Top Flange Thickness");
      (*pTable)(2,0) << _T("Web Thickness");
      (*pTable)(3,0) << _T("Bottom Flange Thickness");

      INIT_UV_PROTOTYPE( rptLengthSectionValue,      dim,      pDisplayUnits->GetComponentDimUnit(),  false );

      if ( IsZero(pArtifact->GetProvidedTopFlangeThickness()) )
      {
         // There is no top flange... Assume this is not a bulb-T or I type section
         (*pTable)(1,1) << _T("-");
         (*pTable)(1,2) << _T("-");
         (*pTable)(1,3) << RPT_NA << rptNewLine << _T("See LRFD C5.14.1.2.2");
      }
      else
      {
         (*pTable)(1,1) << dim.SetValue(pArtifact->GetMinTopFlangeThickness());
         (*pTable)(1,2) << dim.SetValue(pArtifact->GetProvidedTopFlangeThickness());
         if ( pArtifact->GetProvidedTopFlangeThickness() < pArtifact->GetMinTopFlangeThickness() )
         {
            (*pTable)(1,3) << RPT_FAIL;
         }
         else
         {
            (*pTable)(1,3) << RPT_PASS;
         }
      }

      if ( IsZero(pArtifact->GetProvidedWebThickness()) )
      {
         // There is no web... voided slab type girder
         (*pTable)(2,1) << _T("-");
         (*pTable)(2,2) << _T("-");
         (*pTable)(2,3) << RPT_NA;
      }
      else
      {
         (*pTable)(2,1) << dim.SetValue(pArtifact->GetMinWebThickness());
         (*pTable)(2,2) << dim.SetValue(pArtifact->GetProvidedWebThickness());
         if ( pArtifact->GetProvidedWebThickness() < pArtifact->GetMinWebThickness() )
         {
            (*pTable)(2,3) << RPT_FAIL;
         }
         else
         {
            (*pTable)(2,3) << RPT_PASS;
         }
      }

      if ( IsZero(pArtifact->GetProvidedBottomFlangeThickness()) )
      {
         // There is no bottom flange... Assume this is stemmed girder
         (*pTable)(3,1) << _T("-");
         (*pTable)(3,2) << _T("-");
         (*pTable)(3,3) << RPT_NA;
      }
      else
      {
         (*pTable)(3,1) << dim.SetValue(pArtifact->GetMinBottomFlangeThickness());
         (*pTable)(3,2) << dim.SetValue(pArtifact->GetProvidedBottomFlangeThickness());
         if ( pArtifact->GetProvidedBottomFlangeThickness() < pArtifact->GetMinBottomFlangeThickness() )
         {
            (*pTable)(3,3) << RPT_FAIL;
         }
         else
         {
            (*pTable)(3,3) << RPT_PASS;
         }
      }
   }
}


void CGirderDetailingCheck::BuildStirrupLayoutCheck(rptChapter* pChapter,
                              IBroker* pBroker,const pgsGirderArtifact* pGirderArtifact,
                              IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeometry);

#pragma Reminder("UPDATE: need to report stirrup layout check for closure joints")

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara << _T("Stirrup Layout Geometry Check") << rptNewLine;

   INIT_FRACTIONAL_LENGTH_PROTOTYPE( gdim,  IS_US_UNITS(pDisplayUnits), 8, pDisplayUnits->GetComponentDimUnit(), true, true );
   rptRcScalar scalar;

   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(pGirderArtifact->GetGirderKey());
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(pGirderArtifact->GetGirderKey(),segIdx);

      if ( 1 < nSegments )
      {
         rptParagraph* pSubHeading = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         *pChapter << pSubHeading;
         *pSubHeading << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3);
      *pPara << p_table;

      (*p_table)(0,0) << _T("Zone");
      (*p_table)(0,1) << _T("Stirrup Layout");
      (*p_table)(0,2) << _T("Status");


      GET_IFACE2(pBroker,IShear,pShear);
      const CShearData2* pShearData = pShear->GetSegmentShearData(segmentKey);
   
      ZoneIndexType squishyZoneIdx = INVALID_INDEX;
      if ( pShearData->bAreZonesSymmetrical )
      {
         // if zones are symmetrical, the last zone input is the "squishy" zone
         squishyZoneIdx = pShearData->ShearZones.size()-1;
      }

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      ZoneIndexType nZones = pStirrupGeometry->GetPrimaryZoneCount(segmentKey);
      for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++, row++)
      {
         (*p_table)(row,0) << LABEL_STIRRUP_ZONE(zoneIdx);

         Float64 zoneStart, zoneEnd;
         pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, zoneIdx, &zoneStart, &zoneEnd);
         Float64 zoneLength = zoneEnd-zoneStart;

         matRebar::Size barSize;
         Float64 spacing;
         Float64 nStirrups;
         pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey,zoneIdx,&barSize,&nStirrups,&spacing);

         if (barSize != matRebar::bsNone && TOLERANCE < spacing)
         {
            // If spacings fit within 1%, then pass. Otherwise fail
            Float64 nFSpaces = zoneLength / spacing;
            Int32 nSpaces = (Int32)nFSpaces;
            Float64 rmdr = nFSpaces - nSpaces;
   
   
            if ( zoneIdx == squishyZoneIdx )
            {
               nSpaces++; // round up one (the value was truncated above)
               (*p_table)(row,1) << nSpaces <<_T(" Spa. @ ")<<gdim.SetValue(spacing)<<_T(" (Max) = ")<<gdim.SetValue(zoneLength);
               (*p_table)(row,2) << _T("OK");
            }
            else
            {
               bool pass = IsZero(rmdr, 0.01) || IsEqual(rmdr, 1.0, 0.01);
   
               (*p_table)(row,1) <<scalar.SetValue(nFSpaces)<<_T(" Spa. @ ")<<gdim.SetValue(spacing)<<_T(" = ")<<gdim.SetValue(zoneLength);
               if (pass)
               {
                  (*p_table)(row,2) << _T("OK");
               }
               else
               {
                  (*p_table)(row,2) << color(Red) << _T("Zone length is not compatible with stirrup spacing") << color(Black);
               }
            }
         }
         else
         {
            (*p_table)(row,1) << _T("(None)");
            (*p_table)(row,2) << RPT_NA;
         }
      }
   }
}


//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CGirderDetailingCheck::AssertValid() const
{
   return true;
}

void CGirderDetailingCheck::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CGirderDetailingCheck") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CGirderDetailingCheck::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CGirderDetailingCheck");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CGirderDetailingCheck");

   TESTME_EPILOG("LiveLoadDistributionFactorTable");
}
#endif // _UNITTEST
