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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\EditLossParameters.h"
#include <IFace\Project.h>

txnEditLossParameters::txnEditLossParameters(const txnEditLossParametersData& oldData,const txnEditLossParametersData& newData)
{
   m_LossParameters[0] = oldData;
   m_LossParameters[1] = newData;
}

txnEditLossParameters::~txnEditLossParameters()
{
}

bool txnEditLossParameters::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditLossParameters::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditLossParameters::CreateClone() const
{
   return new txnEditLossParameters(m_LossParameters[0],m_LossParameters[1]);
}

std::_tstring txnEditLossParameters::Name() const
{
   std::_tostringstream os;
   os << "Edit Prestress Loss Parameters" << std::endl;
   return os.str();
}

bool txnEditLossParameters::IsUndoable()
{
   return true;
}

bool txnEditLossParameters::IsRepeatable()
{
   return false;
}

void txnEditLossParameters::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);

   pLossParameters->SetTendonPostTensionParameters(m_LossParameters[i].Dset_PT,
                                                   m_LossParameters[i].WobbleFriction_PT,
                                                   m_LossParameters[i].FrictionCoefficient_PT);

   pLossParameters->SetTemporaryStrandPostTensionParameters(m_LossParameters[i].Dset_TTS,
                                                            m_LossParameters[i].WobbleFriction_TTS,
                                                            m_LossParameters[i].FrictionCoefficient_TTS);

   pLossParameters->UseGeneralLumpSumLosses(          m_LossParameters[i].bUseLumpSumLosses );
   pLossParameters->SetBeforeXferLosses(              m_LossParameters[i].BeforeXferLosses );
   pLossParameters->SetAfterXferLosses(               m_LossParameters[i].AfterXferLosses );
   pLossParameters->SetLiftingLosses(                 m_LossParameters[i].LiftingLosses );
   pLossParameters->SetShippingLosses(                m_LossParameters[i].ShippingLosses );
   pLossParameters->SetBeforeTempStrandRemovalLosses( m_LossParameters[i].BeforeTempStrandRemovalLosses );
   pLossParameters->SetAfterTempStrandRemovalLosses(  m_LossParameters[i].AfterTempStrandRemovalLosses );
   pLossParameters->SetAfterDeckPlacementLosses(      m_LossParameters[i].AfterDeckPlacementLosses );
   pLossParameters->SetAfterSIDLLosses(               m_LossParameters[i].AfterSIDLLosses );
   pLossParameters->SetFinalLosses(                   m_LossParameters[i].FinalLosses );

   pEvents->FirePendingEvents();
}
