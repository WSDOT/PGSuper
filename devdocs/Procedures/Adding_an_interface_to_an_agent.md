# Adding an interface to an agent {#adding_an_interface_to_an_agent}

# Purpose
Ths procedure describes how to add an interface to an agent implemented with the ATL library.

# Procedure

- Either add the interface to an existing header file or create a new header file and store it in Include\IFace.  The interface should look something like this:
~~~
// {D88670F0-3B83-11d2-8EC5-006097DF3C68}
DEFINE_GUID(IID_IFoo,
0xD88670F0, 0x3B83, 0x11d2, 0x8E, 0xC5, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface IFoo : IUnknown
{
   virtual int Bar() const = 0;
};
~~~

- Use the GUIDGEN utility to get a unique identifier for this interface. Don't use the IID shown in the example code.

- Determine which agent is going to implement the interface

- Make the following modifications to the agent's class declaration
-# Derive the agent class from the new interface.  ATL uses multiple inheritance for supporting multiple interfaces

-# Add a interface entry to the agent's interface map
~~~
BEGIN_COM_MAP(CSomeAgentImp)
   COM_INTERFACE_ENTRY(IFoo)  // Add this line
END_COM_MAP()
~~~

-# Add the interface's methods in the class declaration
~~~
// IFoo
public:
   virtual int Bar() const override;
~~~

- Make the following modifications to the agent's class definition (implementation)

-# Register the interface with the broker at startup
~~~
STDMETHODIMP CSomeAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInit,&IID_IBrokerInit> pBrokerInit( m_pBroker );

   pBrokerInit->RegInterface( IID_IFoo, this ); // Add this line

   return S_OK;
}
~~~

-# Implement the interface's methods.
~~~
/////////////////////////////////////////////////////////////////////////
// IFoo
//
int CSomeAgentImp::Bar()
{
    int value = 0.0;
   // ...
   return value;
}
~~~
