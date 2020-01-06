///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include "stdafx.h"
#include <Beams\Helper.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <PgsExt\BridgeDescription2.h>
#include "AgeAdjustedMaterial.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void ReportLeverRule(rptParagraph* pPara,bool isMoment, Float64 specialFactor, lrfdILiveLoadDistributionFactor::LeverRuleMethod& lrd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   if (lrd.Nb>1)
   {
      INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,    pDisplayUnits->GetSpanLengthUnit(),    true );
      INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

      std::vector<Float64>::iterator iter;
      std::_tstring strImageName(lrd.bWasExterior ? _T("LeverRuleExterior.gif") : _T("LeverRuleInterior.gif"));
      (*pPara) << rptRcImage(std::_tstring(std::_tstring(rptStyleManager::GetImagePath())) + strImageName) << rptNewLine;
      (*pPara) << _T("Multiple Presence Factor: m = ") << lrd.m << rptNewLine;
      if (isMoment)
      {
         (*pPara) << _T("mg") << Super((lrd.bWasExterior ? _T("ME") : _T("MI"))) << Sub((lrd.nLanesUsed > 1 ? _T("2+") : _T("1"))) << _T(" = (");
      }
      else
      {
         (*pPara) << _T("mg") << Super((lrd.bWasExterior ? _T("VE") : _T("VI"))) << Sub((lrd.nLanesUsed > 1 ? _T("2+") : _T("1"))) << _T(" = (");
      }

      if (specialFactor != 1.0)
      {
         (*pPara) << lrd.m << _T(")(")<<specialFactor<<_T(")[");
      }
      else
      {
         (*pPara) << lrd.m << _T(")[");
      }

      Float64 Sleft = lrd.Sleft;
      Float64 Sright = lrd.Sright;

      if (IsEqual(Sleft,Sright))
      {
         // equal spacing, take absolute values
         for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
         {
            Float64 d = fabs(*iter);

            if ( iter != lrd.AxleLocations.begin() )
               (*pPara) << _T(" + ");

            (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
         }
         (*pPara) << _T("]/[(") << xdim.SetValue(Sleft) << _T(")(P)]");
      }
      else
      {
         // Unequal spacing, see if we have left and/or right
         bool is_left=false;
         bool is_right=false;
         for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
         {
            Float64 d = (*iter);
            if (d < 0.0)
            {
               is_left=true;
            }
            else
            {
               is_right=true;
            }
         }

         if (is_left && is_right)
            (*pPara) << _T(" [");

         // do left, then right
         if (is_left)
         {
            bool first=true;
            for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
            {
               Float64 d = (*iter);
               if (d < 0.0)
               {
                  if ( !first )
                     (*pPara) << _T(" + ");

                  d*=-1;

                  (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
                  first = false;
               }
            }
            (*pPara) << _T("]/[(") << xdim.SetValue(Sleft) << _T(")(P)]");
         }

         // right
         if (is_left && is_right)
            (*pPara) << _T(" + [");

         if (is_right)
         {
            bool first=true;
            for ( iter = lrd.AxleLocations.begin(); iter != lrd.AxleLocations.end(); iter++ )
            {
               Float64 d = (*iter);
               if (d > 0.0)
               {
                  if ( !first )
                     (*pPara) << _T(" + ");


                  (*pPara) << _T("(") << xdim.SetValue(d) << _T(")") << _T("(P/2)");
                  first = false;
               }
            }
            (*pPara) << _T("]/[(") << xdim.SetValue(Sright) << _T(")(P)]");
         }

         if (is_left && is_right)
            (*pPara) << _T(" ]");
      }

      (*pPara) << _T(" = ") << scalar.SetValue(lrd.mg) << rptNewLine;
   }
   else
   {
      ATLASSERT(lrd.Nb==1);
      (*pPara) << _T("For a single-beam superstructure, the lever rule decomposes to the Lanes/Beams method") << rptNewLine;
      lrfdILiveLoadDistributionFactor::LanesBeamsMethod lbm;
      lbm.mg = lrd.mg;
      lbm.Nl = lrd.nLanesUsed;
      lbm.Nb = 1;
      lbm.m  = lrd.m;

      ReportLanesBeamsMethod(pPara,lbm,pBroker,pDisplayUnits);
   }
}

void ReportRigidMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::RigidMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,    pDisplayUnits->GetSpanLengthUnit(),    true );

   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << rptRcImage(std::_tstring(std::_tstring(rptStyleManager::GetImagePath())) + _T("RigidMethod.gif")) << rptNewLine;
   (*pPara) << _T("Multiple Presence Factor: m = ") << rd.m << rptNewLine;
   (*pPara) << _T("g = ") << _T("(") << rd.Nl << _T("/") << rd.Nb << _T(") + (") << xdim.SetValue(rd.Xext) << _T(")[");
   std::vector<Float64>::iterator iter;
   for ( iter = rd.e.begin(); iter != rd.e.end(); iter++ )
   {
      Float64 e = *iter;
      if ( iter != rd.e.begin() )
         (*pPara) << _T(" + ");

      (*pPara) << _T("(") << xdim.SetValue(e) << _T(")");

   }
   (*pPara) << _T("]/[");
   for ( iter = rd.x.begin(); iter != rd.x.end(); iter++ )
   {
      Float64 x = *iter;
      if ( iter != rd.x.begin() )
         (*pPara) << _T(" + ");

      (*pPara) << _T("(") << xdim.SetValue(x) << _T(")") << Super(_T("2"));
   }
   (*pPara) << _T("] = ") << scalar.SetValue(rd.mg/rd.m) << rptNewLine;
   (*pPara) << _T("mg") << Super(_T("ME")) << Sub((rd.e.size() > 1 ? _T("2+") : _T("1"))) << _T(" = ") << scalar.SetValue(rd.mg) << rptNewLine;
}

void ReportLanesBeamsMethod(rptParagraph* pPara,lrfdILiveLoadDistributionFactor::LanesBeamsMethod& rd,IBroker* pBroker,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   (*pPara) << _T("Multiple Presence Factor: m = ") << rd.m << rptNewLine;
   (*pPara) << _T("g = ") << _T("(") << rd.m <<_T(")(")<< rd.Nl << _T("/") << rd.Nb << _T(") = ") << scalar.SetValue(rd.mg) << rptNewLine;
}


