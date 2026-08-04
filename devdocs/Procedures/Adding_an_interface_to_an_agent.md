# Adding an interface to an agent {#adding_an_interface_to_an_agent}

# Purpose
This procedure describes how to add an interface to an agent implemented with the WBFL EAF (Extensible Application Framework) ComponentObject/Agent framework.

# Procedure

- Either add the interface to an existing header file or create a new header file and store it in Include\IFace.  The interface should look something like this:
~~~
// {D88670F0-3B83-11d2-8EC5-006097DF3C68}
DEFINE_GUID(IID_IFoo,
0xD88670F0, 0x3B83, 0x11d2, 0x8E, 0xC5, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
class IFoo
{
public:
   virtual int Bar() const = 0;
};
~~~
The interface is a plain abstract C++ class. The DEFINE_GUID(IID_IFoo, ...) is required; it's the key the Broker uses to look up the interface at runtime.

- Use the GUIDGEN utility to get a unique identifier for this interface. Don't use the IID shown in the example code.

- Determine which agent is going to implement the interface

- Make the following modifications to the agent's class declaration
-# Derive the agent class from the new interface. Plain C++ multiple inheritance is used to support multiple interfaces on a single agent. For example:
~~~
class CSomeAgentImp : public WBFL::EAF::Agent,
   public IFoo,             // Add this line
   public ISomeOtherIFace
{
   ...
};
~~~
See `CAnalysisAgentImp` (AnalysisAgent\AnalysisAgentImp.h) for a real, larger example of this pattern.

-# Add the interface's methods in the class declaration
~~~
// IFoo
public:
   int Bar() const override;
~~~

- Make the following modifications to the agent's class definition (implementation)

-# Register the interface with the broker at startup, in the agent's RegisterInterfaces() override
~~~
bool CSomeAgentImp::RegisterInterfaces()
{
   EAF_AGENT_REGISTER_INTERFACES;

   REGISTER_INTERFACE(IFoo); // Add this line

   return true;
}
~~~

-# Implement the interface's methods.
~~~
/////////////////////////////////////////////////////////////////////////
// IFoo
//
int CSomeAgentImp::Bar() const
{
   int value = 0;
   // ...
   return value;
}
~~~
