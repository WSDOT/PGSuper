
#pragma once

/*****************************************************************************
INTERFACE
   IReporterEventSink

   Callback interface for report agent events.
*****************************************************************************/
// {E2E4D451-EFA3-408D-8674-6BD3EE70CAE5}
DEFINE_GUID(IID_IReporterEventSink, 
0xe2e4d451, 0xefa3, 0x408d, 0x86, 0x74, 0x6b, 0xd3, 0xee, 0x70, 0xca, 0xe5);
interface __declspec(uuid("{E2E4D451-EFA3-408D-8674-6BD3EE70CAE5}")) IReporterEventSink : IUnknown
{
   virtual HRESULT OnReportsChanged() = 0;
};
