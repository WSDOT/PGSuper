///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderModelFactory.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\AnalysisResults.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PoiIDType pgsGirderModelFactory::ms_FemModelPoiID = 0;

// predicate compare method
bool ComparePoiLocation(const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2)
{
   if ( !poi1.GetSegmentKey().IsEqual(poi2.GetSegmentKey()) )
   {
      return false;
   }

   if ( !IsEqual(poi1.GetDistFromStart(),poi2.GetDistFromStart()) )
   {
      return false;
   }

   return true;
}

pgsGirderModelFactory::pgsGirderModelFactory(void)
{
}


pgsGirderModelFactory::~pgsGirderModelFactory(void)
{
}

void pgsGirderModelFactory::CreateGirderModel(IBroker* pBroker,                            // broker to access PGSuper data
                                 IntervalIndexType intervalIdx,               // used for looking up section properties and section transition POIs
                                 const CSegmentKey& segmentKey,               // this is the segment that the modeling is build for
                                 Float64 leftSupportLoc,                      // distance from the left end of the model to the left support location
                                 Float64 rightSupportLoc,                     // distance from the right end of the model to the right support location
                                 Float64 E,                                   // modulus of elasticity
                                 LoadCaseIDType lcidGirder,                   // load case ID that is to be used to define the girder dead load
                                 bool bModelLeftCantilever,                   // if true, the cantilever defined by leftSupportLoc is modeled
                                 bool bModelRightCantilever,                  // if true, the cantilever defined by rightSupportLoc is modeled
                                 const std::vector<pgsPointOfInterest>& vPOI, // vector of PGSuper POIs that are to be modeld in the Fem2d Model
                                 IFem2dModel** ppModel,                       // the Fem2d Model
                                 pgsPoiPairMap* pPoiMap                           // a mapping of PGSuper POIs to Fem2d POIs
                                 )
{
   // Build the model... always model the cantilevers in the geometry of the FEM model
   BuildModel(pBroker, intervalIdx, segmentKey, leftSupportLoc, rightSupportLoc, E, lcidGirder, true, true, vPOI, ppModel);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

   ApplyLoads(pBroker, segmentKey, segmentLength, leftSupportLoc, rightSupportLoc, E, lcidGirder, bModelLeftCantilever, bModelRightCantilever, vPOI, ppModel);

   ApplyPointsOfInterest(pBroker, segmentKey, leftSupportLoc, rightSupportLoc, E, lcidGirder, bModelLeftCantilever, bModelRightCantilever, vPOI, ppModel, pPoiMap);
}

