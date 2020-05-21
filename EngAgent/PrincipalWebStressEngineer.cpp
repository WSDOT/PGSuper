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
#include <PgsExt\PgsExt.h>
#include "PrincipalWebStressEngineer.h"

#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>
#include <IFace\Allowables.h>

#include <IFace\Project.h>
#include <PgsExt\LoadFactors.h>

pgsPrincipalWebStressEngineer::pgsPrincipalWebStressEngineer() :
   m_pBroker(nullptr), m_StatusGroupID(INVALID_ID)
{
   m_MorhCircle.CoCreateInstance(CLSID_MohrCircle);
}

pgsPrincipalWebStressEngineer::pgsPrincipalWebStressEngineer(IBroker* pBroker, StatusGroupIDType statusGroupID) :
   m_pBroker(pBroker), m_StatusGroupID(statusGroupID)
{
   m_MorhCircle.CoCreateInstance(CLSID_MohrCircle);
}

void pgsPrincipalWebStressEngineer::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

void pgsPrincipalWebStressEngineer::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;
}

const PRINCIPALSTRESSINWEBDETAILS* pgsPrincipalWebStressEngineer::GetPrincipalStressInWeb(const pgsPointOfInterest& poi) const
{
   if (poi.GetID() == INVALID_ID)
   {
      ATLASSERT(false);
      return nullptr;
   }

   auto found = m_Details.find(poi.GetID());
   if (found != m_Details.end())
   {
      return &((*found).second); // already been computed
   }

   PRINCIPALSTRESSINWEBDETAILS details = ComputePrincipalStressInWeb(poi);

   ATLASSERT(poi.GetID() != INVALID_ID);
   auto retval = m_Details.insert(std::make_pair(poi.GetID(), details));
   return &((*(retval.first)).second);
}

void pgsPrincipalWebStressEngineer::Check(const PoiList& vPois, pgsPrincipalTensionStressArtifact* pArtifact) const
{
   GET_IFACE(IAllowableConcreteStress, pAllowables);
   GET_IFACE(IMaterials, pMaterials);

   Float64 coefficient = pAllowables->GetAllowablePrincipalWebTensionStressCoefficient();

   GET_IFACE(IPointOfInterest, pPoi);
   Float64 fcReqd = -Float64_Max;
   for (const pgsPointOfInterest& poi : vPois)
   {
      const auto* pDetails = GetPrincipalStressInWeb(poi);

      Float64 stress_limit = pAllowables->GetAllowablePrincipalWebTensionStress(poi);

      // find controlling web section
      Float64 fmax = -Float64_Max;
      Float64 Yg; // web section elevation at controlling fmax
      std::_tstring strWebLocation; // web section description at controlling fmax
      for (const auto& webSection : pDetails->WebSections)
      {
         if (fmax < webSection.fmax)
         {
            fmax = webSection.fmax;
            Yg = webSection.YwebSection;
            strWebLocation = webSection.strLocation;
         }
      }

      // compute required concrete strength for this web section
      CClosureKey closureKey;
      Float64 lambda;
      if(pPoi->IsInClosureJoint(poi,&closureKey))
      {
         lambda = pMaterials->GetClosureJointLambda(closureKey);
      }
      else
      {
         lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());
      }
      Float64 fc_reqd = pow(fmax / (coefficient*lambda), 2);
      fcReqd = Max(fcReqd, fc_reqd); // we want the max

      // create check artifact for this poi
      pgsPrincipalTensionSectionArtifact artifact(poi, stress_limit, fmax, Yg, strWebLocation.c_str(), fcReqd);

      pArtifact->AddPrincipalTensionStressArtifact(artifact);
   } // next poi
}

