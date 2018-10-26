///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// EffectiveFlangeWidthTool.cpp : Implementation of CEffectiveFlangeWidthTool
#include "stdafx.h"

#include "EffectiveFlangeWidthTool.h"
#include <MathEx.h>
#include <PgsExt\ReportStyleHolder.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>

#include <MfcTools\Exceptions.h>
#include "..\PGSuperException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Tributary width and effective flange width can be cached to improve speed
#pragma Reminder("UPDATE: This could be more efficient")

/////////////////////////////////////////////////////////////////////////////
// CEffectiveFlangeWidthTool
void CEffectiveFlangeWidthTool::Init(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   // Register status callbacks that we want to use
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidInformationalWarning = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidEffectiveFlangeWidthWarning = pStatusCenter->RegisterCallback(new pgsEffectiveFlangeWidthStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidEffectiveFlangeWidthInfo = pStatusCenter->RegisterCallback(new pgsEffectiveFlangeWidthStatusCallback(m_pBroker,eafTypes::statusOK));
}

HRESULT CEffectiveFlangeWidthTool::FinalConstruct()
{
   HRESULT hr = m_Tool.CoCreateInstance(CLSID_EffectiveFlangeWidthTool);
   if ( FAILED(hr) )
      return hr;

   m_bUseTribWidth = VARIANT_FALSE;

   return S_OK;
}

void CEffectiveFlangeWidthTool::FinalRelease()
{
   m_Tool.Release();
}

STDMETHODIMP CEffectiveFlangeWidthTool::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
      &IID_IReportEffectiveFlangeWidth,
		&IID_IPGSuperEffectiveFlangeWidthTool,
		&IID_IEffectiveFlangeWidthTool
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CEffectiveFlangeWidthTool::put_UseTributaryWidth(VARIANT_BOOL bUseTribWidth)
{
   m_bUseTribWidth = bUseTribWidth;
   return S_OK;
}

STDMETHODIMP CEffectiveFlangeWidthTool::get_UseTributaryWidth(VARIANT_BOOL* bUseTribWidth)
{
   *bUseTribWidth = m_bUseTribWidth;
   return S_OK;
}

STDMETHODIMP CEffectiveFlangeWidthTool::TributaryFlangeWidth(IGenericBridge* bridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 location, Float64 *tribFlangeWidth)
{
   Float64 twLeft,twRight;
   return TributaryFlangeWidthEx(bridge,spanIdx,gdrIdx,location,&twLeft,&twRight,tribFlangeWidth);
}

STDMETHODIMP CEffectiveFlangeWidthTool::TributaryFlangeWidthEx(IGenericBridge* bridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 location,Float64* twLeft, Float64* twRight, Float64 *tribFlangeWidth)
{
   ATLASSERT( spanIdx != ALL_SPANS);
   ATLASSERT( gdrIdx != ALL_GIRDERS );

   PoiStageKey key(pgsTypes::BridgeSite2,spanIdx,gdrIdx,location);
   TWContainer::iterator found = m_TFW.find(key);
   if ( found != m_TFW.end() )
   {
      TribWidth tw = (*found).second;
      (*twLeft)          = tw.twLeft;
      (*twRight)         = tw.twRight;
      (*tribFlangeWidth) = tw.tribWidth;
   }
   else
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      pgsTypes::SupportedBeamSpacing beamSpacing = pBridgeDesc->GetGirderSpacingType();
      if ( IsSpreadSpacing(beamSpacing) )
      {
         // the bridge uses spread girders... the WBFL::GenericBridgeTools knows how to deal with this
         HRESULT hr = m_Tool->TributaryFlangeWidthEx(bridge,spanIdx,gdrIdx,location,twLeft,twRight,tribFlangeWidth);
         if ( FAILED(hr) )
            return hr;

         TribWidth tw;
         tw.twLeft    = *twLeft;
         tw.twRight   = *twRight;
         tw.tribWidth = *tribFlangeWidth;

         m_TFW.insert( std::make_pair(key,tw) );
      }
      else
      {
         // the bridge uses adjacent girders... tributary width is simply the width of the girder + half
         // the joint spacing on either side.
         pgsPointOfInterest poi(spanIdx,gdrIdx,location);
         GET_IFACE(IGirder,pGirder);

         Float64 topWidth = pGirder->GetTopWidth(poi);
         Float64 botWidth = pGirder->GetBottomWidth(poi);

         GirderIndexType nGirders = pBridgeDesc->GetSpan(spanIdx)->GetGirderCount();
         Float64 width;
         if ( gdrIdx == 0 || gdrIdx == nGirders-1 )
         {
            // exterior girder

            // for exterior girders, the deck only goes to the edge of the top flange on the exterior side
            // on the interior side, the width is the governing of the top and bottom widths.

            // squint really hard and you'll see a box beam here that has a bottom width greater than
            // the top width.

            //     w top/2    w bot/2
            //   |<-------->|<------->|
            //   |      trib width    |
            //   |<------------------>|
            //   ==================== |
            //    ||              ||  |
            //    ||              ||  |
            //    ||              ||  |
            //    ||              ||  |
            //    ||              ||  |
            //  ======================|
            width = topWidth/2 + _cpp_max(topWidth,botWidth)/2;
         }
         else
         {
            width = _cpp_max(topWidth,botWidth);
         }

         // deal with joints
         Float64 leftJoint  = 0.0;
         Float64 rightJoint = 0.0;

         if ( IsJointSpacing(beamSpacing) )
         {
            // The joint width is stored as the "girder spacing".
            SpacingIndexType nSpaces = nGirders-1;
            SpacingIndexType leftSpaceIdx = (0 < gdrIdx ? gdrIdx-1 : INVALID_INDEX);
            SpacingIndexType rightSpaceIdx = gdrIdx;

            Float64 leftJointStart, leftJointEnd, rightJointStart, rightJointEnd;
            if ( leftSpaceIdx == INVALID_INDEX )
            {
               leftJointStart = 0;
               leftJointEnd   = 0;
            }
            else
            {
               leftJointStart = pBridgeDesc->GetSpan(spanIdx)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(leftSpaceIdx);
               leftJointEnd   = pBridgeDesc->GetSpan(spanIdx)->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(leftSpaceIdx);
            }

            if ( nSpaces-1 < rightSpaceIdx )
            {
               rightJointStart = 0;
               rightJointEnd   = 0;
            }
            else
            {
               rightJointStart = pBridgeDesc->GetSpan(spanIdx)->GetGirderSpacing(pgsTypes::metStart)->GetGirderSpacing(rightSpaceIdx);
               rightJointEnd   = pBridgeDesc->GetSpan(spanIdx)->GetGirderSpacing(pgsTypes::metEnd)->GetGirderSpacing(rightSpaceIdx);
            }

            GET_IFACE(IBridge,pBridge);
            Float64 gdr_length = pBridge->GetGirderLength(spanIdx,gdrIdx);

            leftJoint  = LinInterp(location, leftJointStart,  leftJointEnd,  gdr_length);
            rightJoint = LinInterp(location, rightJointStart, rightJointEnd, gdr_length);
         }

         ATLASSERT( !IsZero(width) );


         TribWidth tw;
         tw.twLeft    = (leftJoint + width)/2;
         tw.twRight   = (rightJoint + width)/2;
         tw.tribWidth = tw.twLeft + tw.twRight;

         (*twLeft)          = tw.twLeft;
         (*twRight)         = tw.twRight;
         (*tribFlangeWidth) = tw.tribWidth;

         m_TFW.insert( std::make_pair(key,tw) );
      }
   }

   return S_OK;
}

