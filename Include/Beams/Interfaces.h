#pragma once

#include <IFace\PsLossEngineer.h>

// {3A9CAD2E-2CD2-433d-BDE5-F8C9566148DE}
DEFINE_GUID(IID_IInitialize, 
0x3a9cad2e, 0x2cd2, 0x433d, 0xbd, 0xe5, 0xf8, 0xc9, 0x56, 0x61, 0x48, 0xde);
interface IInitialize : IUnknown
{
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) = 0;
};

typedef enum BeamTypes { IBeam, UBeam, SolidSlab, BoxBeam, SingleT } BeamTypes;

// {95C7A3E2-854E-41e2-BA49-96C611B335F3}
DEFINE_GUID(IID_IPsBeamLossEngineer, 
0x95c7a3e2, 0x854e, 0x41e2, 0xba, 0x49, 0x96, 0xc6, 0x11, 0xb3, 0x35, 0xf3);
interface IPsBeamLossEngineer : IPsLossEngineer
{
   virtual void Init(BeamTypes beamType) = 0;
};