void pgsGirderModelFactory::BuildModel(IBroker* pBroker,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,
                                          Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                          LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,
                                          const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel)
{
   if ( *ppModel )
   {
      (*ppModel)->Clear();
   }
   else
   {
      CComPtr<IFem2dModel> model;
      model.CoCreateInstance(CLSID_Fem2dModel);
      (*ppModel) = model;
      (*ppModel)->AddRef();
   }

   // get all the cross section changes
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> xsPOI = pPoi->GetPointsOfInterest(segmentKey,POI_SECTCHANGE);
   pPoi->RemovePointsOfInterest(xsPOI,POI_ERECTED_SEGMENT,POI_CANTILEVER);

   // sometimes we loose the released segment POI at 0L and 1.0L in the call to RemovePointsOfInterest above
   // these are key POI so include them here so we are guarenteed to have them.
   std::vector<pgsPointOfInterest> vPoi = pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE);
   ATLASSERT(vPoi.size() == 1);
   xsPOI.push_back(vPoi.front());
   vPoi = pPoi->GetPointsOfInterest(segmentKey,POI_END_FACE);
   ATLASSERT(vPoi.size() == 1);
   xsPOI.push_back(vPoi.front());


   // add support locations if there aren't already POIs at those locations
   bool bLeftSupportLoc  = true;
   bool bRightSupportLoc = true;
   std::vector<pgsPointOfInterest>::iterator xsIter(xsPOI.begin());
   std::vector<pgsPointOfInterest>::iterator xsIterEnd(xsPOI.end());
   for ( ; xsIter != xsIterEnd && (bLeftSupportLoc == true || bRightSupportLoc == true); xsIter++ )
   {
      pgsPointOfInterest& poi(*xsIter);
      if ( IsEqual(leftSupportLoc,poi.GetDistFromStart()) )
      {
         bLeftSupportLoc = false;
      }

      if ( IsEqual(rightSupportLoc,poi.GetDistFromStart()) )
      {
         bRightSupportLoc = false;
      }
   }

   if ( bLeftSupportLoc )
   {
      xsPOI.push_back(pgsPointOfInterest(segmentKey,leftSupportLoc));
   }

   if ( bRightSupportLoc )
   {
      xsPOI.push_back(pgsPointOfInterest(segmentKey,rightSupportLoc));
   }

   // sort the POI
   std::sort(xsPOI.begin(),xsPOI.end());
   xsPOI.erase(std::unique(xsPOI.begin(),xsPOI.end(),ComparePoiLocation),xsPOI.end()); // eliminate any duplicates

   // layout the joints
   JointIDType jntID = 0;
   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);
   std::vector<pgsPointOfInterest>::iterator jointIter(xsPOI.begin());
   std::vector<pgsPointOfInterest>::iterator jointIterEnd(xsPOI.end());
   for ( ; jointIter < jointIterEnd; jointIter++ )
   {
      pgsPointOfInterest& poi( *jointIter );
      Float64 Xpoi = poi.GetDistFromStart();
      if ( (!bModelLeftCantilever && (Xpoi < leftSupportLoc)) || 
           (!bModelRightCantilever && (rightSupportLoc < Xpoi)) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next joint
         continue;
      }

      CComPtr<IFem2dJoint> jnt;
      joints->Create(jntID++,Xpoi,0,&jnt);

      // set boundary conditions if this is a support joint
      if ( IsEqual(Xpoi,leftSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtFx);
         jnt->ReleaseDof(jrtMz);
      }
      else if ( IsEqual(Xpoi,rightSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtMz);
      }

      if ( poi.HasAttribute(POI_SECTCHANGE_LEFTFACE) )
      {
         // jump over the right face
         jointIter++;

         if ( jointIter == jointIterEnd )
         {
            break;
         }

         if ( IsEqual(jointIter->GetDistFromStart(),rightSupportLoc) )
         {
            ATLASSERT(jointIter->HasAttribute(POI_SECTCHANGE_RIGHTFACE));
            // the right face is at the support... that means this joint
            // has to be the right support
            jnt->Support();
            jnt->ReleaseDof(jrtMz);
         }
      }
   }

   // create members
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);

   // for consistancy with all structural analysis models, sections properties are based on the mid-span location of segments
   std::vector<pgsPointOfInterest> vMyPoi( pPoi->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_5L) );
   ATLASSERT( vMyPoi.size() == 1 );
   pgsPointOfInterest spPoi = vMyPoi.front();
   ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

   Float64 Ix = pSectProp->GetIx(intervalIdx,spPoi);
   Float64 Ag = pSectProp->GetAg(intervalIdx,spPoi);
   Float64 EI = E*Ix;
   Float64 EA = E*Ag;

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   MemberIDType mbrID = 0;
   JointIDType prevJntID = 0;
   jntID = prevJntID + 1;
   std::vector<pgsPointOfInterest>::iterator prevJointIter( xsPOI.begin() );
   jointIter = prevJointIter;
   jointIter++;
   jointIterEnd = xsPOI.end();
   for ( ; jointIter < jointIterEnd; jointIter++, prevJointIter++ )
   {
      pgsPointOfInterest& prevPoi( *prevJointIter );
      pgsPointOfInterest& poi( *jointIter );

      if ( (!bModelLeftCantilever  && (prevPoi.GetDistFromStart() < leftSupportLoc)) || 
           (!bModelRightCantilever && (rightSupportLoc <= prevPoi.GetDistFromStart())) )
      {
         // location is before or after the left/right support and we aren't modeling
         // the cantilevers... next member
         continue;
      }

      CComPtr<IFem2dMember> member;
      members->Create(mbrID++,prevJntID,jntID,EA,EI,&member);

      prevJntID++;
      jntID++;

      if ( poi.HasAttribute(POI_SECTCHANGE_LEFTFACE) )
      {
         // if we are at an abrupt section change, jump ahead
         prevJointIter++;
         jointIter++;

         if ( jointIter == jointIterEnd )
         {
            break;
         }
      }
   }
}

void pgsGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                       Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                       bool bModelLeftCantilever, bool bModelRightCantilever,const std::vector<pgsPointOfInterest>& vPOI,
                                       IFem2dModel** ppModel)
{
   // apply loads
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   (*ppModel)->get_Loadings(&loadings);
   loadings->Create(lcidGirder,&loading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   std::vector<SegmentLoad> segLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   std::vector<ClosureJointLoad> cjLoads;
   pProductLoads->GetSegmentSelfWeightLoad(segmentKey,&segLoads,&diaphLoads,&cjLoads);

   // apply girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<SegmentLoad>::iterator segLoadIter(segLoads.begin());
   std::vector<SegmentLoad>::iterator segLoadIterEnd(segLoads.end());
   for ( ; segLoadIter != segLoadIterEnd; segLoadIter++ )
   {
      SegmentLoad& segLoad = *segLoadIter;

      Float64 wStart = segLoad.wStart;
      Float64 wEnd   = segLoad.wEnd;
      Float64 start  = segLoad.StartLoc;
      Float64 end    = segLoad.EndLoc;

      if ( !bModelLeftCantilever && ::IsLT(start,leftSupportLoc) )
      {
         // this load segment begins before the left support and we are ignoring loads out there
  
         // compute load intensity at the left support
         wStart = ::LinInterp(leftSupportLoc,wStart,wEnd,end-start);
         start = leftSupportLoc;
      }

      if ( !bModelRightCantilever && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment ends after the right support and we are ignoring loads out there

         // compute load intensity at the right support
         wEnd = ::LinInterp(rightSupportLoc-start,wStart,wEnd,end-start);
         end = rightSupportLoc;
      }

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member
         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wStart,wEnd,lotMember,&distLoad);
      }
      else
      {
         // load straddles two or more members
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 w1,w2; // start and end load intensity on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            CComPtr<IFem2dMember> mbr;
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            CComPtr<IFem2dJoint> jntStart, jntEnd;
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               w1 = wStart;
               x1 = xStart;
            }
            else
            {
               w1 = ::LinInterp(xMbrStart-start,wStart,wEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               w2 = wEnd;
               x2 = xEnd;
            }
            else
            {
               w2 = ::LinInterp(xMbrEnd-start,wStart,wEnd,end-start);
               x2 = Lmbr; // end of member
            }

            if ( !IsEqual(x1,x2) )
            {
               // no need to add the laod if its length is 0
               CComPtr<IFem2dDistributedLoad> distLoad;
               distributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,w1,w2,lotMember,&distLoad);
            }
         }
      }
   }

   // apply diaphragm loads for precast diaphragms
   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);


   std::vector<DiaphragmLoad>::iterator diaLoadIter(diaphLoads.begin());
   std::vector<DiaphragmLoad>::iterator diaLoadIterEnd(diaphLoads.end());
   for ( ; diaLoadIter != diaLoadIterEnd; diaLoadIter++ )
   {
      Float64 x;
      DiaphragmLoad& diaphragmLoad = *diaLoadIter;

      mbrID = 0;
      bool bApplyLoad = false;

      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      for ( CollectionIndexType jntIdx = 1; jntIdx < nJoints; jntIdx++, mbrID++ )
      {
         CComPtr<IFem2dJoint> prevJoint, nextJoint;
         joints->get_Item(jntIdx-1,&prevJoint);
         joints->get_Item(jntIdx,&nextJoint);

         Float64 xPrev, xNext;
         prevJoint->get_X(&xPrev);
         nextJoint->get_X(&xNext);

         if ((!bModelLeftCantilever  && ::IsLT(xPrev,leftSupportLoc) ) ||
             (!bModelRightCantilever && ::IsLT(rightSupportLoc,xPrev) ) )
         {
            // location is before or after the left/right support and we arn't modeling
            // the cantilevers... next member
            continue;
         }

         if ( InRange(xPrev,diaphragmLoad.Loc,xNext) )
         {
            x = diaphragmLoad.Loc - xPrev;
            bApplyLoad = true;
            break;
         }
      }

      if ( bApplyLoad )
      {
         CComPtr<IFem2dPointLoad> pointLoad;
         pointLoads->Create(loadID++,mbrID,x,0,diaphragmLoad.Load,0,lotMember,&pointLoad);
      }
   }
}

