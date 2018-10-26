///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <IFace\DisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>


#include <PsgLib\LiveLoadLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiveLoadDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CLiveLoadDetailsChapterBuilder::CLiveLoadDetailsChapterBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CLiveLoadDetailsChapterBuilder::GetName() const
{
   return TEXT("Live Load Details");
}

rptChapter* CLiveLoadDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

   rptParagraph* pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Live Loads Used for Design Limit States"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   std::vector<std::string> user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);

   if (user_loads.empty())
   {
      *pPara<<"No live loads were applied to the design (Service and Strength I) limit states"<< rptNewLine;
   }
   else
   {
      *pPara<<"The following live loads were applied to the design (Service and Strength I) limit states:"<< rptNewLine;
   }

   std::vector<std::string>::iterator it;
   for (it=user_loads.begin(); it!=user_loads.end(); it++)
   {
      std::string& load_name = *it;

      pPara = new rptParagraph;
      *pChapter << pPara;

      ReportLiveLoad(pBroker, load_name, pPara, pDispUnits);
   }

   // Fatigue live loads
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< "Live Loads Used for Fatigue Limit States"<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);

      if (user_loads.empty())
      {
         *pPara<<"No live loads were applied to the Fatigue I limit state"<< rptNewLine;
      }
      else
      {
         *pPara<<"The following live loads were applied to the Fatigue I limit state:"<< rptNewLine;
      }

      for (it=user_loads.begin(); it!=user_loads.end(); it++)
      {
         std::string& load_name = *it;

         pPara = new rptParagraph;
         *pChapter << pPara;

         ReportLiveLoad(pBroker, load_name, pPara, pDispUnits);
      }
   }

   // Strength II live loads
   pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pPara;
   *pPara<< "Live Loads Used for Permit Limit States"<<rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;

   user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);

   if (user_loads.empty())
   {
      *pPara<<"No live loads were applied to the permit (Strength II) limit states"<< rptNewLine;
   }
   else
   {
      *pPara<<"The following live loads were applied to the permit (Strength II) limit states:"<< rptNewLine;
   }

   for (it=user_loads.begin(); it!=user_loads.end(); it++)
   {
      std::string& load_name = *it;

      pPara = new rptParagraph;
      *pChapter << pPara;

      ReportLiveLoad(pBroker, load_name, pPara, pDispUnits);
   }


   return pChapter;
}

void CLiveLoadDetailsChapterBuilder::ReportLiveLoad(IBroker* pBroker, std::string& load_name, rptParagraph* pPara,IDisplayUnits* pDispUnits)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,     pDispUnits->GetSpanLengthUnit(),  false );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl, pDispUnits->GetForcePerLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,    pDispUnits->GetGeneralForceUnit(), false );
   INIT_UV_PROTOTYPE( rptPressureUnitValue,       pressure, pDispUnits->GetSidewalkPressureUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         sw,     pDispUnits->GetSpanLengthUnit(),  true );

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const LiveLoadLibraryEntry* ll_entry = pLibrary->GetLiveLoadEntry( load_name.c_str());

   // if the entry is NULL, the name better be HL-93
   if ( ll_entry == NULL )
   {
      if ( load_name == "HL-93" )
      {
         *pPara << Bold("AASHTO LRFD 3.6.1.2: ") << "HL-93 Design Vehicular Live Load" << rptNewLine;
      }
      else if ( load_name == "Fatigue" )
      {
         *pPara << Bold("AASHTO LRFD 3.6.1.4: ") << "Fatigue Vehicular Live Load" << rptNewLine;
      }
      else if ( load_name == "Pedestrian on Sidewalk" )
      {
         *pPara << Bold("AASHTO LRFD 3.6.1.6: ") << "Pedestrian Live Load" << rptNewLine;

         GET_IFACE2(pBroker,ISpecification,pSpec);
         const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
         *pPara << "A pedestrian load of " << pressure.SetValue(pSpecEntry->GetPedestrianLiveLoad()) << " is applied to sidewalks wider than " << sw.SetValue(pSpecEntry->GetMinSidewalkWidth()) << " and considered simultaneously with the vehicular design live load." << rptNewLine;
      }
      else
         ATLASSERT(false);

      return;
   }


   *pPara<< Bold("User-defined vehicular live load: ") << load_name << rptNewLine;

   LiveLoadLibraryEntry::LiveLoadConfigurationType ll_config = ll_entry->GetLiveLoadConfigurationType();
   if (ll_config == LiveLoadLibraryEntry::lcTruckOnly)
   {
      *pPara<<"Configuration: Truck applied only (no lane)"<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcLaneOnly)
   {
      *pPara<<"Configuration: Lane load applied only (no truck)"<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckPlusLane)
   {
      *pPara<<"Configuration: Truck response added with lane response"<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      *pPara<<"Configuration: Truck response enveloped with lane response"<<rptNewLine;
   }
   else
   {
      ATLASSERT(0);
   }

   // lane load if present
   if (ll_config== LiveLoadLibraryEntry::lcLaneOnly || 
       ll_config== LiveLoadLibraryEntry::lcTruckPlusLane ||
       ll_config== LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      *pPara<<"Lane Load Value: "<<fpl.SetValue(ll_entry->GetLaneLoad())<<fpl.GetUnitTag()<<rptNewLine;
   }

   // truck axles
   if (ll_config== LiveLoadLibraryEntry::lcTruckOnly || 
       ll_config== LiveLoadLibraryEntry::lcTruckPlusLane ||
       ll_config== LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,"");
      *pPara << p_table;

      (*p_table)(0,0) << "Axle";
      (*p_table)(0,1) << COLHDR("Weight",       rptForceUnitTag, pDispUnits->GetGeneralForceUnit() );
      (*p_table)(0,2) << COLHDR("Spacing",      rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );

      AxleIndexType var_axl = ll_entry->GetVariableAxleIndex();

      AxleIndexType nAxles = ll_entry->GetNumAxles();
      for (AxleIndexType axleIdx = 0; axleIdx < nAxles; axleIdx++)
      {
         RowIndexType row = (RowIndexType)(axleIdx+1);
         LiveLoadLibraryEntry::Axle axle = ll_entry->GetAxle(axleIdx);

         (*p_table)(row,0) << row;
         (*p_table)(row,1) << force.SetValue(axle.Weight);

         if (0 < axleIdx)
         {
            (*p_table)(row,2) << dim.SetValue(axle.Spacing);

            if (axleIdx == var_axl)
               (*p_table)(row,2) <<"-"<<dim.SetValue(ll_entry->GetMaxVariableAxleSpacing());
         }
         else
         {
            (*p_table)(row,2) << " ";
         }
      }

      if (0 < nAxles)
      {
         if (ll_entry->GetIsNotional())
         {
            *pPara<<"    Note: Only those axles contributing to extreme force under consideration are applied. (LRFD 3.6.1.3.1)"<<rptNewLine;
         }
      }
   }
}

CChapterBuilder* CLiveLoadDetailsChapterBuilder::Clone() const
{
   return new CLiveLoadDetailsChapterBuilder;
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
