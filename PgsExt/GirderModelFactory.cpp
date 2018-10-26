///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
                                 bool bIncludeCantilevers,                    // if true, cantilevers defined by leftSupportLoc and rightSupportLoc are modeled
                                 const std::vector<pgsPointOfInterest>& vPOI, // vector of PGSuper POIs that are to be modeld in the Fem2d Model
                                 IFem2dModel** ppModel,                       // the Fem2d Model
                                 pgsPoiMap* pPoiMap                           // a mapping of PGSuper POIs to Fem2d POIs
                                 )
{
   // use template methods
   BuildModel(pBroker, intervalIdx, segmentKey, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

   ApplyLoads(pBroker, segmentKey, segmentLength, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);

   ApplyPointsOfInterest(pBroker, segmentKey, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);
}

void pgsGirderModelFactory::BuildModel(IBroker* pBroker,IntervalIndexType intervalIdx,const CSegmentKey& segmentKey,
                                          Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,
                                          LoadCaseIDType lcidGirder,bool bIncludeCantilevers,
                                          const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
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
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> xsPOI = pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE);
   pPOI->RemovePointsOfInterest(xsPOI,POI_ERECTED_SEGMENT);


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
      if ( !bIncludeCantilevers && (Xpoi < leftSupportLoc || rightSupportLoc < Xpoi) )
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

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   MemberIDType mbrID = 0;
   JointIDType prevJntID = 0;
   jntID = prevJntID + 1;
   std::vector<pgsPointOfInterest>::iterator prevJointIter( xsPOI.begin() );
   jointIter = prevJointIter;
   jointIter++;
   jointIterEnd = xsPOI.end();
   for ( ; jointIter < jointIterEnd; jointIter++, prevJointIter++, jntID++, prevJntID++ )
   {
      pgsPointOfInterest& prevPoi( *prevJointIter );
      pgsPointOfInterest& poi( *jointIter );

      if ( !bIncludeCantilevers && (prevPoi.GetDistFromStart() < leftSupportLoc || rightSupportLoc < prevPoi.GetDistFromStart()) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next member
         continue;
      }

      Float64 prevEI = E*pSectProp->GetIx(intervalIdx,prevPoi);
      Float64 prevEA = E*pSectProp->GetAg(intervalIdx,prevPoi);

      Float64 currEI = E*pSectProp->GetIx(intervalIdx,poi);
      Float64 currEA = E*pSectProp->GetAg(intervalIdx,poi);

      Float64 EI = (prevEI + currEI)/2;
      Float64 EA = (prevEA + currEA)/2;

      CComPtr<IFem2dMember> member;
      members->Create(mbrID++,prevJntID,jntID,EA,EI,&member);

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
                                       bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,
                                       IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
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

   std::vector<GirderLoad> gdrLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   pProductLoads->GetGirderSelfWeightLoad(segmentKey,&gdrLoads,&diaphLoads);

   // apply girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<GirderLoad>::iterator gdrLoadIter(gdrLoads.begin());
   std::vector<GirderLoad>::iterator gdrLoadIterEnd(gdrLoads.end());
   for ( ; gdrLoadIter != gdrLoadIterEnd; gdrLoadIter++ )
   {
      GirderLoad& gdrLoad = *gdrLoadIter;

      Float64 wStart = gdrLoad.wStart;
      Float64 wEnd   = gdrLoad.wEnd;
      Float64 start  = gdrLoad.StartLoc;
      Float64 end    = gdrLoad.EndLoc;

      if ( !bIncludeCantilevers && ::IsLT(start,leftSupportLoc) )
      {
         // this load segment begins before the left support and we are ignoring loads out there
  
         // compute load intensity at the left support
         wStart = ::LinInterp(leftSupportLoc,wStart,wEnd,end-start);
         start = leftSupportLoc;
      }
      else if ( !bIncludeCantilevers && ::IsLT(rightSupportLoc,end) )
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

         if ( !bIncludeCantilevers && (xPrev < leftSupportLoc || rightSupportLoc < xPrev) )
         {
            // location is before or after the left/right support and we arn't modeling
            // the cantilevers... next member
            continue;
         }

         if ( InRange(xPrev,diaphragmLoad.Loc,xNext) )
         {
            x = diaphragmLoad.Loc - xPrev;
            break;
         }
      }

      CComPtr<IFem2dPointLoad> pointLoad;
      pointLoads->Create(loadID++,mbrID,x,0,diaphragmLoad.Load,0,lotMember,&pointLoad);
   }
}

void pgsGirderModelFactory::ApplyPointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                                  bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,
                                                  IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
{
   // layout poi on fem model
   pPoiMap->Clear();
   std::vector<PoiIDType> poiIDs = pgsGirderModelFactory::AddPointsOfInterest(*ppModel,vPOI);
   std::vector<PoiIDType>::iterator poiIDiter(poiIDs.begin());
   std::vector<PoiIDType>::iterator poiIDiterEnd(poiIDs.end());
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

PoiIDType pgsGirderModelFactory::AddPointOfInterest(IFem2dModel* pModel,const pgsPointOfInterest& poi)
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

   for ( ; jntIdx < nJoints; jntIdx++, mbrID++ )
   {
      CComPtr<IFem2dJoint> jnt;
      joints->get_Item(jntIdx,&jnt);
      Float64 location;
      jnt->get_X(&location);

      if ( InRange(prevLocation,poi.GetDistFromStart(),location) )
      {
         dist_from_start_of_member = poi.GetDistFromStart() - prevLocation;
         break;
      }

      prevLocation = location;
   }

   CComPtr<IFem2dPOI> objPOI;

   PoiIDType femID = ms_FemModelPoiID++;

   HRESULT hr = pois->Create(femID,mbrID,dist_from_start_of_member,&objPOI);
   ATLASSERT(SUCCEEDED(hr));

   return femID;
}

std::vector<PoiIDType> pgsGirderModelFactory::AddPointsOfInterest(IFem2dModel* pModel,const std::vector<pgsPointOfInterest>& vPOI)
{
   std::vector<PoiIDType> femIDs;

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
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                                  bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,
                                                  IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
{
   ATLASSERT(bIncludeCantilevers); // kdot method should always include cantilevers

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

   std::vector<GirderLoad> gdrLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   pProductLoads->GetGirderSelfWeightLoad(segmentKey,&gdrLoads,&diaphLoads);

   // apply dynamically factored girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<GirderLoad>::iterator gdrLoadIter(gdrLoads.begin());
   std::vector<GirderLoad>::iterator gdrLoadIterEnd(gdrLoads.end());
   for ( ; gdrLoadIter != gdrLoadIterEnd; gdrLoadIter++ )
   {
      GirderLoad& gdrLoad = *gdrLoadIter;

      Float64 wStart = gdrLoad.wStart;
      Float64 wEnd   = gdrLoad.wEnd;
      Float64 start  = gdrLoad.StartLoc;
      Float64 end    = gdrLoad.EndLoc;

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