void pgsGirderModelFactory::ApplyPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                                  bool bModelLeftCantilever, bool bModelRightCantilever,const std::vector<pgsPointOfInterest>& vPOI,
                                                  IFem2dModel** ppModel,pgsPoiPairMap* pPoiMap)
{
   // layout poi on fem model
   pPoiMap->Clear();
   std::vector<PoiIDPairType> poiIDs = pgsGirderModelFactory::AddPointsOfInterest(*ppModel,vPOI);
   std::vector<PoiIDPairType>::iterator poiIDiter(poiIDs.begin());
   std::vector<PoiIDPairType>::iterator poiIDiterEnd(poiIDs.end());
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPOI.end());
   for ( ; poiIDiter != poiIDiterEnd && poiIter != poiIterEnd; poiIDiter++, poiIter++ )
   {
      pPoiMap->AddMap(*poiIter,*poiIDiter);
   }
}

void pgsGirderModelFactory::FindMember(IFem2dModel* pModel,Float64 distFromStartOfModel,MemberIDType* pMbrID,Float64* pDistFromStartOfMbr)
{
   CComPtr<IFem2dMemberCollection> members;
   pModel->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   pModel->get_Joints(&joints);

   CComPtr<IFem2dEnumMember> enumMembers;
   members->get__EnumElements(&enumMembers);

   CComPtr<IFem2dMember> mbr;
   while ( enumMembers->Next(1,&mbr,NULL) != S_FALSE )
   {
      CComPtr<IFem2dJoint> j1, j2;
      JointIDType jntID1, jntID2;
      mbr->get_StartJoint(&jntID1);
      mbr->get_EndJoint(&jntID2);

      joints->Find(jntID1,&j1);
      joints->Find(jntID2,&j2);

      Float64 x1,x2;
      j1->get_X(&x1);
      j2->get_X(&x2);

      if ( InRange(x1,distFromStartOfModel,x2) )
      {
         mbr->get_ID(pMbrID);
         *pDistFromStartOfMbr = distFromStartOfModel - x1;
         return;
      }

      mbr.Release();
   }

   ATLASSERT(false); // didn't find a solution
}