PRINCIPALSTRESSINWEBDETAILS pgsPrincipalWebStressEngineer::ComputePrincipalStressInWeb(const pgsPointOfInterest& poi) const
{
   PRINCIPALSTRESSINWEBDETAILS details;
   pgsTypes::LimitState limitState = pgsTypes::ServiceIII;

   GET_IFACE(ISectionProperties, pSectProps);
   GET_IFACE(ILimitStateForces, pLSForces);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IProductForces, pProductForces);
   GET_IFACE(IAllowableConcreteStress, pAllowables);
   GET_IFACE(IGirder, pGirder);
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE(IDuctLimits, pDuctLimits);
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   std::_tstring specName = pSpec->GetSpecification();
   const auto* pSpecEntry = pLib->GetSpecEntry(specName.c_str());

   pgsTypes::PrincipalTensileStressMethod method;
   Float64 coefficient;
   Float64 ductDiameterFactor;
   pSpecEntry->GetPrincipalTensileStressInWebsParameters(&method, &coefficient,&ductDiameterFactor);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType cjCompsiteIntervalIdx = INVALID_INDEX;
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   // we will need these values latter in side loops.... get there here
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   pgsTypes::SectionPropertyType spType = pgsTypes::sptGross; // Section properties are based on gross concrete section (LRFD 5.9.2.3.3)

   GET_IFACE(IPointOfInterest, pPoi);
   bool bInClosureJoint = false;
   CClosureKey closureKey;
   if (pPoi->IsInClosureJoint(poi, &closureKey))
   {
      bInClosureJoint = true;
   }

   Float64 stress_limit = pAllowables->GetAllowablePrincipalWebTensionStress(poi);

   if (bInClosureJoint)
   {
      cjCompsiteIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
      details.Hg = pSectProps->GetHg(spType, cjCompsiteIntervalIdx, poi);
      details.Inc = pSectProps->GetIxx(spType, cjCompsiteIntervalIdx, poi);
   }
   else
   {
      details.Hg = pSectProps->GetHg(spType, releaseIntervalIdx, poi);
      details.Inc = pSectProps->GetIxx(spType, releaseIntervalIdx, poi);
   }

   details.Ic = pSectProps->GetIxx(spType, intervalIdx, poi);

   GET_IFACE(IPretensionForce, pPretensionForce);
   GET_IFACE(IPosttensionForce, pPTForce);
   details.Vp = pPretensionForce->GetVertHarpedStrandForce(poi, intervalIdx, pgsTypes::End, nullptr);
   details.Vp += pPTForce->GetSegmentTendonVerticalForce(poi, intervalIdx, pgsTypes::End, ALL_DUCTS);
   details.Vp += pPTForce->GetGirderTendonVerticalForce(poi, intervalIdx, pgsTypes::End, ALL_DUCTS);

   auto vWebFlangeInterfaces = pGirder->GetWebSections(poi);

   Float64 Ytnc;
   if (bInClosureJoint)
   {
      Ytnc = pSectProps->GetY(spType, cjCompsiteIntervalIdx, poi, pgsTypes::TopGirder);
   }
   else
   {
      Ytnc = pSectProps->GetY(spType, releaseIntervalIdx, poi, pgsTypes::TopGirder);
   }

   Float64 Ytc = pSectProps->GetY(spType, intervalIdx, poi, pgsTypes::TopGirder);
   Float64 tw = pGirder->GetWebWidth(poi); // Don't use GetShearWidth(). GetShearWidth is an average value, accounting for ducts, over the full depth of the member
                                           // We want the web width, not adjusted for ducts. Duct adjustment is done below.
                                           // NOTE: I-beams webs can vary in thickness over the depth of the member. This is almost never done in practice
                                           // We will use whatever GetWebWidth returns instead of creating a new method that gives web width at an elevation
   vWebFlangeInterfaces.emplace_back(-Ytc, tw, _T("Composite Centroid"));
   vWebFlangeInterfaces.emplace_back(-Ytnc, tw, _T("Noncomposite Centroid"));

   // sort so the elevations where principal tension stress is computed downwards from the top of the girder
   std::sort(vWebFlangeInterfaces.begin(), vWebFlangeInterfaces.end(), std::greater<>());

   for (const auto& wfInterface : vWebFlangeInterfaces)
   {
      auto YwebSection = std::get<0>(wfInterface); // this is in girder section coordinates
      auto bw = std::get<1>(wfInterface);
      auto strLocation(std::get<2>(wfInterface));

      // compute axial stress

      // if the web section is above the girder centroid at this evaluation interval (the composite section interval), we want maximum (tensile) stress near the top of the girder.
      // The stress is maximized at the top of the girder from negative moments, which are minimum moments for our sign convention. We also need the corresponding
      // stress in the bottom of the girder for the minimum moment which is the minimum (compressive) stress.

      // if the web section is below the composite girder centroid, we want the maximum (tensile) stress near the bottom of the girder. This occurs with positive (maximum) moment.
      // The corresponding stress at the top of the girder for the maximum moment is the minimum (compressive) stression.

      bool bWebSectionAboveCentroid = ::IsLE(-Ytc, YwebSection) ? true : false; // -Ytc because YwebSection is in girder section coordinates (Y=0 at top, negative is downwards)
      auto bat = pProductForces->GetBridgeAnalysisType(bWebSectionAboveCentroid ? pgsTypes::Minimize /*want minimum, or negative moments*/ : pgsTypes::Maximize /*want maximum, or positive moment*/);
      std::array<Float64, 2> fMin, fMax;
      pLSForces->GetStress(intervalIdx, limitState, poi, bat, true/*include prestress*/, pgsTypes::TopGirder, &fMin[pgsTypes::TopGirder], &fMax[pgsTypes::TopGirder]);
      pLSForces->GetStress(intervalIdx, limitState, poi, bat, true/*include prestress*/, pgsTypes::BottomGirder, &fMin[pgsTypes::BottomGirder], &fMax[pgsTypes::BottomGirder]);

      // we are seeking the maximum principal tensile stress so we want the maximum axial stress. At the extremeties of the section, it is typically easy to know
      // which case governings, however at the centroids, it can get a little more tricky. we will compute them both to determine which controls
      Float64 fpcx1 = fMax[pgsTypes::TopGirder] - (fMin[pgsTypes::BottomGirder] - fMax[pgsTypes::TopGirder])*YwebSection / details.Hg;
      Float64 fpcx2 = fMin[pgsTypes::TopGirder] - (fMax[pgsTypes::BottomGirder] - fMin[pgsTypes::TopGirder])*YwebSection / details.Hg;

      Float64 fTop, fBot, fpcx;
      if (fpcx2 < fpcx1)
      {
         fpcx = fpcx1;
         fTop = fMax[pgsTypes::TopGirder];
         fBot = fMin[pgsTypes::BottomGirder];
      }
      else
      {
         fpcx = fpcx2;
         fTop = fMin[pgsTypes::TopGirder];
         fBot = fMax[pgsTypes::BottomGirder];
      }

      // adjust bw for ducts

      // see if there is a segment duct near this section
      bool bNearSegmentDuct = false;
      Float64 max_segment_duct_deduction = 0;
      Float64 segment_duct_deduction_factor = pDuctLimits->GetSegmentDuctDeductionFactor(segmentKey, intervalIdx);
      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         Float64 OD = pSegmentTendonGeometry->GetOutsideDiameter(segmentKey, ductIdx);

         CComPtr<IPoint2d> pntDuct;
         pSegmentTendonGeometry->GetSegmentDuctPoint(poi, ductIdx, &pntDuct);
         if (pntDuct)
         {
            Float64 Yduct;
            pntDuct->get_Y(&Yduct);

            if (::InRange(Yduct - ductDiameterFactor*OD, YwebSection, Yduct + ductDiameterFactor*OD))
            {
               // the duct is near. compute the deduction for this duct
               bNearSegmentDuct = true;
               max_segment_duct_deduction = Max(max_segment_duct_deduction, segment_duct_deduction_factor*OD);
            }
         }
      }

      // see if there is a girder duct near this section
      bool bNearGirderDuct = false;
      Float64 max_girder_duct_deduction = 0;
      for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
      {
         Float64 girder_duct_deduction_factor = pDuctLimits->GetGirderDuctDeductionFactor(segmentKey, ductIdx, intervalIdx);
         Float64 OD = pGirderTendonGeometry->GetOutsideDiameter(segmentKey, ductIdx);

         CComPtr<IPoint2d> pntDuct;
         pGirderTendonGeometry->GetGirderDuctPoint(poi, ductIdx, &pntDuct);
         if (pntDuct)
         {
            Float64 Yduct;
            pntDuct->get_Y(&Yduct);

            if (::InRange(Yduct - ductDiameterFactor*OD, YwebSection, Yduct + ductDiameterFactor*OD))
            {
               // the duct is near. compute the deduction for this duct
               bNearGirderDuct = true;
               max_girder_duct_deduction = Max(max_girder_duct_deduction, girder_duct_deduction_factor*OD);
            }
         }
      }

      if (bNearSegmentDuct || bNearGirderDuct)
      {
         // there are one or more duct near this duct
         // adjust the web width by the maximum duct deduction
         Float64 duct_deduction = Max(max_segment_duct_deduction, max_girder_duct_deduction);
         bw -= duct_deduction;
      }

      // Get shear force
      auto maxBat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
      auto minBat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);
      Float64 Vnc, Vc;
      if (method == pgsTypes::ptsmLRFD)
      {
         sysSectionValue dummy, Vmin, Vmax;
         pLSForces->GetShear(intervalIdx, limitState, poi, maxBat, &dummy, &Vmax);
         pLSForces->GetShear(intervalIdx, limitState, poi, minBat, &Vmin, &dummy);
         Vc = Max(fabs(Vmin.Left()), fabs(Vmin.Right()), fabs(Vmax.Left()), fabs(Vmax.Right()));
      }
      else
      {
         sysSectionValue V_nc = GetNonCompositeShear(maxBat, noncompositeIntervalIdx, limitState, poi);
         Vnc = Max(fabs(V_nc.Left()), fabs(V_nc.Right()));
         details.Vnc = Vnc;

         sysSectionValue dummy, Vmin, Vmax;
         GetCompositeShear(maxBat, intervalIdx, limitState, poi, &dummy, &Vmax);
         GetCompositeShear(minBat, intervalIdx, limitState, poi, &Vmin, &dummy);
         Vc = Max(fabs(Vmin.Left()), fabs(Vmin.Right()), fabs(Vmax.Left()), fabs(Vmax.Right()));
      }

      Float64 Qc, Qnc;
      Float64 t, f_max;
      if (method == pgsTypes::ptsmLRFD)
      {
         Qnc = 0;
         Qc = pSectProps->GetQ(spType, intervalIdx, poi, YwebSection);

         t = (Vc - details.Vp)*Qc / (bw*details.Ic);

         m_MorhCircle->put_Sii(fpcx);
         m_MorhCircle->put_Sjj(0);
         m_MorhCircle->put_Sij(t);

         m_MorhCircle->get_Smax(&f_max);
      }
      else
      {
         if (bInClosureJoint)
         {
            Qnc = pSectProps->GetQ(spType, cjCompsiteIntervalIdx, poi, YwebSection);
         }
         else
         {
            Qnc = pSectProps->GetQ(spType, releaseIntervalIdx, poi, YwebSection);
         }
         Qc = pSectProps->GetQ(spType, intervalIdx, poi, YwebSection);

         t = (Vnc - details.Vp)*Qnc / (bw*details.Inc) + Vc*Qc / (bw*details.Ic);

         m_MorhCircle->put_Sii(fpcx);
         m_MorhCircle->put_Sjj(0);
         m_MorhCircle->put_Sij(t);

         m_MorhCircle->get_Smax(&f_max);
      }

      details.WebSections.emplace_back(strLocation.c_str(), YwebSection, fTop, fBot, fpcx, Vc, Qnc, Qc, bw, bNearSegmentDuct || bNearGirderDuct/*was bw reduced for a duct?*/, t, f_max);
   } // next evaluation point

   return details;
}

