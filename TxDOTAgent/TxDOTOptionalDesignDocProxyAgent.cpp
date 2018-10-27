///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include "TxDOTOptionalDesignDocProxyAgent.h"
#include "TxDOTOptionalDesignDoc.h"
#include "TxDOTAppPlugin.h"
#include "TxDOTOptionalDesignUtilities.h"

#include "resource.h"

#include <MFCTools\VersionInfo.h>

#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Allowables.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CTxDOTOptionalDesignDocProxyAgent
****************************************************************************/

CTxDOTOptionalDesignDocProxyAgent::CTxDOTOptionalDesignDocProxyAgent():
m_NeedValidate(true),
m_GirderArtifact(CSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0))
{
   m_pTxDOTOptionalDesignDoc = nullptr;
}

CTxDOTOptionalDesignDocProxyAgent::~CTxDOTOptionalDesignDocProxyAgent()
{
}

void CTxDOTOptionalDesignDocProxyAgent::SetDocument(CTxDOTOptionalDesignDoc* pDoc)
{
   m_pTxDOTOptionalDesignDoc = pDoc;

   pDoc->m_ProjectData.Attach(this);
}

//////////////////////////////////////////////////////////
// IAgentEx
STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_IUpdateTemplates, this );
   pBrokerInit->RegInterface( IID_ISelection,       this );
   pBrokerInit->RegInterface( IID_IDocumentType,    this );
   pBrokerInit->RegInterface( IID_IVersionInfo,     this );
   pBrokerInit->RegInterface( IID_IGetTogaData,     this );
   pBrokerInit->RegInterface( IID_IGetTogaResults,  this );
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Init()
{
//   EAF_AGENT_INIT;

   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Init2()
{
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::Reset()
{
   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
//   CLOSE_LOGFILE;

   return S_OK;
}

STDMETHODIMP CTxDOTOptionalDesignDocProxyAgent::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_TxDOTOptionalDesignDocProxyAgent;
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////
// IUpdateTemplates
bool CTxDOTOptionalDesignDocProxyAgent::UpdatingTemplates()
{
   // By returning true here we are forcing local library entries to be overwritten by
   // master library entries if there is a conflict. 
   // This behavior is by design so if a library update occurs, new data will be used.
   return true;
}

////////////////////////////////////////////////////////////////////
// ISelection

// NOTE: Not really utilizing this interface - just appealing functions that need it.
CSelection CTxDOTOptionalDesignDocProxyAgent::GetSelection()
{
   CSelection selection;
   selection.Type = CSelection::Girder;
   selection.GroupIdx = TOGA_SPAN;
   selection.GirderIdx = TOGA_FABR_GDR;
   return selection;
}

void CTxDOTOptionalDesignDocProxyAgent::ClearSelection()
{
   ATLASSERT(false); // not using this method
}

PierIndexType CTxDOTOptionalDesignDocProxyAgent::GetSelectedPier()
{
   return TOGA_SPAN;
}

SpanIndexType CTxDOTOptionalDesignDocProxyAgent::GetSelectedSpan()
{
   return TOGA_SPAN;
}

CGirderKey CTxDOTOptionalDesignDocProxyAgent::GetSelectedGirder()
{
   return CGirderKey(TOGA_SPAN,TOGA_FABR_GDR);
}

CSegmentKey CTxDOTOptionalDesignDocProxyAgent::GetSelectedSegment()
{
   ATLASSERT(false); // not using this method
   return CSegmentKey();
}

CClosureKey CTxDOTOptionalDesignDocProxyAgent::GetSelectedClosureJoint()
{
   ATLASSERT(false); // not using this method
   return CClosureKey();
}

SupportIDType CTxDOTOptionalDesignDocProxyAgent::GetSelectedTemporarySupport()
{
   ATLASSERT(false); // not using this method
   return INVALID_ID;
}

bool CTxDOTOptionalDesignDocProxyAgent::IsDeckSelected()
{
   ATLASSERT(false); // not using this method
   return false;
}

bool CTxDOTOptionalDesignDocProxyAgent::IsAlignmentSelected()
{
   ATLASSERT(false); // not using this method
   return false;
}

bool CTxDOTOptionalDesignDocProxyAgent::IsRailingSystemSelected(pgsTypes::TrafficBarrierOrientation orientation)
{
   ATLASSERT(false); // not using this method
   return false;
}

void CTxDOTOptionalDesignDocProxyAgent::SelectPier(PierIndexType pierIdx)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectSpan(SpanIndexType spanIdx)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectGirder(const CGirderKey& girderKey)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectSegment(const CSegmentKey& segmentKey)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectClosureJoint(const CClosureKey& closureKey)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectTemporarySupport(SupportIDType tsID)
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectDeck()
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectAlignment()
{
   ATLASSERT(false); // not using this method
}

void CTxDOTOptionalDesignDocProxyAgent::SelectRailingSystem(pgsTypes::TrafficBarrierOrientation orientation)
{
   ATLASSERT(false); // not using this method
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetSectionCutStation()
{
   ASSERT(false); // should never get here
   return -999999;
}

////////////////////////////////////////////////////////////////////
// IVersionInfo
CString CTxDOTOptionalDesignDocProxyAgent::GetVersionString(bool bIncludeBuildNumber)
{
   CString str(_T("Version "));
   str += GetVersion(bIncludeBuildNumber);
#if defined _BETA_VERSION
   str += CString(_T(" BETA"));
#endif

   str += CString(_T(" - Built on "));
   str += CString(__DATE__);
   return str;
}

CString CTxDOTOptionalDesignDocProxyAgent::GetVersion(bool bIncludeBuildNumber)
{
   // This plug-in is like an application... we want the version information for this DLL
   // set the module state to this DLL
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CWinApp* pApp = AfxGetApp();
   CString strExe( pApp->m_pszExeName );
   strExe += _T(".dll");

   CVersionInfo verInfo;
   verInfo.Load(strExe);
   
   CString strVersion = verInfo.GetProductVersionAsString();

#if defined _DEBUG || defined _BETA_VERSION
   // always include the build number in debug and beta versions
   bIncludeBuildNumber = true;
#endif

   if (!bIncludeBuildNumber)
   {
      // remove the build number
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
   }

   return strVersion;
}

///////////////////////////////////////////////////////////////////
// IGetTogaData
const CTxDOTOptionalDesignData* CTxDOTOptionalDesignDocProxyAgent::GetTogaData()
{
   return &(this->m_pTxDOTOptionalDesignDoc->m_ProjectData);
}

///////////////////////////////////////////////////////////////////
// IGetTogaResults
void CTxDOTOptionalDesignDocProxyAgent::GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)
{
   Validate();

   *pStress        = m_CtrlTensileStress;
   *pDistFromStart = m_CtrlTensileStressLocation;
   *pStressFactor  = m_CtrlTensileStressFactor;
}

void CTxDOTOptionalDesignDocProxyAgent::GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)
{
   Validate();

   *pStress        = m_CtrlCompressiveStress;
   *pDistFromStart = m_CtrlCompressiveStressLocation;
   *pStressFactor  = m_CtrlCompressiveStressFactor;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredUltimateMoment()
{
   Validate();

   return m_RequiredUltimateMoment;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetUltimateMomentCapacity()
{
   Validate();

   return m_UltimateMomentCapacity;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetMaximumCamber()
{
   Validate();
   
   return m_MaximumCamber;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetFabricatorMaximumCamber()
{
   Validate();
   
   return m_FabricatorMaximumCamber;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredFc()
{
   Validate();

   return m_RequiredFc;
}

bool CTxDOTOptionalDesignDocProxyAgent::ShearPassed()
{
   Validate();

   return m_ShearPassed;
}

Float64 CTxDOTOptionalDesignDocProxyAgent::GetRequiredFci()
{
   Validate();

   return m_RequiredFci;
}

const pgsGirderArtifact* CTxDOTOptionalDesignDocProxyAgent::GetFabricatorDesignArtifact()
{
   Validate();

   return &m_GirderArtifact;
}

void CTxDOTOptionalDesignDocProxyAgent::OnTxDotDataChanged(int change)
{
   // any change type affects us
   m_NeedValidate = true;
}

void CTxDOTOptionalDesignDocProxyAgent::Validate()
{
   if ( m_NeedValidate )
   {
      CSegmentKey origSegmentKey(TOGA_SPAN,TOGA_ORIG_GDR,0);
      CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

      // build model
      IBroker* pBroker = this->m_pTxDOTOptionalDesignDoc->GetUpdatedBroker();

      GET_IFACE2(pBroker,IAllowableConcreteStress, pAllowable );

      GET_IFACE2(pBroker,IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(origSegmentKey);
      IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
      IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
    
      // Get responses from design based on original data
      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsGirderArtifact* pOriginalGdrArtifact = pIArtifact->GetGirderArtifact(origSegmentKey);
      const pgsSegmentArtifact* pOriginalSegmentArtifact = pOriginalGdrArtifact->GetSegmentArtifact(0);

      const CTxDOTOptionalDesignData* pDesignData = GetTogaData();

      // Our model is always prismatic - max's will occur at mid-span poi
      GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
      PoiList vPOI;
      pIPoi->GetPointsOfInterest(origSegmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPOI);
      ATLASSERT( vPOI.size() == 1 );
      const pgsPointOfInterest& orig_ms_poi = vPOI.front();

      // Find max compressive stress and compute stress factor from original model
      const pgsFlexuralStressArtifact* pOriginalStressArtifact;
      pOriginalStressArtifact = pOriginalSegmentArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression,orig_ms_poi.GetID());

      Float64 fTop = pOriginalStressArtifact->GetExternalEffects(pgsTypes::TopGirder);

      m_CtrlCompressiveStress = fTop;
      m_CtrlCompressiveStressLocation = orig_ms_poi.GetDistFromStart();

      Float64 ft_des = -1.0 * pDesignData->GetFt(); // opposite sign convention
      m_CtrlCompressiveStressFactor = ft_des/m_CtrlCompressiveStress;

      // Tensile stress factor
      pOriginalStressArtifact = pOriginalSegmentArtifact->GetFlexuralStressArtifactAtPoi( liveLoadIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension,orig_ms_poi.GetID());
      Float64 fBot = pOriginalStressArtifact->GetExternalEffects(pgsTypes::BottomGirder);

      m_CtrlTensileStress = fBot;
      m_CtrlTensileStressLocation = orig_ms_poi.GetDistFromStart();

      Float64 fb_des = -1.0 * pDesignData->GetFb(); // opposite sign convention
      m_CtrlTensileStressFactor = fb_des/m_CtrlTensileStress;

      // Camber from original model
      GET_IFACE2(pBroker,ICamber,pCamber);
      m_MaximumCamber = pCamber->GetDCamberForGirderScheduleUnfactored(orig_ms_poi,CREEP_MAXTIME);

      // Now we need results from fabricator model
      // =========================================
      const pgsGirderArtifact* pFabricatorGdrArtifact = pIArtifact->GetGirderArtifact(fabrSegmentKey);

      // Create a copy of PGSuper's artifact and factor stresses in our new copy
      m_GirderArtifact = *pFabricatorGdrArtifact;

      // List of possible stress checks
      const int num_cases = 7;
      IntervalIndexType intervals[num_cases];
      intervals[0] = castDeckIntervalIdx;
      intervals[1] = castDeckIntervalIdx;
      intervals[2] = compositeDeckIntervalIdx;
      intervals[3] = liveLoadIntervalIdx;
      intervals[4] = liveLoadIntervalIdx;
      intervals[5] = liveLoadIntervalIdx;
      intervals[6] = liveLoadIntervalIdx;

      pgsTypes::LimitState lstates[num_cases] = {pgsTypes::ServiceI,   pgsTypes::ServiceI,   pgsTypes::ServiceI,    pgsTypes::ServiceI   , pgsTypes::ServiceIII,  pgsTypes::FatigueI,    pgsTypes::ServiceIA};
      pgsTypes::StressType ststype[num_cases] = {pgsTypes::Tension,    pgsTypes::Compression,pgsTypes::Compression, pgsTypes::Compression, pgsTypes::Tension,     pgsTypes::Compression, pgsTypes::Compression};

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         lstates[num_cases-1] = pgsTypes::FatigueI;

      // Loop over all pois and limit states and factor results in our copy
      pgsFlexuralStressArtifact* pFabrStressArtifact;

      for (int icase=0; icase<num_cases; icase++)
      {
         vPOI.clear(); // recycle list
         pIPoi->GetPointsOfInterest(fabrSegmentKey,&vPOI);
         ATLASSERT(vPOI.size()>0);

         if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && lstates[icase] == pgsTypes::FatigueI) || 
              (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& lstates[icase] == pgsTypes::ServiceIA)
              )
         {
            // if before LRFD 2009 and Fatigue I 
            // - OR -
            // LRFD 2009 and later and Service IA
            //
            // ... don't evaluate this case
            continue;
         }

         Float64 k;
         if (lstates[icase] == pgsTypes::ServiceIA || lstates[icase] == pgsTypes::FatigueI )
            k = 0.5; // Use half prestress stress if service IA  (See Tbl 5.9.4.2.1-1 2008 or before) or Fatigue I (LRFD 5.5.3.1 2009)
         else
            k = 1.0;

         CollectionIndexType nArtifacts = m_GirderArtifact.GetSegmentArtifact(0)->GetFlexuralStressArtifactCount( intervals[icase],lstates[icase],ststype[icase]);
         for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++) 
         {
            pFabrStressArtifact = m_GirderArtifact.GetSegmentArtifact(0)->GetFlexuralStressArtifact( intervals[icase],lstates[icase],ststype[icase],idx );
            const pgsPointOfInterest& poi(pFabrStressArtifact->GetPointOfInterest());

            // factor external stresses
            Float64 fTopExt = pFabrStressArtifact->GetExternalEffects(pgsTypes::TopGirder);
            Float64 fBotExt = pFabrStressArtifact->GetExternalEffects(pgsTypes::BottomGirder);

            fTopExt *= m_CtrlCompressiveStressFactor;
            fBotExt *= m_CtrlTensileStressFactor;

            pFabrStressArtifact->SetExternalEffects( pgsTypes::TopGirder,    fTopExt);
            pFabrStressArtifact->SetExternalEffects( pgsTypes::BottomGirder, fBotExt);

            // recompute demand
            Float64 fTopPs = pFabrStressArtifact->GetPretensionEffects(pgsTypes::TopGirder);
            Float64 fBotPs = pFabrStressArtifact->GetPretensionEffects(pgsTypes::BottomGirder);

            fTop = k*fTopPs + fTopExt;
            fBot = k*fBotPs + fBotExt;

            pFabrStressArtifact->SetDemand(pgsTypes::TopGirder,   fTop);
            pFabrStressArtifact->SetDemand(pgsTypes::BottomGirder,fBot);

            // Compute and store required concrete strength
            if ( ststype[icase] == pgsTypes::Compression )
            {
               Float64 c = pAllowable->GetSegmentAllowableCompressionStressCoefficient(poi,intervals[icase],lstates[icase]);
               Float64 fc_reqd = (IsZero(c) ? 0 : Min(fTop,fBot)/-c);
               
               if ( fc_reqd < 0 ) // the minimum stress is tensile so compression isn't an issue
                  fc_reqd = 0;

               if ( MinIndex(fTop,fBot) == 0 )
               {
                  pFabrStressArtifact->SetRequiredConcreteStrength(pgsTypes::TopGirder,fc_reqd);
               }
               else
               {
                  pFabrStressArtifact->SetRequiredConcreteStrength(pgsTypes::BottomGirder,fc_reqd);
               }
            }
            else
            {
               Float64 t;
               bool bCheckMax;
               Float64 fmax;

               pAllowable->GetAllowableTensionStressCoefficient(poi,pgsTypes::TopGirder,intervals[icase],lstates[icase],false/*without rebar*/,false,&t,&bCheckMax,&fmax);

               // if this is bridge site 3, only look at the bottom stress (stress in the precompressed tensile zone)
               // otherwise, take the controlling tension
               Float64 f = (intervals[icase] == liveLoadIntervalIdx ? fBot : Max(fTop,fBot));

               Float64 fc_reqd;
               if (f>0.0)
               {
                  fc_reqd = (IsZero(t) ? 0 : BinarySign(f)*pow(f/t,2));
               }
               else
               {
                  // the maximum stress is compressive so tension isn't an issue
                  fc_reqd = 0;
               }

               if ( bCheckMax &&                  // allowable stress is limited -AND-
                    (0 < fc_reqd) &&              // there is a concrete strength that might work -AND-
                    (pow(fmax/t,2) < fc_reqd) )   // that strength will exceed the max limit on allowable
               {
                  // too bad... this isn't going to work
                  fc_reqd = -1;
               }

               if ( MaxIndex(fTop,fBot) == 0 )
               {
                  pFabrStressArtifact->SetRequiredConcreteStrength(pgsTypes::TopGirder,fc_reqd);
               }
               else
               {
                  pFabrStressArtifact->SetRequiredConcreteStrength(pgsTypes::BottomGirder,fc_reqd);
               }
            }
         }
      }
   
      // mid span in fab model
      vPOI.clear(); // recycle list
      pIPoi->GetPointsOfInterest(fabrSegmentKey, POI_5L | POI_ERECTED_SEGMENT,&vPOI);
      ATLASSERT( vPOI.size() == 1 );
      pgsPointOfInterest fabr_ms_poi = vPOI.front();

      // Ultimate Moment
      const pgsFlexuralCapacityArtifact* pFabCap;
      pFabCap = m_GirderArtifact.FindPositiveMomentFlexuralCapacityArtifact(liveLoadIntervalIdx,pgsTypes::StrengthI,fabr_ms_poi);
      ATLASSERT(pFabCap != nullptr);

      m_UltimateMomentCapacity = pFabCap->GetCapacity();

      // Required from original model
      const pgsFlexuralCapacityArtifact* pOrigCap;
      pOrigCap = pOriginalGdrArtifact->FindPositiveMomentFlexuralCapacityArtifact(liveLoadIntervalIdx,pgsTypes::StrengthI,orig_ms_poi);
      ATLASSERT(pOrigCap != nullptr);

      m_RequiredUltimateMoment = Max(pOrigCap->GetDemand(),pOrigCap->GetMinCapacity());

      // Required concrete strengths - fabricator model
      m_RequiredFci =  Max(m_GirderArtifact.GetRequiredReleaseStrength(),        ::ConvertToSysUnits(4.0, unitMeasure::KSI));
      m_RequiredFc  =  Max(m_GirderArtifact.GetRequiredGirderConcreteStrength(), ::ConvertToSysUnits(5.0, unitMeasure::KSI));

      // Get camber from fabricator model
      m_FabricatorMaximumCamber = pCamber->GetDCamberForGirderScheduleUnfactored(fabr_ms_poi,CREEP_MAXTIME);

      // Shear 
      CheckShear(pIPoi);

      m_NeedValidate = false;
   }
}

void CTxDOTOptionalDesignDocProxyAgent::CheckShear(IPointOfInterest* pIPoi)
{
   IBroker* pBroker = this->m_pTxDOTOptionalDesignDoc->GetUpdatedBroker();

   CSegmentKey origSegmentKey(TOGA_SPAN,TOGA_ORIG_GDR,0);
   CSegmentKey fabrSegmentKey(TOGA_SPAN,TOGA_FABR_GDR,0);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   m_ShearPassed = true; // until otherwise

   const pgsStirrupCheckArtifact *pStirrups = m_GirderArtifact.GetSegmentArtifact(0)->GetStirrupCheckArtifact();
   CollectionIndexType nArtifacts = pStirrups->GetStirrupCheckAtPoisArtifactCount(liveLoadIntervalIdx,pgsTypes::StrengthI);
   for ( CollectionIndexType idx = 0; idx < nArtifacts; idx++ )
   {
      // Only checking Strength I here. No TxDOT permit truck
      const pgsStirrupCheckAtPoisArtifact* pPoiArtifacts = pStirrups->GetStirrupCheckAtPoisArtifact( liveLoadIntervalIdx,pgsTypes::StrengthI,idx );

      const pgsPointOfInterest& poi = pPoiArtifacts->GetPointOfInterest();

      const pgsVerticalShearArtifact* pShear = pPoiArtifacts->GetVerticalShearArtifact();
      if (!pShear->Passed())
      {
         m_ShearPassed = false;
         return;
      }

      const pgsLongReinfShearArtifact* pLongReinf = pPoiArtifacts->GetLongReinfShearArtifact();
      if (!pLongReinf->Passed() )
      {
         m_ShearPassed = false;
         return;
      }

      const pgsHorizontalShearArtifact* pHor = pPoiArtifacts->GetHorizontalShearArtifact();
      if ( !pHor->Passed() )
      {
         m_ShearPassed = false;
         return;
      }

      const pgsStirrupDetailArtifact* pDet = pPoiArtifacts->GetStirrupDetailArtifact();
      if ( !pDet->Passed() )
      {
         m_ShearPassed = false;
         return;
      }
   }
}