PoiIDPairType pgsGirderModelFactory::AddPointOfInterest(IFem2dModel* pModel,const pgsPointOfInterest& poi)
{
   // layout poi on fem model
   CComPtr<IFem2dJointCollection> joints;
   pModel->get_Joints(&joints);
   CollectionIndexType nJoints;
   joints->get_Count(&nJoints);

   CComPtr<IFem2dPOICollection> pois;
   pModel->get_POIs(&pois);

   Float64 dist_from_start_of_member;
   MemberIDType mbrID = 0;
   
   CollectionIndexType jntIdx = 0;

   CComPtr<IFem2dJoint> prevJnt;
   joints->get_Item(jntIdx++,&prevJnt);
   Float64 prevLocation;
   prevJnt->get_X(&prevLocation);

   Float64 poi_dist_from_start = poi.GetDistFromStart();

   bool is_dual_pois = false; // if poi straddles a support location, add fem pois at either side
   for ( ; jntIdx < nJoints; jntIdx++, mbrID++ )
   {
      CComPtr<IFem2dJoint> jnt;
      joints->get_Item(jntIdx,&jnt);
      Float64 location;
      jnt->get_X(&location);
      VARIANT_BOOL is_support;
      jnt->IsSupport(&is_support);

      if (is_support && IsEqual(poi_dist_from_start,location) && jntIdx!=nJoints-1 )
      {
         // poi is directly over an interior support joint. we need fem pois on either side
         CComPtr<IFem2dPOI> objPOI_left,objPOI_right;

         PoiIDType femID_left = ms_FemModelPoiID++;
         PoiIDType femID_right = ms_FemModelPoiID++;

         HRESULT hr = pois->Create(femID_left, mbrID, -1.0, &objPOI_left);
         ATLASSERT(SUCCEEDED(hr));
         hr = pois->Create(femID_right, mbrID+1, 0.0, &objPOI_right);
         ATLASSERT(SUCCEEDED(hr));

         return PoiIDPairType(femID_left, femID_right);
      }
      else if ( InRange(prevLocation, poi_dist_from_start, location) )
      {
         dist_from_start_of_member = poi_dist_from_start - prevLocation;

         CComPtr<IFem2dPOI> objPOI;

         PoiIDType femID = ms_FemModelPoiID++;

         HRESULT hr = pois->Create(femID,mbrID,dist_from_start_of_member,&objPOI);
         ATLASSERT(SUCCEEDED(hr));

         return PoiIDPairType(femID, femID);
      }

      prevLocation = location;
   }

   // poi not on model. should never happen
   ATLASSERT(0); 
   return PoiIDPairType(INVALID_ID,INVALID_ID);
}

std::vector<PoiIDPairType> pgsGirderModelFactory::AddPointsOfInterest(IFem2dModel* pModel,const std::vector<pgsPointOfInterest>& vPOI)
{
   std::vector<PoiIDPairType> femIDs;

   std::vector<pgsPointOfInterest>::const_iterator i(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; i != end; i++)
   {
      const pgsPointOfInterest& poi = *i;
      femIDs.push_back(pgsGirderModelFactory::AddPointOfInterest(pModel,poi));
   }

   return femIDs;
}

/////////////////////////////////////////////////////////////////////
/////////// class pgsKdotHaulingGirderModelFactory //////////////////
/////////////////////////////////////////////////////////////////////
pgsKdotHaulingGirderModelFactory::pgsKdotHaulingGirderModelFactory(Float64 overhangFactor, Float64 interiorFactor):
m_OverhangFactor(overhangFactor), m_InteriorFactor(interiorFactor)
{
}

pgsKdotHaulingGirderModelFactory::~pgsKdotHaulingGirderModelFactory(void)
{
}

void pgsKdotHaulingGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                                  LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,
                                                  const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel)
{
   ATLASSERT(bModelLeftCantilever && bModelRightCantilever); // kdot method should always include cantilevers

   // apply  loads
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   (*ppModel)->get_Loadings(&loadings);
   loadings->Create(lcidGirder,&loading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   std::vector<SegmentLoad> segLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   std::vector<ClosureJointLoad> cjLoads;
   pProductLoads->GetSegmentSelfWeightLoad(segmentKey,&segLoads,&diaphLoads,&cjLoads);

   // apply dynamically factored girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<SegmentLoad>::iterator segLoadIter(segLoads.begin());
   std::vector<SegmentLoad>::iterator segLoadIterEnd(segLoads.end());
   for ( ; segLoadIter != segLoadIterEnd; segLoadIter++ )
   {
      SegmentLoad& segLoad = *segLoadIter;

      Float64 wStart = segLoad.wStart;
      Float64 wEnd   = segLoad.wEnd;
      Float64 start  = segLoad.StartLoc;
      Float64 end    = segLoad.EndLoc;

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member and is all interior
         wStart *= m_InteriorFactor;
         wEnd   *= m_InteriorFactor;

         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wStart,wEnd,lotMember,&distLoad);
      }
      else
      {
         // load straddles two or more members
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 w1,w2; // start and end load intensity on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            CComPtr<IFem2dMember> mbr;
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            CComPtr<IFem2dJoint> jntStart, jntEnd;
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               w1 = wStart;
               x1 = xStart;
            }
            else
            {
               w1 = ::LinInterp(xMbrStart,wStart,wEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               w2 = wEnd;
               x2 = xEnd;
            }
            else
            {
               w2 = ::LinInterp(xMbrEnd,wStart,wEnd,end-start);
               x2 = Lmbr; // end of member
            }

            // Factor loads depending on whether they are on cantilever, or interior
            bool onLeft  = IsLE(xMbrStart, leftSupportLoc)  && IsLE(xMbrEnd, leftSupportLoc);
            bool onRight = IsLE(rightSupportLoc, xMbrStart) && IsLE(rightSupportLoc, xMbrEnd);
            if ( onLeft || onRight)
            {
               w1 *= this->m_OverhangFactor;
               w2 *= this->m_OverhangFactor;
            }
            else
            {
               w1 *= this->m_InteriorFactor;
               w2 *= this->m_InteriorFactor;
            }

            CComPtr<IFem2dDistributedLoad> distLoad;
            distributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,w1,w2,lotMember,&distLoad);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////
/////////// class pgsDesignHaunchLoadGirderModelFactory //////////////////
/////////////////////////////////////////////////////////////////////
pgsDesignHaunchLoadGirderModelFactory::pgsDesignHaunchLoadGirderModelFactory( const std::vector<SlabLoad>& slabLoads, LoadCaseIDType slabLoadCase, LoadCaseIDType slabPadLoadCase):
m_SlabLoads(slabLoads), m_SlabLoadCase(slabLoadCase), m_SlabPadLoadCase(slabPadLoadCase)
{
}

pgsDesignHaunchLoadGirderModelFactory::~pgsDesignHaunchLoadGirderModelFactory(void)
{
}

void pgsDesignHaunchLoadGirderModelFactory::ApplyLoads(IBroker* pBroker,const CSegmentKey& segmentKey,Float64 segmentLength,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                                  LoadCaseIDType lcidGirder,bool bModelLeftCantilever, bool bModelRightCantilever,
                                                  const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel)
{
   // apply  loads
   // We dont need girder self weight, so don't use it
   CComPtr<IFem2dLoadingCollection> loadings;
   (*ppModel)->get_Loadings(&loadings);

   CComPtr<IFem2dLoading> slabPadLoading, slabLoading;

   loadings->Create(m_SlabLoadCase,   &slabLoading);
   loadings->Create(m_SlabPadLoadCase,&slabPadLoading);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);

   CComPtr<IFem2dDistributedLoadCollection> slabDistributedLoads, slabPadDistributedLoads;
   slabLoading->get_DistributedLoads(&slabDistributedLoads);
   slabPadLoading->get_DistributedLoads(&slabPadDistributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;

   std::vector<SlabLoad>::iterator slabLoadIter(m_SlabLoads.begin());
   std::vector<SlabLoad>::iterator slabLoadIterEnd(m_SlabLoads.end());

   // load information is at individual locations. Need to get get first value to get ball rolling
   SlabLoad& startSlabLoad = *slabLoadIter;
   Float64 start = startSlabLoad.Loc;
   Float64 wslabStart = startSlabLoad.MainSlabLoad + startSlabLoad.PanelLoad;
   Float64 wslabPadStart = startSlabLoad.PadLoad;
   slabLoadIter++;

   for ( ; slabLoadIter != slabLoadIterEnd; slabLoadIter++ )
   {
      SlabLoad& slabLoad = *slabLoadIter;

      Float64 end = slabLoad.Loc;
      Float64 wslabEnd = slabLoad.MainSlabLoad + slabLoad.PanelLoad;
      Float64 wslabPadEnd = slabLoad.PadLoad;


      if ( !bModelLeftCantilever && ::IsLT(start,leftSupportLoc) )
      {
         // this load segment begins before the left support and we are ignoring loads out there
  
         // compute load intensity at the left support
         wslabStart = ::LinInterp(leftSupportLoc,wslabStart,wslabEnd,end-start);
         wslabPadStart = ::LinInterp(leftSupportLoc,wslabPadStart,wslabPadEnd,end-start);
         start = leftSupportLoc;
      }

      if ( !bModelRightCantilever && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment ends after the right support and we are ignoring loads out there

         // compute load intensity at the right support
         wslabEnd = ::LinInterp(rightSupportLoc-start,wslabStart,wslabEnd,end-start);
         wslabPadEnd = ::LinInterp(rightSupportLoc-start,wslabPadStart,wslabPadEnd,end-start);
         end = rightSupportLoc;
      }

      // apply the loading
      MemberIDType mbrIDStart; // member ID at the start of the load
      MemberIDType mbrIDEnd;   // member ID at the end of the load
      Float64 xStart; // distance from start of member mbrIDStart to the start of the load
      Float64 xEnd;   // distance from start of member mbrIDEnd to end of the load
      FindMember(*ppModel,start,&mbrIDStart,&xStart);
      FindMember(*ppModel,end,  &mbrIDEnd,  &xEnd);

      if ( mbrIDStart == mbrIDEnd )
      {
         // load is contained on a single member and is all interior
         CComPtr<IFem2dDistributedLoad> slabDistLoad, slabPadDistLoad;
         slabDistributedLoads->Create(   loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wslabStart,   wslabEnd,   lotMember,&slabDistLoad);
         slabPadDistributedLoads->Create(loadID++,mbrIDStart,loadDirFy,xStart,xEnd,wslabPadStart,wslabPadEnd,lotMember,&slabPadDistLoad);
      }
      else
      {
         // load straddles two or more members
         for ( MemberIDType mbrID = mbrIDStart; mbrID <= mbrIDEnd; mbrID++ )
         {
            Float64 wsl1, wsl2, wsp1, wsp2; // start and end load intensity of slab and slab pad on this member
            Float64 x1,x2; // start and end load location from the start of this member

            Float64 Lmbr;
            CComPtr<IFem2dMember> mbr;
            members->Find(mbrID,&mbr);
            mbr->get_Length(&Lmbr); 

            JointIDType jntIDStart,jntIDEnd;
            mbr->get_StartJoint(&jntIDStart);
            mbr->get_EndJoint(&jntIDEnd);

            CComPtr<IFem2dJoint> jntStart, jntEnd;
            joints->Find(jntIDStart,&jntStart);
            joints->Find(jntIDEnd,  &jntEnd);

            Float64 xMbrStart, xMbrEnd;
            jntStart->get_X(&xMbrStart);
            jntEnd->get_X(&xMbrEnd);

            if ( mbrID == mbrIDStart )
            {
               wsl1 = wslabStart;
               wsp1 = wslabPadStart;
               x1 = xStart;
            }
            else
            {
               wsl1 = ::LinInterp(xMbrStart,wslabStart,   wslabEnd,   end-start);
               wsp1 = ::LinInterp(xMbrStart,wslabPadStart,wslabPadEnd,end-start);
               x1 = 0; // start of member
            }

            if (mbrID == mbrIDEnd )
            {
               wsl2 = wslabEnd;
               wsp2 = wslabPadEnd;
               x2 = xEnd;
            }
            else
            {
               wsl2 = ::LinInterp(xMbrEnd,wslabStart,   wslabEnd,   end-start);
               wsp2 = ::LinInterp(xMbrEnd,wslabPadStart,wslabPadEnd,end-start);
               x2 = Lmbr; // end of member
            }


            CComPtr<IFem2dDistributedLoad> slabDistLoad, slabPadDistLoad;
            slabDistributedLoads->Create(   loadID++,mbrID,loadDirFy,x1,x2,wsl1,wsl2,lotMember,&slabDistLoad);
            slabPadDistributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,wsp1,wsp2,lotMember,&slabPadDistLoad);
         }
      }

      // cycle
      start = end;
      wslabStart = wslabEnd;
      wslabPadStart = wslabPadEnd;
   }
}