///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2009  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderModelFactory.h>
#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>

struct SectionData
{
   Float64 Location;
   Float64 EA;
   Float64 EI;

   bool operator<(const SectionData& other) const { return Location < other.Location && !IsEqual(Location,other.Location); }
};

PoiIDType pgsGirderModelFactory::ms_FemModelPoiID = 0;

pgsGirderModelFactory::pgsGirderModelFactory(void)
{
}


pgsGirderModelFactory::~pgsGirderModelFactory(void)
{
}

void pgsGirderModelFactory::CreateGirderModel(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 leftSupportLoc,Float64 rightSupportLoc,Float64 E,LoadCaseIDType lcidGirder,bool bIncludeCantilevers,const std::vector<pgsPointOfInterest>& vPOI,IFem2dModel** ppModel,pgsPoiMap* pPoiMap)
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
   std::vector<pgsPointOfInterest> xsPOI = pPOI->GetPointsOfInterest(pgsTypes::CastingYard,spanIdx,gdrIdx,POI_SECTCHANGE);

   std::set<SectionData> section_data;
   
   // collect data for all section changes (we will put fem model joints here)
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = xsPOI.begin(); iter != xsPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      SectionData sd;
      sd.Location = poi.GetDistFromStart();
      sd.EA = E*pSectProp2->GetAg(pgsTypes::CastingYard,poi);
      sd.EI = E*pSectProp2->GetIx(pgsTypes::CastingYard,poi);

      section_data.insert(sd);
   }

   // add section data for the support locations
   SectionData sd;
   sd.Location = leftSupportLoc;
   sd.EA = E*pSectProp2->GetAg(pgsTypes::CastingYard, pgsPointOfInterest(spanIdx,gdrIdx,sd.Location));
   sd.EI = E*pSectProp2->GetIx(pgsTypes::CastingYard, pgsPointOfInterest(spanIdx,gdrIdx,sd.Location));
   section_data.insert(sd);

   sd.Location = rightSupportLoc;
   sd.EA = E*pSectProp2->GetAg(pgsTypes::CastingYard, pgsPointOfInterest(spanIdx,gdrIdx,sd.Location));
   sd.EI = E*pSectProp2->GetIx(pgsTypes::CastingYard, pgsPointOfInterest(spanIdx,gdrIdx,sd.Location));
   section_data.insert(sd);

   // layout the joints
   JointIDType jntID = 0;
   CComPtr<IFem2dJointCollection> joints;
   (*ppModel)->get_Joints(&joints);
   std::set<SectionData>::iterator jointIter;
   for ( jointIter = section_data.begin(); jointIter != section_data.end(); jointIter++ )
   {
      SectionData sd = *jointIter;
      if ( !bIncludeCantilevers && (sd.Location < leftSupportLoc || rightSupportLoc < sd.Location) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next joint
         continue;
      }

      CComPtr<IFem2dJoint> jnt;
      joints->Create(jntID++,sd.Location,0,&jnt);

      // set boundary conditions if this is a support joint
      if ( IsEqual(sd.Location,leftSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtFx);
         jnt->ReleaseDof(jrtMz);
      }
      else if ( IsEqual(sd.Location,rightSupportLoc) )
      {
         jnt->Support();
         jnt->ReleaseDof(jrtMz);
      }
   }

   // create members
   CComPtr<IFem2dMemberCollection> members;
   (*ppModel)->get_Members(&members);

   MemberIDType mbrID = 0;
   JointIDType prevJntID = 0;
   jntID = prevJntID + 1;
   std::set<SectionData>::iterator prevJointIter = section_data.begin();
   jointIter = prevJointIter;
   jointIter++;
   for ( ; jointIter != section_data.end(); jointIter++, prevJointIter++, jntID++, prevJntID++ )
   {
      SectionData prevSD = *prevJointIter;
      SectionData sd     = *jointIter;

      if ( !bIncludeCantilevers && (prevSD.Location < leftSupportLoc || rightSupportLoc < prevSD.Location) )
      {
         // location is before or after the left/right support and we arn't modeling
         // the cantilevers... next member
         continue;
      }

      // use average properties for a segment
      Float64 EI = (prevSD.EI + sd.EI)/2;
      Float64 EA = (prevSD.EA + sd.EA)/2;

      CComPtr<IFem2dMember> member;
      members->Create(mbrID++,prevJntID,jntID,EA,EI,&member);
   }

   // apply loads
   GET_IFACE2(pBroker,IProductForces,pProductForces);
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   (*ppModel)->get_Loadings(&loadings);
   loadings->Create(lcidGirder,&loading);

   std::vector<GirderLoad> gdrLoads;
   std::vector<DiaphragmLoad> diaphLoads;
   pProductForces->GetGirderSelfWeightLoad(spanIdx,gdrIdx,&gdrLoads,&diaphLoads);

   // apply girder self weight load

   CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
   loading->get_DistributedLoads(&distributedLoads);
   
   mbrID = 0;
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
         Float64 w = ::LinInterp(leftSupportLoc,wStart,wEnd,end-start);

         // apply load from left support to end
         Float64 x;
         FindMember(*ppModel,(leftSupportLoc+end)/2,&mbrID,&x);

         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,w,wEnd,lotMember,&distLoad);
      }
      else if ( !bIncludeCantilevers && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment ends after the right support and we are ignoring loads out there

         // compute load intensity at the right support
         Float64 w = ::LinInterp(rightSupportLoc-start,wStart,wEnd,end-start);

         // apply load from start of segment to right support
         Float64 x;
         FindMember(*ppModel,(start+rightSupportLoc)/2,&mbrID,&x);

         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wStart,w,lotMember,&distLoad);
      }
      else if ( bIncludeCantilevers && ::IsLT(start,leftSupportLoc) && ::IsLT(rightSupportLoc,end) )
      {
         // this load segment goes over both the left and right supports... it needs to
         // be applied as 3 loads

         // Compute load intensity at supports
         Float64 wLeft  = ::LinInterp(leftSupportLoc, wStart,wEnd,end-start);
         Float64 wRight = ::LinInterp(rightSupportLoc,wStart,wEnd,end-start);

         // apply load from start of load segment to left support
         Float64 x;
         FindMember(*ppModel,(start+leftSupportLoc)/2,&mbrID,&x);

         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wStart,wLeft,lotMember,&distLoad);

         // apply load between supports
         FindMember(*ppModel,(leftSupportLoc+rightSupportLoc)/2,&mbrID,&x);
         distLoad.Release();
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wLeft,wRight,lotMember,&distLoad);

         // apply load from right support to end of load segment
         FindMember(*ppModel,(rightSupportLoc+end)/2,&mbrID,&x);
         distLoad.Release();
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wRight,wEnd,lotMember,&distLoad);
      }
      else if ( bIncludeCantilevers && ::IsLT(start,leftSupportLoc) && ::IsLT(leftSupportLoc,end) )
      {
         // this loading segment straddles the the left support location ... it needs to
         // be applied as 2 loads
         Float64 x;
         FindMember(*ppModel,(start+leftSupportLoc)/2,&mbrID,&x);
         
         // compute load intensity at left support
         Float64 w = ::LinInterp(leftSupportLoc,wStart,wEnd,end-start);

         // apply load from start to left support
         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wStart,w,lotMember,&distLoad);

         // apply load from left support to end
         distLoad.Release();
         FindMember(*ppModel,(leftSupportLoc+end)/2,&mbrID,&x);
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,w,wEnd,lotMember,&distLoad);
      }
      else if ( bIncludeCantilevers && ::IsLT(start,rightSupportLoc) && ::IsLT(rightSupportLoc,end) )
      {
         // this loading segment straddles the right support... it needs to
         // be applied as 2 loads
         Float64 x;
         FindMember(*ppModel,(start+rightSupportLoc)/2,&mbrID,&x);
         
         // compute load intensity at right support
         Float64 w = ::LinInterp(rightSupportLoc,wStart,wEnd,end-start);

         // apply load from start to right support
         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wStart,w,lotMember,&distLoad);

         // apply load from right support to end
         distLoad.Release();
         FindMember(*ppModel,(rightSupportLoc+end)/2,&mbrID,&x);
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,w,wEnd,lotMember,&distLoad);
      }
      else
      {
         // apply the loading
         Float64 x;
         FindMember(*ppModel,(start+end)/2,&mbrID,&x);
         CComPtr<IFem2dDistributedLoad> distLoad;
         distributedLoads->Create(loadID++,mbrID,loadDirFy,0.0,-1.0,wStart,wEnd,lotMember,&distLoad);
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
      prevJointIter = section_data.begin();
      jointIter = prevJointIter;
      jointIter++;
      for ( ; jointIter != section_data.end(); jointIter++, prevJointIter++, mbrID++ )
      {
         SectionData prevSD = *prevJointIter;
         SectionData sd     = *jointIter;

         if ( !bIncludeCantilevers && (prevSD.Location < leftSupportLoc || rightSupportLoc < prevSD.Location) )
         {
            // location is before or after the left/right support and we arn't modeling
            // the cantilevers... next member
            continue;
         }

         if ( InRange(prevSD.Location,diaphragmLoad.Loc,sd.Location) )
         {
            x = diaphragmLoad.Loc - prevSD.Location;
            break;
         }
      }

      CComPtr<IFem2dPointLoad> pointLoad;
      pointLoads->Create(loadID++,mbrID,x,0,diaphragmLoad.Load,0,lotMember,&pointLoad);
   }

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