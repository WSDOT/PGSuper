///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include "EditLossParameters.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

std::unique_ptr<CEAFTransaction> txnEditLossParameters::CreateClone() const
{
   return std::make_unique<txnEditLossParameters>(m_LossParameters[0],m_LossParameters[1]);
}

std::_tstring txnEditLossParameters::Name() const
{
   std::_tostringstream os;
   os << "Edit Prestress Loss Parameters" << std::endl;
   return os.str();
}

bool txnEditLossParameters::IsUndoable() const
{
   return true;
}

bool txnEditLossParameters::IsRepeatable() const
{
   return false;
}

void txnEditLossParameters::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   // Exception-safe holder to keep from fireing events until we are done
   CIEventsHolder event_holder(pEvents);

   GET_IFACE2(pBroker,ILossParameters,pLossParameters);

   pLossParameters->IgnoreTimeDependentEffects(m_LossParameters[i].bIgnoreCreepEffects,
                                               m_LossParameters[i].bIgnoreShrinkageEffects,
                                               m_LossParameters[i].bIgnoreRelaxationEffects);

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
}
