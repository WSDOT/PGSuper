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
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\ReportNotes.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>


#include <psgLib/LiveLoadLibraryEntry.h>
#include <psgLib/LiveLoadCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void ReportPedestrian(ILiveLoads::PedestrianLoadApplicationType pedType, rptParagraph* pPara)
{
   if (pedType==ILiveLoads::PedConcurrentWithVehicular)
   {
      (*pPara) << _T("Pedestrian live load was applied concurrently with vehicular live loads.")<<rptNewLine;
   }
   else if (pedType==ILiveLoads::PedEnvelopeWithVehicular)
   {
      (*pPara) << _T("Pedestrian live load response was enveloped with vehicular live loads.")<<rptNewLine;
   }
   else if (pedType==ILiveLoads::PedDontApply)
   {
      (*pPara) << _T("Pedestrian live load was not applied.")<<rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }
}

/****************************************************************************
CLASS
   CLiveLoadDetailsChapterBuilder
****************************************************************************/

CLiveLoadDetailsChapterBuilder::CLiveLoadDetailsChapterBuilder(bool bDesign,bool bRating,bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

LPCTSTR CLiveLoadDetailsChapterBuilder::GetName() const
{
   return TEXT("Live Load Details");
}

rptChapter* CLiveLoadDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pBrokerRptSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pBrokerRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);

   bool bDesign = m_bDesign;
   bool bRating = m_bRating;

   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   bool bPedestrian = pProductLoads->HasPedestrianLoad();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;

   std::vector<std::_tstring> user_loads;
   std::vector<std::_tstring>::iterator it;

   if ( bDesign )
   {
      *pPara<< _T("Live Loads used for Design")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);

      if (user_loads.empty())
      {
         *pPara<<_T("No live loads were applied to the design (Service and Strength I) limit states")<< rptNewLine;
      }
      else
      {
         *pPara<<_T("The following live loads were applied to the design (Service and Strength I) limit states:")<< rptNewLine;
      }

      for (it=user_loads.begin(); it!=user_loads.end(); it++)
      {
         std::_tstring& load_name = *it;

         pPara = new rptParagraph;
         *pChapter << pPara;

         ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
      }

      if(bPedestrian)
      {
         ReportPedestrian(pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltDesign), pPara);
      }

      // Fatigue live loads
      if ( WBFL::LRFD::LRFDVersionMgr::Version::FourthEditionWith2009Interims <= WBFL::LRFD::LRFDVersionMgr::GetVersion() )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Live Loads Used for Fatigue Limit States")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltFatigue);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were applied to the Fatigue I limit state")<< rptNewLine;
         }
         else
         {
            *pPara<<_T("The following live loads were applied to the Fatigue I limit state:")<< rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }
      }

      if(bPedestrian)
      {
         ReportPedestrian(pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltFatigue), pPara);
      }

      // Strength II live loads
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      *pChapter << pPara;
      *pPara<< _T("Live Loads Used for Design Permit Limit State")<<rptNewLine;

      pPara = new rptParagraph;
      *pChapter << pPara;

      user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermit);

      if (user_loads.empty())
      {
         *pPara<<_T("No live loads were applied to the design permit (Strength II) limit state")<< rptNewLine;
      }
      else
      {
         *pPara<<_T("The following live loads were applied to the design permit (Strength II) limit state:")<< rptNewLine;
      }

      for (it=user_loads.begin(); it!=user_loads.end(); it++)
      {
         std::_tstring& load_name = *it;

         pPara = new rptParagraph;
         *pChapter << pPara;

         ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
      }

      if(bPedestrian)
      {
         ReportPedestrian(pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit), pPara);
      }
   }


   if ( bRating )
   {
      GET_IFACE2(pBroker, IRatingSpecification, pRatingSpec);
      bool rate_pedestrian = pRatingSpec->IncludePedestrianLiveLoad();

      if ( !bDesign && (pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) || pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating)) )
      {
         *pPara<< _T("Live Loads used for Design Load Rating")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltDesign);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Live Loads used for Legal Load Rating for Routine Commercial Traffic")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }


      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Live Loads used for Legal Load Rating for Special Hauling Vehicles")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }

      if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara << _T("Live Loads used for Legal Load Rating for Emergency Vehicles") << rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Emergency);

         if (user_loads.empty())
         {
            *pPara << _T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it = user_loads.begin(); it != user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Live Loads used for Routine Permit Load Rating")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }

      if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      {
         pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
         *pChapter << pPara;
         *pPara<< _T("Live Loads used for Special Permit Load Rating")<<rptNewLine;

         pPara = new rptParagraph;
         *pChapter << pPara;

         user_loads = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

         if (user_loads.empty())
         {
            *pPara<<_T("No live loads were defined for this load rating") << rptNewLine;
         }

         for (it=user_loads.begin(); it!=user_loads.end(); it++)
         {
            std::_tstring& load_name = *it;

            pPara = new rptParagraph;
            *pChapter << pPara;

            ReportLiveLoad(pBroker, load_name, pPara, pDisplayUnits);
         }

         if (bPedestrian)
         {
            ReportPedestrian(rate_pedestrian ? ILiveLoads::PedConcurrentWithVehicular : ILiveLoads::PedDontApply, pPara);
         }
      }
   }


   return pChapter;
}