STDMETHODIMP CEffectiveFlangeWidthTool::EffectiveFlangeWidth(IGenericBridge* bridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 location, Float64 *effFlangeWidth)
{
   EffFlangeWidth efw;
   HRESULT hr = EffectiveFlangeWidthDetails(bridge,spanIdx,gdrIdx,location, &efw);
   *effFlangeWidth = efw.effFlangeWidth;
   return S_OK;
}

HRESULT CEffectiveFlangeWidthTool::EffectiveFlangeWidthDetails(IGenericBridge* bridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 location, EffFlangeWidth* effFlangeWidth)
{
   // Computes effective flange width, retaining details of calculation per LRFD 4.6.2.6.1
   if ( m_bUseTribWidth == VARIANT_TRUE || lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IGirder,pGirder);
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEffectiveFlangeWidth, pIEffFW);

      if ( pIEffFW->IgnoreEffectiveFlangeWidthLimits() )
      {
         std::_tostringstream os;
         os << "Limitations on effective flange width calculations defined in LRFD 4.6.2.6.1 have been ignored" << std::endl;
         pgsEffectiveFlangeWidthStatusItem* pStatusItem = 
            new pgsEffectiveFlangeWidthStatusItem(m_StatusGroupID,m_scidEffectiveFlangeWidthInfo,os.str().c_str());

         pStatusCenter->Add(pStatusItem);
      }

      // start by checking applicability of the method
      // 2.0 <= L/S (span length over girder spacing)
      // Largest skew in the bridge <= 75(deg)... skew measured relative to centerline of girder!!!!
      // Overhang <= S/2

      // If Overhang > S/2, then limit amount of overhang to S/2 and don't use traffic barrier stiffness
      // (See C4.6.2.6.1, last paragraph on page 4-52... since provisions were developed for overhangs
      // < S/2, we will limit the overhang to S/2 for purposes for this calculation).

      // is this an exterior girder?
      bool bIsExteriorGirder = pBridge->IsExteriorGirder(spanIdx,gdrIdx);

      // get tributary width
      Float64 twLeft,twRight,wTrib;
      TributaryFlangeWidthEx(bridge,spanIdx,gdrIdx,location,&twLeft,&twRight,&wTrib);

      CComPtr<ISpanCollection> spans;
      bridge->get_Spans(&spans);

      CComPtr<ISpan> span;
      spans->get_Item(spanIdx,&span);

      GirderIndexType nGirders;
      span->get_GirderCount(&nGirders);

      bool bOverhangCheckFailed = false;

      if ( 1 < nGirders )
      {
         // get span length
         Float64 L = pBridge->GetSpanLength(spanIdx,gdrIdx);

         // Girder spacing normal to the CL girder is the tributary width
         Float64 S1,S2;
         TributaryFlangeWidth(bridge,spanIdx,gdrIdx,0.0,&S1);
         TributaryFlangeWidth(bridge,spanIdx,gdrIdx,  L,&S2);

         Float64 S = _cpp_max(S1,S2);

         if ( !pIEffFW->IgnoreEffectiveFlangeWidthLimits() && L/S < 2.0 )
         {
            //  ratio of span length to girder spacing is out of range
            std::_tostringstream os;
            os << "The ratio of span length to girder spacing (L/S) is less that 2. The effective flange width cannot be computed (LRFD 4.6.2.6.1)" << std::endl;

            pgsEffectiveFlangeWidthStatusItem* pStatusItem = 
               new pgsEffectiveFlangeWidthStatusItem(m_StatusGroupID,m_scidEffectiveFlangeWidthWarning,os.str().c_str());

            pStatusCenter->Add(pStatusItem);

            os << "See Status Center for Details";
            THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
         }

         // check overhang spacing if it is a CIP or SIP deck
         // overlay decks don't have overhangs
         if ( pBridge->GetDeckType() != pgsTypes::sdtCompositeOverlay )
         {
            Float64 left_trib_width_adjustment  = pGirder->GetCL2ExteriorWebDistance(pgsPointOfInterest(spanIdx,0,location));
            Float64 right_trib_width_adjustment = pGirder->GetCL2ExteriorWebDistance(pgsPointOfInterest(spanIdx,nGirders-1,location));
            Float64 left_overhang  = twLeft - left_trib_width_adjustment;
            Float64 right_overhang = twRight - right_trib_width_adjustment;
            if ( !pIEffFW->IgnoreEffectiveFlangeWidthLimits() && (gdrIdx == 0 && S/2 < left_overhang && !IsEqual(S/2,left_overhang)) || (gdrIdx == (nGirders-1) && S/2 < right_overhang && !IsEqual(S/2,right_overhang)) )
            {
               bOverhangCheckFailed = true;
               // force tributary area to be S/2
               if ( gdrIdx == 0 && IsGE(S/2,left_overhang) )
                  twLeft = twRight;

               if ( gdrIdx == nGirders-1 && IsGE(S/2,right_overhang) )
                  twRight = twLeft;

               // overhang is too big
               std::_tostringstream os;
               os << "The slab overhang exceeds S/2. The overhang is taken to be equal to S/2 for purposes of computing the effective flange width and the effect of structurally continuous barriers has been ignored. (LRFD 4.6.2.6.1)" << std::endl;

               pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalWarning,os.str().c_str());
               pStatusCenter->Add(pStatusItem);

               wTrib = twLeft + twRight;
            }
         }


         // the code below could be more efficient if the evaluation happens once and is cached
         // if the framing plan doesn't change, this evaluation will not change
   #pragma Reminder("UPDATE: This could be more efficient")
         if ( !pIEffFW->IgnoreEffectiveFlangeWidthLimits() )
         {
            // check maximum skew angle... AASHTO defines the skew angle as...
            // The largest skew angle (theta) in the BRIDGE SYSTEM where (theta)
            // is the angle of a bearing line measured relative to a normal to
            // the cneterline of a longitudial component
            Float64 maxSkew = 0;
            PierIndexType nPiers = pBridge->GetPierCount();
            for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
            {
               CComPtr<IDirection> pierDirection;
               pBridge->GetPierDirection(pierIdx,&pierDirection);

               SpanIndexType spanIndex = pierIdx;
               if ( nPiers-1 == pierIdx ) // at the last pier... use the previous span index
               {
                  ATLASSERT(0 < spanIndex);
                  spanIndex -= 1;
               }

               GirderIndexType nGirders = pBridge->GetGirderCount(spanIndex);
               for ( GirderIndexType gdr = 0; gdr < nGirders; gdr++ )
               {
                  CComPtr<IDirection> girderDirection;
                  pBridge->GetGirderBearing(spanIndex,gdr,&girderDirection);

                  CComPtr<IDirection> girderNormal;
                  girderDirection->Increment(CComVariant(PI_OVER_2),&girderNormal);

                  CComPtr<IAngle> angle;
                  girderNormal->AngleBetween(pierDirection,&angle);

                  Float64 angle_value;
                  angle->get_Value(&angle_value);

                  if ( M_PI < angle_value )
                     angle_value = TWO_PI - angle_value;

                  maxSkew = _cpp_max(maxSkew,angle_value);
               }
            }

            if ( 75.*M_PI/180. < maxSkew )
            {
               // skew is too large
               std::_tostringstream os;
               os << "The maximum skew angle in the bridge system exceeds the limit of 75 degrees for computing effective flange width (LRFD 4.6.2.6.1)" << std::endl;

               pgsBridgeDescriptionStatusItem* pStatusItem = 
                  new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,0,os.str().c_str());

               pStatusCenter->Add(pStatusItem);

               os << "See Status Center for Details";
               THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
            }
         }
      }


      // Figure out if it is an exterior girder and if so, if the barrier is structurally continuous.
      // If the barrier is structurally continuous extend the effective flange width by Ab/(2ts)
      Float64 wAdd = 0;


      Float64 Ab = 0;
      Float64 ts = 0;
      VARIANT_BOOL bIsStructurallyContinuous = VARIANT_FALSE;
      // is it an exterior girder?
      if ( bIsExteriorGirder )
      {
         // yes!!!

         // left or right exterior girder... get the barrier
         CComPtr<ISidewalkBarrier> barrier;
         if ( gdrIdx == 0 )
         {
            // if this is an exterior girder then we need to add the effect of the barriers if they are continuous
            bridge->get_LeftBarrier(&barrier);
         }
         else if ( gdrIdx == (nGirders-1) )
         {
            bridge->get_RightBarrier(&barrier);
         }

         // is it structurally continuous
         barrier->get_IsStructurallyContinuous(&bIsStructurallyContinuous);
         if ( bIsStructurallyContinuous == VARIANT_TRUE && !bOverhangCheckFailed )
         {
            // yes!!
            CComPtr<IShape> shape;
            barrier->get_StructuralShape(&shape);

            CComPtr<IShapeProperties> props;
            shape->get_ShapeProperties(&props);

            // get area of barrier
            props->get_Area(&Ab);

            // if there is a deck, get the slab thickness
            CComPtr<IBridgeDeck> deck;
            bridge->get_Deck(&deck);
            if ( deck )
            {
               deck->get_StructuralDepth(&ts);
            }
            else
            {
               ATLASSERT(false); // should never get here... there is no effective flange width if there isn't a deck

               // but just incase... use the top flange thickness


               // otherwise, use the thickness of the top flange of the girder (this is a decked girder)
               GET_IFACE(IGirder,pGirder);
               ts = pGirder->GetMinTopFlangeThickness(pgsPointOfInterest(spanIdx,gdrIdx,location));
            }

            // amount to be added to tributary width
            ATLASSERT( !IsZero(Ab) );
            ATLASSERT( !IsZero(ts) );
            wAdd = Ab/(2*ts);
         }
      }

      effFlangeWidth->Ab = Ab;
      effFlangeWidth->ts = ts;
      effFlangeWidth->bContinuousBarrier = (bIsStructurallyContinuous == VARIANT_TRUE);
      effFlangeWidth->tribWidth = wTrib;
      effFlangeWidth->twLeft    = twLeft;
      effFlangeWidth->twRight   = twRight;
      effFlangeWidth->effFlangeWidth = wTrib + wAdd;

      return S_OK;
   }
   else
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      pgsTypes::SupportedBeamSpacing beamSpacing = pBridgeDesc->GetGirderSpacingType();
      if ( IsSpreadSpacing(beamSpacing) )
      {
         GET_IFACE(IBridge,pBridge);

         CComPtr<IEffectiveFlangeWidthDetails> details;
         HRESULT hr = EffectiveFlangeWidthEx(bridge,spanIdx,gdrIdx,location,&details);
         if ( FAILED(hr) )
            return hr;

         effFlangeWidth->m_Details = details;
         details->EffectiveFlangeWidth(&(effFlangeWidth->effFlangeWidth));

         // exterior girders - must take half of interior and add 
         if (pBridge->IsExteriorGirder(spanIdx,gdrIdx) && pBridge->GetGirderCount(spanIdx)>2)
         {
            GirderIndexType gdr = (0 < gdrIdx) ? gdrIdx-1 : 1;

            CComPtr<IEffectiveFlangeWidthDetails> intdetails;
            HRESULT hr = EffectiveFlangeWidthEx(bridge,spanIdx,gdr,location,&intdetails);
            if ( FAILED(hr) )
               return hr;

            Float64 eff_wid;
            intdetails->EffectiveFlangeWidth(&eff_wid);

            effFlangeWidth->effFlangeWidth = (effFlangeWidth->effFlangeWidth + eff_wid)/2.0;
         }
      }
      else
      {
         Float64 wTrib, twLeft, twRight;
         TributaryFlangeWidthEx(bridge,spanIdx,gdrIdx,location,&twLeft,&twRight,&wTrib);
         effFlangeWidth->Ab = 0;
         effFlangeWidth->ts = 0;
         effFlangeWidth->bContinuousBarrier = false;
         effFlangeWidth->tribWidth = wTrib;
         effFlangeWidth->twLeft    = twLeft;
         effFlangeWidth->twRight   = twRight;
         effFlangeWidth->effFlangeWidth = wTrib;
      }

      return S_OK;
   }

   return E_FAIL;
}

