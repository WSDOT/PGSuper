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

#include "stdafx.h"
#include "PrivateHelpers.h"
#include <IFace\Bridge.h>
#include "AgeAdjustedMaterial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void BuildSplicedGirderMaterialModel(IBroker* pBroker,const CPrecastSegmentData* pSegment,ISplicedGirderSegment* segment, IGirderSection* gdrSection)
{
   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   bool bHasClosure[2] = {
                           pSegment->GetStartClosure()  != NULL, 
                           pSegment->GetEndClosure() != NULL
                         };

   // If this is the first segment of a girder and there is a previous group -OR-
   // if this is the last segment of a girder and there is next group, 
   // then there is a cast-in-place diaphragm between the groups. Use the deck concrete as the closure joint
   // concrete in this case.
   bool bPierDiaphragm[2] = { 
                              (pSegment->GetPrevSegment() == NULL && pGirder->GetGirderGroup()->GetPrevGirderGroup() != NULL),
                              (pSegment->GetNextSegment() == NULL && pGirder->GetGirderGroup()->GetNextGirderGroup() != NULL)   
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

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,segmentMaterial,NULL);

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

         segment->put_ClosureJointForegroundMaterial(endType,closureMaterial);
         segment->put_ClosureJointBackgroundMaterial(endType,NULL);
      }
      else if ( bPierDiaphragm[endType] )
      {
         CGirderKey girderKey(segmentKey);
         CComObject<CAgeAdjustedMaterial>* pDiaphragmMaterial;
         CComObject<CAgeAdjustedMaterial>::CreateInstance(&pDiaphragmMaterial);
         CComPtr<IAgeAdjustedMaterial> diaphragmMaterial = pDiaphragmMaterial;
         diaphragmMaterial->InitDeck(girderKey,pMaterials);

         segment->put_ClosureJointForegroundMaterial(endType,diaphragmMaterial);
         segment->put_ClosureJointBackgroundMaterial(endType,NULL);
      }
   }
}