void CLiveLoadDetailsChapterBuilder::ReportLiveLoad(IBroker* pBroker, std::_tstring& load_name, rptParagraph* pPara,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         dim,         pDisplayUnits->GetSpanLengthUnit(),       false );
   INIT_UV_PROTOTYPE( rptForceUnitValue,          force,       pDisplayUnits->GetGeneralForceUnit(),     false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         span_length, pDisplayUnits->GetSpanLengthUnit(),       true );
   INIT_UV_PROTOTYPE( rptForcePerLengthUnitValue, fpl,         pDisplayUnits->GetForcePerLengthUnit(),   true );
   INIT_UV_PROTOTYPE( rptPressureUnitValue,       pressure,    pDisplayUnits->GetSidewalkPressureUnit(), true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,         sw,          pDisplayUnits->GetSpanLengthUnit(),       true );

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const LiveLoadLibraryEntry* ll_entry = pLibrary->GetLiveLoadEntry( load_name.c_str());

   // if the entry is nullptr, the name better be HL-93
   if ( ll_entry == nullptr )
   {
      if ( load_name == _T("HL-93") )
      {
         *pPara << Bold(_T("AASHTO LRFD 3.6.1.2: ")) << _T("HL-93 Design Vehicular Live Load") << rptNewLine;
      }
      else if ( load_name == _T("Fatigue") )
      {
         *pPara << Bold(_T("AASHTO LRFD 3.6.1.4: ")) << _T("Fatigue Vehicular Live Load") << rptNewLine;
      }
      else if ( load_name == _T("Pedestrian on Sidewalk") )
      {
         *pPara << Bold(_T("AASHTO LRFD 3.6.1.6: ")) << _T("Pedestrian Live Load") << rptNewLine;

         GET_IFACE2(pBroker,ISpecification,pSpec);
         const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
         const auto& live_load_criteria = pSpecEntry->GetLiveLoadCriteria();
         *pPara << _T("A pedestrian load of ") << pressure.SetValue(live_load_criteria.PedestrianLoad) << _T(" is applied to sidewalks wider than ") << sw.SetValue(live_load_criteria.MinSidewalkWidth) << _T(" and considered simultaneously with the vehicular design live load.") << rptNewLine;
      }
      else if ( load_name == _T("AASHTO Legal Loads") )
      {
         *pPara << Bold(_T("AASHTO MBE 6A.4.4.2.1a: ")) << _T("Routine Commercial Traffic") << rptNewLine;
      }
      else if ( load_name == _T("Notional Rating Load (NRL)") )
      {
         *pPara << Bold(_T("AASHTO MBE 6A.4.4.2.1b: ")) << _T("Specialized Hauling Vehicles (NRL)") << rptNewLine;
      }
      else if ( load_name == _T("Single-Unit SHVs") )
      {
         *pPara << Bold(_T("AASHTO MBE 6A.4.4.2.1b: ")) << _T("Specialized Hauling Vehicles (SU)") << rptNewLine;
      }
      else if (load_name == _T("Emergency Vehicles"))
      {
         *pPara << Bold(_T("FHWA Memo, November 3, 2016: ")) << _T("FAST Act Emergency Vehicles") << rptNewLine;
      }
      else
         ATLASSERT(false);

      return;
   }


   *pPara<< Bold(_T("User-defined vehicular live load: ")) << load_name << rptNewLine;

   LiveLoadLibraryEntry::LiveLoadConfigurationType ll_config = ll_entry->GetLiveLoadConfigurationType();
   if (ll_config == LiveLoadLibraryEntry::lcTruckOnly)
   {
      *pPara<<_T("Configuration: Truck applied only (no lane)")<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcLaneOnly)
   {
      *pPara<<_T("Configuration: Lane load applied only (no truck)")<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckPlusLane)
   {
      *pPara<<_T("Configuration: Truck response added with lane response")<<rptNewLine;
   }
   else if (ll_config== LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      *pPara<<_T("Configuration: Truck response enveloped with lane response")<<rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   pgsTypes::LiveLoadApplicabilityType ll_applicability = ll_entry->GetLiveLoadApplicabilityType();
   if ( ll_applicability == pgsTypes::llaEntireStructure )
   {
      *pPara << _T("Usage: Use for all actions at all locations") << rptNewLine;
   }
   else if ( ll_applicability == pgsTypes::llaNegMomentAndInteriorPierReaction  )
   {
      *pPara << _T("Usage: Use only for negative moments and interior pier reactions") << rptNewLine;
   }
   else if ( ll_applicability == pgsTypes::llaContraflexure  )
   {
      *pPara << _T("Usage: Use only for negative moments between points of contraflexure and interior pier reactions") << rptNewLine;
   }
   else
   {
      ATLASSERT(false);
   }

   // lane load if present
   if (ll_config == LiveLoadLibraryEntry::lcLaneOnly      || 
       ll_config == LiveLoadLibraryEntry::lcTruckPlusLane ||
       ll_config == LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      *pPara<<_T("Lane Load Value: ")<< fpl.SetValue(ll_entry->GetLaneLoad()) << rptNewLine;
      *pPara << _T("Lane Load is applied for span length greater than ") << span_length.SetValue(ll_entry->GetLaneLoadSpanLength()) << rptNewLine; 
   }

   // truck axles
   if (ll_config == LiveLoadLibraryEntry::lcTruckOnly     || 
       ll_config == LiveLoadLibraryEntry::lcTruckPlusLane ||
       ll_config == LiveLoadLibraryEntry::lcTruckLaneEnvelope)
   {
      rptRcTable* p_table = rptStyleManager::CreateDefaultTable(3,_T(""));
      *pPara << p_table;

      (*p_table)(0,0) << _T("Axle");
      (*p_table)(0,1) << COLHDR(_T("Weight"),  rptForceUnitTag,  pDisplayUnits->GetGeneralForceUnit() );
      (*p_table)(0,2) << COLHDR(_T("Spacing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

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
               (*p_table)(row,2) <<_T(" - ")<<dim.SetValue(ll_entry->GetMaxVariableAxleSpacing());
         }
         else
         {
            (*p_table)(row,2) << _T("");
         }
      }

      if (0 < nAxles)
      {
         if (ll_entry->GetIsNotional())
         {
            *pPara<<_T("    Note: Only those axles contributing to extreme force under consideration are applied. (LRFD 3.6.1.3.1)")<<rptNewLine;
         }
      }
   }
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CLiveLoadDetailsChapterBuilder::Clone() const
{
   return std::make_unique<CLiveLoadDetailsChapterBuilder>(m_bDesign,m_bRating);
}
