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
#include <IFace\AnalysisResults.h>

PoiIDType pgsGirderModelFactory::ms_FemModelPoiID = 0;

pgsGirderModelFactory::pgsGirderModelFactory(void)
{
}


pgsGirderModelFactory::~pgsGirderModelFactory(void)
{
}

void pgsGirderModelFactory::CreateGirderModel(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
{
   // use template methods
   Float64 girderLength = BuildModel(pBroker, spanIdx, gdrIdx, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);

   ApplyLoads(pBroker, spanIdx, gdrIdx, girderLength, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);

   ApplyPointsOfInterest(pBroker, spanIdx, gdrIdx, leftSupportLoc, rightSupportLoc, E, lcidGirder, bIncludeCantilevers, vPOI, ppModel, pPoiMap);
}

Float64 pgsGirderModelFactory::BuildModel(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,
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
   std::vector<pgsPointOfInterest> xsPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE,POIFIND_OR);

   // add section data for the support locations
   if ( !IsEqual(leftSupportLoc,xsPOI.front().GetDistFromStart()) )
      xsPOI.push_back(pgsPointOfInterest(spanIdx,gdrIdx,leftSupportLoc));

   if ( !IsEqual(rightSupportLoc,xsPOI.back().GetDistFromStart()) )
      xsPOI.push_back(pgsPointOfInterest(spanIdx,gdrIdx,rightSupportLoc));

   // sort the POI
   std::sort(xsPOI.begin(),xsPOI.end());

   // layout the joints
   JointIDType jntID = 0;
   Float64 end_loc = 0.0;
   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);
   std::vector<pgsPointOfInterest>::iterator jointIter;
   for ( jointIter = xsPOI.begin(); jointIter < xsPOI.end(); jointIter++ )
   {
      pgsPointOfInterest poi = *jointIter;
      Float64 poi_loc = poi.GetDistFromStart();
      if ( !bIncludeCantilevers && (poi_loc < leftSupportLoc || rightSupportLoc < poi_loc) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next joint
         continue;
      }

      CComPtr<IFem2dJoint> jnt;
      joints->Create(jntID++,poi_loc,0,&jnt);

      // set boundary conditions if this is a support joint
      if ( IsEqual(poi_loc,leftSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtFx);
         jnt->ReleaseDof(jrtMz);
      }
      else if ( IsEqual(poi_loc,rightSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtMz);
      }

      if ( poi.HasAttribute(pgsTypes::CastingYard,POI_SECTCHANGE_LEFTFACE) )
      {
         // jump over the right face
         jointIter++;
         if ( jointIter == xsPOI.end() )
            break;
      }

      // capture last location, which is end of beam
      end_loc = poi_loc;
   }

   // create members
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);

   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   MemberIDType mbrID = 0;
   JointIDType prevJntID = 0;
   jntID = prevJntID + 1;
   std::vector<pgsPointOfInterest>::iterator prevJointIter = xsPOI.begin();
   jointIter = prevJointIter;
   jointIter++;
   for ( ; jointIter < xsPOI.end(); jointIter++, prevJointIter++, jntID++, prevJntID++ )
   {
      pgsPointOfInterest prevPoi = *prevJointIter;
      pgsPointOfInterest poi     = *jointIter;

      if ( !bIncludeCantilevers && (prevPoi.GetDistFromStart() < leftSupportLoc || rightSupportLoc < prevPoi.GetDistFromStart()) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next member
         continue;
      }

      Float64 prevEI = E*pSectProp2->GetIx(pgsTypes::CastingYard,prevPoi);
      Float64 prevEA = E*pSectProp2->GetAg(pgsTypes::CastingYard,prevPoi);

      Float64 currEI = E*pSectProp2->GetIx(pgsTypes::CastingYard,poi);
      Float64 currEA = E*pSectProp2->GetAg(pgsTypes::CastingYard,poi);

      Float64 EI = (prevEI + currEI)/2;
      Float64 EA = (prevEA + currEA)/2;

      CComPtr<IFem2dMember> member;
      members->Create(mbrID++,prevJntID,jntID,EA,EI,&member);

      if ( poi.HasAttribute(pgsTypes::CastingYard,POI_SECTCHANGE_LEFTFACE) )
      {
         // if we are at an abrupt section change, jump ahead
         prevJointIter++;
         jointIter++;

         if ( jointIter == xsPOI.end() )
            break;
      }
   }

   return end_loc;
}

void pgsGirderModelFactory::ApplyLoads(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 girderLength,
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
   pProductLoads->GetGirderSelfWeightLoad(spanIdx,gdrIdx,&gdrLoads,&diaphLoads);

   // apply girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<GirderLoad>::iterator gdrLoadIter;
   for ( gdrLoadIter = gdrLoads.begin(); gdrLoadIter != gdrLoads.end(); gdrLoadIter++ )
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

            CComPtr<IFem2dDistributedLoad> distLoad;
            distributedLoads->Create(loadID++,mbrID,loadDirFy,x1,x2,w1,w2,lotMember,&distLoad);
         }
      }
   }

   // apply diaphragm loads for precast diaphragms
   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);


   std::vector<DiaphragmLoad>::iterator diaLoadIter;
   for ( diaLoadIter = diaphLoads.begin(); diaLoadIter != diaphLoads.end(); diaLoadIter++ )
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

void pgsGirderModelFactory::ApplyPointsOfInterest(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,
                                                  Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,
                                                  bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,
                                                  IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
{
   // layout poi on fem model
   pPoiMap->Clear();
   std::vector<PoiIDType> poiIDs = pgsGirderModelFactory::AddPointsOfInterest(*ppModel,vPOI);
   std::vector<PoiIDType>::iterator poiIDiter;
   std::vector<pgsPointOfInterest>::const_iterator poiIter;
   for ( poiIDiter = poiIDs.begin(), poiIter = vPOI.begin(); poiIDiter != poiIDs.end() && poiIter != vPOI.end(); poiIDiter++, poiIter++ )
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

   for (std::vector<pgsPointOfInterest>::const_iterator i = vPOI.begin(); i != vPOI.end(); i++)
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

void pgsKdotHaulingGirderModelFactory::ApplyLoads(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 girderLength, 
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
   pProductLoads->GetGirderSelfWeightLoad(spanIdx,gdrIdx,&gdrLoads,&diaphLoads);

   // apply dynamically factored girder self weight load
   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   MemberIDType mbrID = 0;
   LoadIDType loadID = 0;
   std::vector<GirderLoad>::iterator gdrLoadIter;
   for ( gdrLoadIter = gdrLoads.begin(); gdrLoadIter != gdrLoads.end(); gdrLoadIter++ )
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