void BuildAgeAdjustedGirderMaterialModel(IBroker* pBroker,const CPrecastSegmentData* pSegment,ISuperstructureMemberSegment* segment,IAgeAdjustedMaterial** ppMaterial)
{
   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   std::array<bool, 2> bHasClosure = {
                           pSegment->GetClosureJoint(pgsTypes::metStart)  != nullptr, 
                           pSegment->GetClosureJoint(pgsTypes::metEnd) != nullptr
                         };

   // If this is the first segment of a girder and there is a previous group -OR-
   // if this is the last segment of a girder and there is next group, 
   // then there is a cast-in-place diaphragm between the groups. Use the deck concrete as the closure joint
   // concrete in this case.
   std::array<bool, 2> bPierDiaphragm = { 
                              (pSegment->GetPrevSegment() == nullptr && pGirder->GetGirderGroup()->GetPrevGirderGroup() != nullptr),
                              (pSegment->GetNextSegment() == nullptr && pGirder->GetGirderGroup()->GetNextGirderGroup() != nullptr)   
                            };


   // NOTE: TRICKY CODE FOR SPLICED GIRDER BEAMS
   // We need to model the material with an age adjusted modulus. The age adjusted modulus
   // is E/(1+XY) where XY is the age adjusted creep coefficient. In order to get the creep coefficient
   // we need the Volume and Surface Area of the segment (for the V/S ratio). This method
   // gets called during the creation of the bridge model. The bridge model is not
   // ready to compute V or S. Calling IMaterial::GetSegmentAgeAdjustedEc() will cause
   // recusion and validation errors. Using the age adjusted material object we can
   // delay the calls to GetAgeAdjustedEc until well after the time the bridge model
   // is validated.
   GET_IFACE2(pBroker,IMaterials,pMaterials);

   CComObject<CAgeAdjustedMaterial>* pSegmentMaterial;
   CComObject<CAgeAdjustedMaterial>::CreateInstance(&pSegmentMaterial);
   CComPtr<IAgeAdjustedMaterial> segmentMaterial = pSegmentMaterial;
   segmentMaterial->InitSegment(segmentKey,pMaterials);

   segmentMaterial.CopyTo(ppMaterial);

   CComQIPtr<ISplicedGirderSegment> pSplicedSegment(segment);
   if ( pSplicedSegment )
   {
      for ( int i = 0; i < 2; i++ )
      {
         EndType endType = (EndType)i;
         if ( bHasClosure[endType] )
         {
            CClosureKey closureKey;
            if ( endType == etStart )
            {
               closureKey = CClosureKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex-1);
            }
            else
            {
               closureKey = segmentKey;
            }

            CComObject<CAgeAdjustedMaterial>* pClosureMaterial;
            CComObject<CAgeAdjustedMaterial>::CreateInstance(&pClosureMaterial);
            CComPtr<IAgeAdjustedMaterial> closureMaterial = pClosureMaterial;
            closureMaterial->InitClosureJoint(closureKey,pMaterials);

            pSplicedSegment->put_ClosureJointForegroundMaterial(endType,closureMaterial);
            pSplicedSegment->put_ClosureJointBackgroundMaterial(endType,nullptr);
         }
         else if ( bPierDiaphragm[endType] )
         {
            // need to get deck casting region index
            std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
            const CPierData2* pPier = (endType == etStart ? vPiers.front() : vPiers.back());
            PierIndexType pierIdx = pPier->GetIndex();

            const auto* pTimelineMgr = pSegment->GetGirder()->GetGirderGroup()->GetBridgeDescription()->GetTimelineManager();
            EventIndexType castDeckEventIdx = pTimelineMgr->GetCastDeckEventIndex();
            ATLASSERT(castDeckEventIdx != INVALID_INDEX);
            const auto* pTimelineEvent = pTimelineMgr->GetEventByIndex(castDeckEventIdx);
            ATLASSERT(pTimelineEvent && pTimelineEvent->GetCastDeckActivity().IsEnabled());
            const auto& castDeckActivity = pTimelineEvent->GetCastDeckActivity();
            IndexType deckCastingRegionIdx = INVALID_INDEX;
            if (castDeckActivity.GetCastingType() == CCastDeckActivity::Staged)
            {
               const auto& regions = castDeckActivity.GetCastingRegions();
               IndexType nRegions = castDeckActivity.GetCastingRegionCount();
               for (IndexType regionIdx = 0; regionIdx < nRegions; regionIdx++)
               {
                  if (regions[regionIdx].m_Type == CCastingRegion::Pier && regions[regionIdx].m_Index == pierIdx)
                  {
                     deckCastingRegionIdx = regionIdx;
                     break;
                  }
               }

               ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
            }
            else
            {
               deckCastingRegionIdx = 0;
            }

            CComObject<CAgeAdjustedMaterial>* pDiaphragmMaterial;
            CComObject<CAgeAdjustedMaterial>::CreateInstance(&pDiaphragmMaterial);
            CComPtr<IAgeAdjustedMaterial> diaphragmMaterial = pDiaphragmMaterial;
            diaphragmMaterial->InitDeck(deckCastingRegionIdx,pMaterials);

            pSplicedSegment->put_ClosureJointForegroundMaterial(endType,diaphragmMaterial);
            pSplicedSegment->put_ClosureJointBackgroundMaterial(endType,nullptr);
         }
      }
   }
}

void BuildAgeAdjustedJointMaterialModel(IBroker* pBroker, const CPrecastSegmentData* pSegment, ISuperstructureMemberSegment* segment, IAgeAdjustedMaterial** ppMaterial)
{
   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
   const CSplicedGirderData* pGirder = pSegment->GetGirder();

   // NOTE: TRICKY CODE FOR SPLICED GIRDER BEAMS
   // We need to model the material with an age adjusted modulus. The age adjusted modulus
   // is E/(1+XY) where XY is the age adjusted creep coefficient. In order to get the creep coefficient
   // we need the Volume and Surface Area of the segment (for the V/S ratio). This method
   // gets called during the creation of the bridge model. The bridge model is not
   // ready to compute V or S. Calling IMaterial::GetSegmentAgeAdjustedEc() will cause
   // recusion and validation errors. Using the age adjusted material object we can
   // delay the calls to GetAgeAdjustedEc until well after the time the bridge model
   // is validated.
   GET_IFACE2(pBroker, IMaterials, pMaterials);

   CComObject<CAgeAdjustedMaterial>* pJointMaterial;
   CComObject<CAgeAdjustedMaterial>::CreateInstance(&pJointMaterial);
   CComPtr<IAgeAdjustedMaterial> jointMaterial = pJointMaterial;
   jointMaterial->InitLongitudinalJoint(segmentKey, pMaterials);

   jointMaterial.CopyTo(ppMaterial);
}

void MakeRectangle(Float64 width, Float64 depth, Float64 xOffset, Float64 yOffset,IShape** ppShape)
{
   CComPtr<IRectangle> harp_rect;
   HRESULT hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT(SUCCEEDED(hr));

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   Float64 hook_offset = 0.0;

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(xOffset, yOffset - depth / 2.0);

   harp_rect->putref_HookPoint(hook);

   harp_rect->get_Shape(ppShape);
}