sysSectionValue pgsPrincipalWebStressEngineer::GetNonCompositeShear(pgsTypes::BridgeAnalysisType bat,IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi) const
{
   GET_IFACE(ILoadFactors, pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDC = pLoadFactors->GetDCMax(limitState);
   Float64 gDW = pLoadFactors->GetDWMax(limitState);
#if defined _DEBUG
   Float64 gDCmin, gDCmax;
   pLoadFactors->GetDC(limitState, &gDCmin, &gDCmax);
   ATLASSERT(IsEqual(gDCmin, gDCmax));
   Float64 gDWmin, gDWmax;
   pLoadFactors->GetDW(limitState, &gDWmin, &gDWmax);
   ATLASSERT(IsEqual(gDWmin, gDWmax));
#endif

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ICombinedForces, pCombinedForces);
   sysSectionValue Vdc = pCombinedForces->GetShear(intervalIdx, lcDC, poi, bat, rtCumulative);
   sysSectionValue Vdw = pCombinedForces->GetShear(intervalIdx, lcDW, poi, bat, rtCumulative);
   sysSectionValue Vnc = gDC*Vdc + gDW*Vdw;

   return Vnc;
}

void pgsPrincipalWebStressEngineer::GetCompositeShear(pgsTypes::BridgeAnalysisType bat, IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, sysSectionValue* pVmin, sysSectionValue* pVmax) const
{
   GET_IFACE(ILimitStateForces, pLimitStateForces);
   sysSectionValue Vu_min, Vu_max;
   pLimitStateForces->GetShear(intervalIdx, limitState, poi, bat, &Vu_min, &Vu_max);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   sysSectionValue Vnc = GetNonCompositeShear(bat, noncompositeIntervalIdx, limitState, poi);

   *pVmin = Vu_min - Vnc;
   *pVmax = Vu_max - Vnc;
}