STDMETHODIMP CEffectiveFlangeWidthTool::EffectiveFlangeWidthEx(IGenericBridge* bridge,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 location, IEffectiveFlangeWidthDetails** details)
{
   // should only get here if the effective flange width is computed
   ATLASSERT( m_bUseTribWidth == VARIANT_FALSE );

   PoiStageKey key(pgsTypes::BridgeSite2,(SpanIndexType)spanIdx,(GirderIndexType)gdrIdx,location);
   FWContainer::iterator found = m_EFW.find(key);
   if ( found != m_EFW.end() )
   {
      EffFlangeWidth efw = (*found).second;

      (*details) = efw.m_Details;
      (*details)->AddRef();
   }
   else
   {
      HRESULT hr = m_Tool->EffectiveFlangeWidthEx(bridge,spanIdx,gdrIdx,location,details);
      if ( FAILED(hr) )
         return hr;

      EffFlangeWidth efw;
      efw.m_Details = *details;
      m_EFW.insert(std::make_pair(key,efw));
   }

   return S_OK;
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bInterior = pBridge->IsInteriorGirder(span,gdr);

   if ( bInterior )
      ReportEffectiveFlangeWidth_InteriorGirder(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
   else
      ReportEffectiveFlangeWidth_ExteriorGirder(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_InteriorGirder(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IGirder,pGirder);
   if ( pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,gdr) )
   {
      ReportEffectiveFlangeWidth_InteriorGirder_Prismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
   }
   else
   {
      ReportEffectiveFlangeWidth_InteriorGirder_Nonprismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
   }
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_InteriorGirder_Prismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
    std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

    INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
    INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );

    rptParagraph* pPara = new rptParagraph;
    *pChapter << pPara;

    GET_IFACE2(pBroker,IBridge,  pBridge);
    GET_IFACE2(pBroker,ISectProp2,pSectProp);
    GET_IFACE2(pBroker,IGirder,  pGdr);
    GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

    GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
    std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_MIDSPAN);
    ATLASSERT(vPoi.size() == 1);
    pgsPointOfInterest poi = vPoi[0];

   EffFlangeWidth efw;
   EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&efw);

   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetInteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   if ( efw.m_Details )
    {
       Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
       Float64 effSpanLength;
       efw.m_Details->get_EffectiveSpanLength(&effSpanLength);
       efw.m_Details->get_SlabThickness(&tSlab);

       FlangeIndexType count;
       efw.m_Details->get_FlangeCount(&count);

       *pPara << rptNewLine;
       *pPara << _T("Effective flange width is the least of: ") << rptNewLine;
       *pPara << symbol(DOT) << _T(" One-quarter of the effective span length (") << Sub2(_T("w"),_T("1")) << _T(")") << rptNewLine;
       *pPara << symbol(DOT) << _T(" 12 times the average thickness of the slab, plus the greater of the web thickness");
       *pPara << _T(" OR one-half the width of the top flange of the girder (") << Sub2(_T("w"),_T("2")) << _T(")") << rptNewLine;
       *pPara << symbol(DOT) << _T(" The average spacing of adjacent beams (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
       *pPara << rptNewLine;

       if ( 1 < count )
       {
          *pPara << Sub2(_T("w"),_T("2")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("2")) << _T(" for each web") << rptNewLine;
          *pPara << Sub2(_T("w"),_T("3")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("3")) << _T(" for each web") << rptNewLine << rptNewLine;
       }

       (*pPara) << Sub2(_T("w"),_T("1")) << _T(" = (0.25)(") << xdim.SetValue(effSpanLength) << _T(") = ");
       (*pPara) << xdim.SetValue(0.25*effSpanLength) << _T(" = ") << xdim2.SetValue(0.25*effSpanLength) << rptNewLine;

       for ( FlangeIndexType flangeIdx = 0; flangeIdx < count; flangeIdx++ )
       {
          efw.m_Details->GetFlangeParameters(flangeIdx,&tWeb,&wFlange,&lSpacing,&rSpacing);

          if ( 1 < count )
             (*pPara) << rptNewLine << _T("Web ") << long(flangeIdx+1) << rptNewLine;

          (*pPara) << Sub2(_T("w"),_T("2")) << _T(" = (12.0)(") << xdim2.SetValue(tSlab) << _T(") + greater of [ ");
          (*pPara) << xdim2.SetValue(tWeb) << _T(" : (0.5)(");
          (*pPara) << xdim2.SetValue(wFlange) << _T(") ] = ");
          (*pPara) << xdim2.SetValue( 12*tSlab + _cpp_max(tWeb,0.5*wFlange) ) << rptNewLine;

          (*pPara) << Sub2(_T("w"),_T("3")) << _T(" = (") << xdim.SetValue(lSpacing*2) << _T(" + ");
          (*pPara) << xdim.SetValue(rSpacing*2) << _T(")/2 = ");
          (*pPara) << xdim.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
          (*pPara) << _T(" = ") << xdim2.SetValue(lSpacing+rSpacing) << rptNewLine;
       }

       Float64 effFlangeWidth;
       efw.m_Details->EffectiveFlangeWidth(&effFlangeWidth);
       (*pPara) << rptNewLine << _T("Effective Flange Width = ") << xdim2.SetValue(effFlangeWidth) << rptNewLine;
    }
   else
   {
      if ( IsSpreadSpacing(pIBridgeDesc->GetGirderSpacingType()) )
      {
         *pPara << _T("Effective flange width is taken as one-half the distance to the adjacent girder on each side of the component") << rptNewLine;
         *pPara << _T("Left Tributary Width = ") << xdim.SetValue(efw.twLeft*2) << rptNewLine;
         *pPara << _T("Right Tributary Width = ") << xdim.SetValue(efw.twRight*2) << rptNewLine;
         *pPara << _T("Effective Flange Width = ") << xdim.SetValue(efw.effFlangeWidth) << _T(" = ") << xdim2.SetValue(efw.effFlangeWidth) << rptNewLine;
      }
      else
      {
         *pPara << _T("Effective flange width is taken as the girder width") << rptNewLine;
         *pPara << _T("Effective Flange Width = ") << xdim.SetValue(efw.effFlangeWidth) << _T(" = ") << xdim2.SetValue(efw.effFlangeWidth) << rptNewLine;
      }
   }
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_InteriorGirder_Nonprismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IGirder,pGirder);
   MatingSurfaceIndexType nWebs = pGirder->GetNumberOfMatingSurfaces(span,gdr);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool use_tributary_width = DoUseTributaryWidth( pBridgeDesc);

   rptRcTable* table;
   GET_IFACE2(pBroker,IBridge,pBridge);

   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetInteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   if ( use_tributary_width )
   {
      *pPara << _T("Effective flange width is taken as one-half the distance to the adjacent girder on each side of the component") << rptNewLine;
      table = pgsReportStyleHolder::CreateDefaultTable(4,_T(""));
   }
   else
   {
      if ( 1 < nWebs )
      {
         *pPara << _T("LRFD 4.6.2.6.1 and  C4.6.2.6.1") << rptNewLine;
         *pPara << _T("For open boxes, the effective flange width of each web should be determined as though each web was an individual supporting element.") << rptNewLine;
      }
      else
      {
         *pPara << _T("LRFD 4.6.2.6.1") << rptNewLine;
      }

      *pPara << rptNewLine;
      *pPara << _T("Effective flange width is the least of: ") << rptNewLine;
      *pPara << symbol(DOT) << _T(" One-quarter of the effective span length (") << Sub2(_T("w"),_T("1")) << _T(")") << rptNewLine;
      *pPara << symbol(DOT) << _T(" 12 times the average thickness of the slab, plus the greater of the web thickness");
      *pPara << _T(" OR one-half the width of the top flange of the girder (") << Sub2(_T("w"),_T("2")) << _T(")") << rptNewLine;
      *pPara << symbol(DOT) << _T(" The average spacing of adjacent beams (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
      *pPara << rptNewLine;

         if ( 1 < nWebs )
      {
         *pPara << Sub2(_T("w"),_T("2")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("2")) << _T(" for each web") << rptNewLine;
         *pPara << Sub2(_T("w"),_T("3")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("2")) << _T(" for each web") << rptNewLine << rptNewLine;
      }
      
      table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
   }

   *pPara << table;


   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   if ( use_tributary_width )
   {
      (*table)(0,1) << COLHDR(_T("Left Spacing"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*table)(0,2) << COLHDR(_T("Right Spacing"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
      (*table)(0,3) << COLHDR(_T("Effective Flange Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      (*table)(0,1) << _T("");
      table->SetColumnWidth(1,5.0);
   }

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,   pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, xdim,   pDisplayUnits->GetComponentDimUnit(),   false );

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_TABULAR);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
   
      EffFlangeWidth efw;
      EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&efw);

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite2, poi, end_size );
      if ( efw.m_Details )
      {

         ReportEffectiveFlangeWidth_InteriorGirderRow(efw.m_Details,row,table,pDisplayUnits);
      }
      else
      {
         (*table)(row,1) << xdim.SetValue(efw.twLeft);
         (*table)(row,2) << xdim.SetValue(efw.twRight);
         (*table)(row,3) << xdim.SetValue(efw.effFlangeWidth);
      }

      row++;
   }
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_InteriorGirderRow(IEffectiveFlangeWidthDetails* details,RowIndexType row,rptRcTable* table,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spanLength, pDisplayUnits->GetSpanLengthUnit(),   true  );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,     pDisplayUnits->GetComponentDimUnit(), true  );

   Float64 effSpanLength;
   details->get_EffectiveSpanLength(&effSpanLength);
   (*table)(row,1) << Sub2(_T("w"),_T("1")) << _T(" = (0.25)(") << spanLength.SetValue(effSpanLength) << _T(") = ");
   (*table)(row,1) << spanLength.SetValue(0.25*effSpanLength) << _T(" = ") << length.SetValue(0.25*effSpanLength) << rptNewLine;

   FlangeIndexType count;
   details->get_FlangeCount(&count);

   for ( FlangeIndexType flangeIdx = 0; flangeIdx < count; flangeIdx++ )
   {
      if ( 1 < count )
         (*table)(row,1) << rptNewLine << _T("Top Flange ") << long(flangeIdx + 1) << rptNewLine;

      Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
      details->get_SlabThickness(&tSlab);
      details->GetFlangeParameters(flangeIdx,&tWeb,&wFlange,&lSpacing,&rSpacing);
      (*table)(row,1) << Sub2(_T("w"),_T("2")) << _T(" = (12.0)(") << length.SetValue(tSlab) << _T(") + greater of [ ");
      (*table)(row,1) << length.SetValue(tWeb) << _T(" : (0.5)(");
      (*table)(row,1) << length.SetValue(wFlange) << _T(") ] = ");
      (*table)(row,1) << length.SetValue( 12*tSlab + _cpp_max(tWeb,0.5*wFlange) ) << rptNewLine;

      (*table)(row,1) << Sub2(_T("w"),_T("3")) << _T(" = (") << spanLength.SetValue(lSpacing*2) << _T(" + ");
      (*table)(row,1) << spanLength.SetValue(rSpacing*2) << _T(")/2 = ");
      (*table)(row,1) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
      (*table)(row,1) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
   }

   Float64 effFlangeWidth;
   details->EffectiveFlangeWidth(&effFlangeWidth);
   (*table)(row,1) << rptNewLine << _T("Effective Flange Width = ") << length.SetValue(effFlangeWidth) << rptNewLine;
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IGirder,pGirder);
   MatingSurfaceIndexType nWebs = pGirder->GetNumberOfMatingSurfaces(span,gdr);

   if ( nWebs == 1 )
      ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
   else
      ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
    GET_IFACE2(pBroker,IGirder,pGirder);
    if ( pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,gdr) )
       ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Prismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
    else
       ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Nonprismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Prismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
    GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("LRFD 4.6.2.6.1") << rptNewLine;
   *pPara << rptNewLine;


   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetExteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,       area,    pDisplayUnits->GetAreaUnit(),            true );

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_MIDSPAN);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poi = vPoi[0];
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   bool bLeftGirder = (gdr == 0 ? true : false);

   EffFlangeWidth efw;
   EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&efw);
   if ( efw.m_Details )
   {
      EffFlangeWidth adjacent_efw;
      EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder() + (bLeftGirder ? 1 : -1),poi.GetDistFromStart(),&adjacent_efw);

      Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
      Float64 effSpanLength;
      efw.m_Details->get_EffectiveSpanLength(&effSpanLength);
      efw.m_Details->get_SlabThickness(&tSlab);
      efw.m_Details->GetFlangeParameters(0,&tWeb,&wFlange,&lSpacing,&rSpacing);

      Float64 overhang = (bLeftGirder ? lSpacing : rSpacing);

      (*pPara) << _T("Effective flange width is one half the effective flange width of the adjacent interior beam, plus the least of:") << rptNewLine;
      (*pPara) << _T("One-eighth of the effective span length: ") << _T("(0.125)(") << xdim.SetValue(effSpanLength) << _T(") = "); 
      (*pPara) << xdim.SetValue(0.125*effSpanLength) << _T(" = ") << xdim2.SetValue(0.125*effSpanLength) << rptNewLine;
      (*pPara) << _T("6.0 times the average thickness of the slab, plus the greater of half the web thickness OR one-quarter the width of the top flange of the girder") << rptNewLine;
      (*pPara) << rptTab << _T("(6.0)(") << xdim2.SetValue(tSlab) << _T(") + (0.5)(");
      (*pPara) << xdim2.SetValue(tWeb) << _T(") = ");
      (*pPara) << xdim2.SetValue(6.0*tSlab + 0.5*tWeb) << rptNewLine;
      (*pPara) << rptTab << _T("(6.0)(") << xdim2.SetValue(tSlab) << _T(") + (0.25)(");
      (*pPara) << xdim2.SetValue(wFlange) << _T(") = ");
      (*pPara) << xdim2.SetValue(6.0*tSlab + 0.25*wFlange) << rptNewLine;
      (*pPara) << _T("The width of the overhang: ") << xdim.SetValue(overhang) << _T(" = ") << xdim2.SetValue(overhang) << rptNewLine;
      (*pPara) << rptNewLine;

      adjacent_efw.m_Details->get_EffectiveSpanLength(&effSpanLength);
      adjacent_efw.m_Details->get_SlabThickness(&tSlab);
      adjacent_efw.m_Details->GetFlangeParameters(0,&tWeb,&wFlange,&lSpacing,&rSpacing);
      Float64 spacing = rSpacing + lSpacing;

      (*pPara) << _T("The effective flange width for the adjacent interior girder is the least of:") << rptNewLine;
      (*pPara) << _T("One-quarter of the effective span length: ") << _T("(0.25)(") << xdim.SetValue(effSpanLength) << _T(") = "); 
      (*pPara) << xdim.SetValue(0.25*effSpanLength) << _T(" = ") << xdim2.SetValue(0.25*effSpanLength) << rptNewLine;
      (*pPara) << _T("12.0 times the average thickness of the slab, plus the greater of web thickness OR one-half the width of the top flange of the girder") << rptNewLine;
      (*pPara) << rptTab << _T("(12.0)(") << xdim2.SetValue(tSlab) << _T(") + ");
      (*pPara) << xdim2.SetValue(tWeb) << _T(" = ");
      (*pPara) << xdim2.SetValue(12.0*tSlab + tWeb) << rptNewLine;
      (*pPara) << rptTab << _T("(12.0)(") << xdim2.SetValue(tSlab) << _T(") + (0.5)(");
      (*pPara) << xdim2.SetValue(wFlange) << _T(") = ");
      (*pPara) << xdim2.SetValue(12.0*tSlab + 0.5*wFlange) << rptNewLine;
      (*pPara) << _T("The average spacing of adjacent beams: ") << xdim.SetValue(spacing) << _T(" = ") << xdim2.SetValue(spacing) << rptNewLine;

      (*pPara) << rptNewLine << _T("Effective Flange Width = ") << xdim2.SetValue(efw.effFlangeWidth) << rptNewLine;
   }
   else
   {
      if ( IsSpreadSpacing(pIBridgeDesc->GetGirderSpacingType()) )
      {
         if ( 1 < pBridge->GetGirderCount(poi.GetSpan()) )
         {
            *pPara << _T("Effective flange width is taken as one-half the distance to the adjacent girder plus the full overhang width") << rptNewLine;
            if ( bLeftGirder )
              *pPara << _T("Left Overhang from CL girder = ") << xdim.SetValue(efw.twLeft) << rptNewLine;
            else
              *pPara << _T("Left Spacing = ") << xdim.SetValue(efw.twLeft*2) << rptNewLine;

            if ( bLeftGirder )
              *pPara << _T("Right Spacing = ") << xdim.SetValue(efw.twRight*2) << rptNewLine;
            else
              *pPara << _T("Right Overhang from CL girder = ") << xdim.SetValue(efw.twRight) << rptNewLine;
         }
         else
         {
            *pPara << _T("Effective flange width is take to be the slab overhangs") << rptNewLine;
            *pPara << _T("Left Overhang from CL girder = ") << xdim.SetValue(efw.twLeft) << rptNewLine;
            *pPara << _T("Right Overhang from CL girder = ") << xdim.SetValue(efw.twRight) << rptNewLine;
         }
      }
      else
      {
         *pPara << _T("Effective flange width is taken as the girder width") << rptNewLine;
      }

      if ( efw.bContinuousBarrier )
      {
         *pPara << _T("Where a structurally continuous concrete barrier is present and is included in the structural analysis as permited in Article 4.5.1 the deck slab overhang width used for the analysis as well as for checking the composite girder resistance may be extended by: ")
            << symbol(DELTA) << _T("w = ") << Sub2(_T("A"),_T("b")) << _T("/(2") << Sub2(_T("t"),_T("s")) << _T(")") << rptNewLine;
         *pPara << Sub2(_T("A"),_T("b")) << _T(" = ") << area.SetValue(efw.Ab) << rptNewLine;
         *pPara << Sub2(_T("t"),_T("s")) << _T(" = ") << xdim2.SetValue(efw.ts) << rptNewLine;
      }

      *pPara << _T("Effective Flange Width = ") << xdim.SetValue(efw.effFlangeWidth) << _T(" = ") << xdim2.SetValue(efw.effFlangeWidth) << rptNewLine;
   }
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Nonprismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("LRFD 4.6.2.6.1") << rptNewLine;

   *pPara << rptNewLine;

   bool bLeftGirder = (gdr == 0 ? true : false);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool use_tributary_width = DoUseTributaryWidth( pBridgeDesc);


   if ( use_tributary_width )
   {
       *pPara << _T("Effective flange width is taken as one-half the distance to the adjacent girder plus the full overhang width") << rptNewLine;
   }
   else
   {
      *pPara << _T("Effective flange width is one-half the effective width of the adjacent interior beam, plus the least of: ") << rptNewLine;
      *pPara << symbol(DOT) << _T(" One-eights of the effective span length (") << Sub2(_T("w"),_T("1")) << _T(")") << rptNewLine;
      *pPara << symbol(DOT) << _T(" 6 times the average thickness of the slab, plus the greater of one-half the web thickness");
      *pPara << _T(" OR one-quarter the width of the top flange of the girder (") << Sub2(_T("w"),_T("2")) << _T(")") << rptNewLine;
      *pPara << symbol(DOT) << _T(" The width of the overhang (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   }
   *pPara << rptNewLine;


   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetExteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   rptRcTable* table;
   if ( use_tributary_width )
   {
      EffFlangeWidth efw;
      EffectiveFlangeWidthDetails(bridge,span,gdr,0.00,&efw);

      int nCol = 4;

      if ( efw.bContinuousBarrier )
      {
          *pPara << _T("Where a structurally continuous concrete barrier is present and is included in the structural analysis as permited in Article 4.5.1 the deck slab overhang width used for the analysis as well as for checking the composite girder resistance may be extended by: ")
                 << symbol(DELTA) << _T("w = ") << Sub2(_T("A"),_T("b")) << _T("/(2") << Sub2(_T("t"),_T("s")) << _T(")") << rptNewLine;

          nCol += 2;
      }

      table = pgsReportStyleHolder::CreateDefaultTable(nCol,_T(""));

      int col = 0;
      (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,col++) << COLHDR((bLeftGirder ? _T("Left Overhang") : _T("Left Spacing")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*table)(0,col++) << COLHDR((bLeftGirder ? _T("Right Spacing") : _T("Right Overhang")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if ( efw.bContinuousBarrier )
      {
         (*table)(0,col++) << COLHDR(Sub2(_T("A"),_T("b")),rptAreaUnitTag,pDisplayUnits->GetAreaUnit() );
         (*table)(0,col++) << COLHDR(Sub2(_T("t"),_T("s")),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit() );
      }

      (*table)(0,col++) << COLHDR(_T("Effective Flange Width"),rptLengthUnitTag,pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      table = pgsReportStyleHolder::CreateDefaultTable(3,_T(""));

      (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*table)(0,1) << _T("Adjacent Interior Beam");
      (*table)(0,2) << _T("This Beam");

      table->SetColumnWidth(1,5.0);
      table->SetColumnWidth(2,5.0);
   }
   *pPara << table;

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,   pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spanLength, pDisplayUnits->GetSpanLengthUnit(),   true  );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,     pDisplayUnits->GetComponentDimUnit(), true  );

   INIT_UV_PROTOTYPE( rptLengthUnitValue, xdim, pDisplayUnits->GetSpanLengthUnit(),   true  );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, xdim2,     pDisplayUnits->GetComponentDimUnit(), true  );
   INIT_UV_PROTOTYPE( rptAreaUnitValue, area,     pDisplayUnits->GetAreaUnit(), true  );

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_TABULAR);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      EffFlangeWidth efw;
      EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&efw);

      int col = 0;
      (*table)(row,col++) << location.SetValue( pgsTypes::BridgeSite2, poi, end_size );
      if ( !use_tributary_width )
      {
         ATLASSERT(efw.m_Details);

         EffFlangeWidth adjacent_efw;
         EffectiveFlangeWidthDetails(bridge,poi.GetSpan(),poi.GetGirder() + (bLeftGirder ? 1 : -1),poi.GetDistFromStart(),&adjacent_efw);

         ReportEffectiveFlangeWidth_InteriorGirderRow(adjacent_efw.m_Details,row,table,pDisplayUnits);
         col++;

         Float64 effSpanLength;
         efw.m_Details->get_EffectiveSpanLength(&effSpanLength);
         (*table)(row,col) << Sub2(_T("w"),_T("1")) << _T(" = (0.125)(") << spanLength.SetValue(effSpanLength) << _T(") = ");
         (*table)(row,col) << spanLength.SetValue(0.125*effSpanLength) << _T(" = ") << length.SetValue(0.125*effSpanLength) << rptNewLine;

         FlangeIndexType count;
         efw.m_Details->get_FlangeCount(&count);

         for ( FlangeIndexType flangeIdx = 0; flangeIdx < count; flangeIdx++ )
         {
            if ( 1 < count )
               (*table)(row,col) << rptNewLine << _T("Top Flange ") << long(flangeIdx + 1) << rptNewLine;

            Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
            efw.m_Details->get_SlabThickness(&tSlab);
            efw.m_Details->GetFlangeParameters(flangeIdx,&tWeb,&wFlange,&lSpacing,&rSpacing);
            (*table)(row,col) << Sub2(_T("w"),_T("2")) << _T(" = (6.0)(") << length.SetValue(tSlab) << _T(") + greater of [ (0.5)(");
            (*table)(row,col) << length.SetValue(tWeb) << _T(") : (0.25)(");
            (*table)(row,col) << length.SetValue(wFlange) << _T(") ] = ");
            (*table)(row,col) << length.SetValue( 6*tSlab + _cpp_max(0.5*tWeb,0.25*wFlange) ) << rptNewLine;

            Float64 overhang = (bLeftGirder ? lSpacing : rSpacing);
            (*table)(row,col) << Sub2(_T("w"),_T("3")) << _T(" = ") << spanLength.SetValue(overhang);
            (*table)(row,col) << _T(" = ") << length.SetValue(overhang) << rptNewLine;
         }

         (*table)(row,col) << rptNewLine << _T("Effective Flange Width = ") << length.SetValue(efw.effFlangeWidth) << rptNewLine;
      }
      else
      {
         xdim2.ShowUnitTag(false);
         area.ShowUnitTag(false);

         if ( bLeftGirder )
           (*table)(row,col++) << xdim2.SetValue(efw.twLeft) << rptNewLine;
         else
           (*table)(row,col++) << xdim2.SetValue(efw.twLeft*2) << rptNewLine;

         if ( bLeftGirder )
           (*table)(row,col++) << xdim2.SetValue(efw.twRight*2) << rptNewLine;
         else
           (*table)(row,col++) << xdim2.SetValue(efw.twRight) << rptNewLine;

         if ( efw.bContinuousBarrier )
         {
            (*table)(row,col++) << area.SetValue(efw.Ab) << rptNewLine;
            (*table)(row,col++) << xdim2.SetValue(efw.ts) << rptNewLine;
         }

         (*table)(row,col++) << xdim2.SetValue(efw.effFlangeWidth) << rptNewLine;
      }

      row++;
   }
}
void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
    GET_IFACE2(pBroker,IGirder,pGirder);
    if ( pGirder->IsPrismatic(pgsTypes::BridgeSite3,span,gdr) )
       ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange_Prismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
    else
       ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange_Nonprismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange_Prismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool use_tributary_width = DoUseTributaryWidth( pBridgeDesc);

   if ( use_tributary_width )
   {
      ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Prismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
      return;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("LRFD 4.6.2.6.1 and  C4.6.2.6.1") << rptNewLine;
   *pPara << _T("For open boxes, the effective flange width of each web should be determined as though each web was an individual supporting element.") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Effective flange width is the least of: ") << rptNewLine;
   *pPara << symbol(DOT) << _T(" One-quarter of the effective span length (") << Sub2(_T("w"),_T("1")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" 12 times the average thickness of the slab, plus the greater of the web thickness");
   *pPara << _T(" OR one-half the width of the top flange of the girder (") << Sub2(_T("w"),_T("2")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" The average spacing of adjacent webs for interior webs (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" The slab overhang plus one-half of the spacing to the adjacent interor web for exterior webs (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   *pPara << rptNewLine;
   
   *pPara << Sub2(_T("w"),_T("2")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("2")) << _T(" for each web") << rptNewLine;
   *pPara << Sub2(_T("w"),_T("3")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("3")) << _T(" for each web") << rptNewLine << rptNewLine;


   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetExteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,   pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spanLength, pDisplayUnits->GetSpanLengthUnit(),   true  );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,     pDisplayUnits->GetComponentDimUnit(), true  );

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_MIDSPAN);
   ATLASSERT(vPoi.size() == 1);
   pgsPointOfInterest poi = vPoi[0];
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   bool bLeftGirder = (gdr == 0 ? true : false);
   CComPtr<IEffectiveFlangeWidthDetails> details;
   EffectiveFlangeWidthEx(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&details);

   Float64 effSpanLength;
   details->get_EffectiveSpanLength(&effSpanLength);

   FlangeIndexType count;
   details->get_FlangeCount(&count);

   for ( FlangeIndexType flangeIdx = 0; flangeIdx < count; flangeIdx++ )
   {
      Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
      details->get_SlabThickness(&tSlab);
      details->GetFlangeParameters(flangeIdx,&tWeb,&wFlange,&lSpacing,&rSpacing);

      if ( 1 < count )
         (*pPara) << rptNewLine << _T("Top Flange ") << long(flangeIdx + 1) << rptNewLine;

      (*pPara) << Sub2(_T("w"),_T("2")) << _T(" = (12.0)(") << length.SetValue(tSlab) << _T(") + greater of [ ");
      (*pPara) << length.SetValue(tWeb) << _T(" : (0.5)(");
      (*pPara) << length.SetValue(wFlange) << _T(") ] = ");
      (*pPara) << length.SetValue( 12*tSlab + _cpp_max(tWeb,0.5*wFlange) ) << rptNewLine;

      if ( bLeftGirder && flangeIdx == 0 )
        {
        // left exterior web 
        (*pPara) << Sub2(_T("w"),_T("3")) << _T(" = ") << spanLength.SetValue(lSpacing) << _T(" + ");
        (*pPara) << spanLength.SetValue(rSpacing*2) << _T("/2 = ");
        (*pPara) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
        (*pPara) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
        }
        else if ( !bLeftGirder && flangeIdx == count-1 )
        {
        // right exterior web 
        (*pPara) << Sub2(_T("w"),_T("3")) << _T(" = ") << spanLength.SetValue(lSpacing*2) << _T("/2 + ");
        (*pPara) << spanLength.SetValue(rSpacing) << _T(" = ");
        (*pPara) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
        (*pPara) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
        }
        else
        {
        (*pPara) << Sub2(_T("w"),_T("3")) << _T(" = (") << spanLength.SetValue(lSpacing*2) << _T(" + ");
        (*pPara) << spanLength.SetValue(rSpacing*2) << _T(")/2 = ");
        (*pPara) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
        (*pPara) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
        }
   }

      Float64 effFlangeWidth;
      details->EffectiveFlangeWidth(&effFlangeWidth);
      (*pPara) << rptNewLine << _T("Effective Flange Width = ") << length.SetValue(effFlangeWidth) << rptNewLine;
}