bool IsInEndBlock(Float64 Xs, pgsTypes::SectionBias sectionBias, Float64 leftEndBlockLength, Float64 rightEndBlockLength, Float64 Lg)
{
   if (
      (0.0 < leftEndBlockLength && (sectionBias == sbLeft ? IsLE(Xs, leftEndBlockLength) : IsLT(Xs, leftEndBlockLength))) ||
      (0.0 < rightEndBlockLength && (sectionBias == sbLeft ? IsLT(Lg - Xs, rightEndBlockLength) : IsLE(Lg - Xs, rightEndBlockLength)))
      )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool IsInEndBlock(Float64 Xs, pgsTypes::SectionBias sectionBias, Float64 endBlockLength, Float64 Lg)
{
   return IsInEndBlock(Xs, sectionBias, endBlockLength, endBlockLength, Lg);
}

bool IsSupportedDeckType(pgsTypes::SupportedDeckType deckType, const IBeamFactory* pFactory, pgsTypes::SupportedBeamSpacing spacingType)
{
   pgsTypes::SupportedDeckTypes deckTypes = pFactory->GetSupportedDeckTypes(spacingType);
   auto found = std::find(deckTypes.cbegin(), deckTypes.cend(), deckType);
   return found == deckTypes.end() ? false : true;
}

void LayoutIBeamEndBlockPointsOfInterest(const CSegmentKey& segmentKey,const CPrecastSegmentData* pSegment, Float64 segmentLength,pgsPoiMgr* pPoiMgr)
{
   if (0 < (pSegment->EndBlockLength[pgsTypes::metStart] + pSegment->EndBlockTransitionLength[pgsTypes::metStart]))
   {
      // start of end block transition (wide end, tapering down to width of web)
      if (IsZero(pSegment->EndBlockTransitionLength[pgsTypes::metStart]))
      {
         // there is an abrupt section change
         pgsPointOfInterest poiStartEndBlock1(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart], POI_SECTCHANGE_LEFTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock1) != INVALID_ID);

         pgsPointOfInterest poiStartEndBlock2(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart], POI_SECTCHANGE_RIGHTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock2) != INVALID_ID);
      }
      else
      {
         // there is a smooth taper over the transition length
         pgsPointOfInterest poiStartEndBlock1(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart], POI_SECTCHANGE_TRANSITION);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock1) != INVALID_ID);

         // end of end block transition (end block is the width of the web)
         pgsPointOfInterest poiStartEndBlock2(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart] + pSegment->EndBlockTransitionLength[pgsTypes::metStart], POI_SECTCHANGE_TRANSITION);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock2) != INVALID_ID);
      }
   }

   if (0 < (pSegment->EndBlockLength[pgsTypes::metEnd] + pSegment->EndBlockTransitionLength[pgsTypes::metEnd]))
   {
      if (IsZero(pSegment->EndBlockTransitionLength[pgsTypes::metEnd]))
      {
         // there is an abrupt section change
         pgsPointOfInterest poiEndEndBlock2(segmentKey, segmentLength - pSegment->EndBlockLength[pgsTypes::metEnd] - pSegment->EndBlockTransitionLength[pgsTypes::metEnd], POI_SECTCHANGE_LEFTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock2) != INVALID_ID);

         pgsPointOfInterest poiEndEndBlock1(segmentKey, segmentLength - pSegment->EndBlockLength[pgsTypes::metEnd] - pSegment->EndBlockTransitionLength[pgsTypes::metEnd], POI_SECTCHANGE_RIGHTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock1) != INVALID_ID);
      }
      else
      {
         // there is a smooth taper over the transition length

         // end of end block transition (end block is the width of the web)
         pgsPointOfInterest poiEndEndBlock2(segmentKey, segmentLength - pSegment->EndBlockLength[pgsTypes::metEnd] - pSegment->EndBlockTransitionLength[pgsTypes::metEnd], POI_SECTCHANGE_TRANSITION);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock2) != INVALID_ID);

         // start of end block transition (wide end)
         pgsPointOfInterest poiEndEndBlock1(segmentKey, segmentLength - pSegment->EndBlockLength[pgsTypes::metEnd], POI_SECTCHANGE_TRANSITION);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock1) != INVALID_ID);
      }
   }
}
