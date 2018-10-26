The Master Library {#master_library}
========================
PGSuper and PGSplice can be customized by installing Configurations. Configurations consist of a Master Library and a collection of Project Templates. The Master Library defines bridge components and related information such as materials, connections, girders, traffic barriers, and design and load rating criteria. The Master Library, when made part of a configuration, can be shared by many PGSuper and PGSplice users all over the world.

> NOTE: You can learn more about Configurations in the PGSuper and PGSplice User Guides.


The available libraries within the Master Library are:

Library | Description | Entry Type 
--------|-------------|-------------
Concrete | Standard definition of concrete materials | Seed  
Connections | Connections describe the end regions of a precast girder, the end diaphragm, and how the dead load of the end diaphragm is applied to the system. | Seed  
Girders | Entries in this library describe standard precast girders. Girders are described by not only their cross sectional dimensions, but by strand patterns, longitudinal mild steel, transverse mild steel, harping point locations, and girder-specific design requirements. | Referenced with Seed Data  
Traffic Barriers | Entries in this library describe the cross sectional dimensions of standard traffic barriers. | Referenced  
Project Criteria | Entries in this library describe project criteria such as: the version of the LRFD Specifications, allowable stress overrides, choices of computations methods for prestress loss, creep, live load distribution factors, and parameters for long girder stability analysis. | Referenced  
Vehicular Live Load | Entries in this library describe user defined live loads | Referenced  
Load Rating Criteria | Entries in this library describe the criteria for load rating including the version of the AASHTO MBE and live load factors | Referenced  
Ducts | Entries in this library define standard ducts for spliced girder bridges. | Referenced  

### Library Types  ###

There are three general types of libraries; Referenced, Seed, and Referenced with Seed Data.

Referenced libraries contain entries that are added to a project by reference. By simply referencing the name of a library entry, all of the information stored in the library entry becomes a part of the project.

Seed libraries contain entries that are copied into your project. This gives users a starting point, but the information can be modified on a case by case bases.

The Girder library is a little bit different. The Girder entries are referenced into a project; however there are some parameters that the user can override. The Girder library is said to be Referenced with Seed Data.