void CEffectiveFlangeWidthTool::ReportEffectiveFlangeWidth_ExteriorGirder_MultiTopFlange_Nonprismatic(IBroker* pBroker,IGenericBridge* bridge,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   bool use_tributary_width = DoUseTributaryWidth( pBridgeDesc);

   if ( use_tributary_width )
   {
      ReportEffectiveFlangeWidth_ExteriorGirder_SingleTopFlange_Nonprismatic(pBroker,bridge,span,gdr,pChapter,pDisplayUnits);
      return;
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   *pPara << _T("LRFD 4.6.2.6.1 and  C4.6.2.6.1") << rptNewLine;
   *pPara << _T("For open boxes, the effective flange width of each web should be determined as though each web was an individual supporting element.") << rptNewLine;
   *pPara << rptNewLine;
   *pPara << _T("Effective flange width is the least of: ") << rptNewLine;
   *pPara << symbol(DOT) << _T(" One-quarter of the effective span length (") << Sub2(_T("w"),_T("1")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" 12 times the average thickness of the slab, plus the greater of the web thickness");
   *pPara << _T(" OR one-half the width of the top flange of the girder (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" The average spacing of adjacent webs for interior webs (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   *pPara << symbol(DOT) << _T(" The slab overhang plus one-half of the spacing to the adjacent interor web for exterior webs (") << Sub2(_T("w"),_T("3")) << _T(")") << rptNewLine;
   *pPara << rptNewLine;
   
   *pPara << Sub2(_T("w"),_T("2")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("2")) << _T(" for each web") << rptNewLine;
   *pPara << Sub2(_T("w"),_T("3")) << _T(" = ") << symbol(SUM) << Sub2(_T("w"),_T("3")) << _T(" for each web") << rptNewLine << rptNewLine;


   CComPtr<IBeamFactory> factory;
   GetBeamFactory(pBroker,span,gdr,&factory);
   std::_tstring strImage = factory->GetExteriorGirderEffectiveFlangeWidthImage(pBroker,pBridge->GetDeckType());

   *pPara << rptRcImage(strImagePath + strImage) << rptNewLine;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(2,_T(""));
   *pPara << table;


   (*table)(0,0) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,1) << _T("");

   table->SetColumnWidth(1,5.0);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location,   pDisplayUnits->GetSpanLengthUnit(),   false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, spanLength, pDisplayUnits->GetSpanLengthUnit(),   true  );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,     pDisplayUnits->GetComponentDimUnit(), true  );

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr, pgsTypes::BridgeSite2,POI_TABULAR);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   bool bLeftGirder = (gdr == 0 ? true : false);

   RowIndexType row = table->GetNumberOfHeaderRows();
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
   
      CComPtr<IEffectiveFlangeWidthDetails> details;
      EffectiveFlangeWidthEx(bridge,poi.GetSpan(),poi.GetGirder(),poi.GetDistFromStart(),&details);

      (*table)(row,0) << location.SetValue( pgsTypes::BridgeSite2, poi, end_size );

      Float64 effSpanLength;
      details->get_EffectiveSpanLength(&effSpanLength);
      (*table)(row,1) << Sub2(_T("w"),_T("1")) << _T(" = (0.25)(") << spanLength.SetValue(effSpanLength) << _T(") = ");
      (*table)(row,1) << spanLength.SetValue(0.25*effSpanLength) << _T(" = ") << length.SetValue(0.25*effSpanLength) << rptNewLine;

      FlangeIndexType count;
      details->get_FlangeCount(&count);

      for ( FlangeIndexType flangeIdx = 0; flangeIdx < count; flangeIdx++ )
      {
         if ( 1 < count )
            (*table)(row,1) << rptNewLine << _T("Web ") << long(flangeIdx + 1) << rptNewLine;

         Float64 tSlab, tWeb, wFlange, lSpacing, rSpacing;
         details->get_SlabThickness(&tSlab);
         details->GetFlangeParameters(flangeIdx,&tWeb,&wFlange,&lSpacing,&rSpacing);
         (*table)(row,1) << Sub2(_T("w"),_T("2")) << _T(" = (12.0)(") << length.SetValue(tSlab) << _T(") + greater of [ ");
         (*table)(row,1) << length.SetValue(tWeb) << _T(" : (0.5)(");
         (*table)(row,1) << length.SetValue(wFlange) << _T(") ] = ");
         (*table)(row,1) << length.SetValue( 12*tSlab + _cpp_max(tWeb,0.5*wFlange) ) << rptNewLine;

         if ( bLeftGirder && flangeIdx == 0 )
         {
            // left exterior web 
            (*table)(row,1) << Sub2(_T("w"),_T("3")) << _T(" = ") << spanLength.SetValue(lSpacing) << _T(" + ");
            (*table)(row,1) << spanLength.SetValue(rSpacing*2) << _T("/2 = ");
            (*table)(row,1) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
            (*table)(row,1) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
         }
         else if ( !bLeftGirder && flangeIdx == count-1 )
         {
            // right exterior web 
            (*table)(row,1) << Sub2(_T("w"),_T("3")) << _T(" = ") << spanLength.SetValue(lSpacing*2) << _T("/2 + ");
            (*table)(row,1) << spanLength.SetValue(rSpacing) << _T(" = ");
            (*table)(row,1) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
            (*table)(row,1) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
         }
         else
         {
            (*table)(row,1) << Sub2(_T("w"),_T("3")) << _T(" = (") << spanLength.SetValue(lSpacing*2) << _T(" + ");
            (*table)(row,1) << spanLength.SetValue(rSpacing*2) << _T(")/2 = ");
            (*table)(row,1) << spanLength.SetValue(lSpacing+rSpacing); // left and right spacing already halved, but present it as if it isn't
            (*table)(row,1) << _T(" = ") << length.SetValue(lSpacing+rSpacing) << rptNewLine;
         }
      }

      Float64 effFlangeWidth;
      details->EffectiveFlangeWidth(&effFlangeWidth);
      (*table)(row,1) << rptNewLine << _T("Effective Flange Width = ") << length.SetValue(effFlangeWidth) << rptNewLine;

      row++;
   }
}

void CEffectiveFlangeWidthTool::GetBeamFactory(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,IBeamFactory** factory)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGdrEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   pGdrEntry->GetBeamFactory(factory);
}

bool CEffectiveFlangeWidthTool::DoUseTributaryWidth(const CBridgeDescription* pBridgeDesc)
{
   pgsTypes::SupportedBeamSpacing beamSpacing = pBridgeDesc->GetGirderSpacingType();

   return m_bUseTribWidth || IsAdjacentSpacing(beamSpacing) || lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion();
}
