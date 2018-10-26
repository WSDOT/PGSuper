Configurations {#configurations}
============
The functionality of PGSuper and PGSplice can be customized with Configurations. Configurations model standard bridge components such as girders and traffic barriers, as well as the design policies of bridge owners such as State DOTs.

Bridge owners, such as WSDOT and TxDOT, publish configurations on the Internet so all of their in-house and consulting engineers are using the same information. These configurations customize PGSuper and PGSplice for a specific agency.

Configurations make using PGSuper and PGSplice easy for consulting engineers who work on projects in different states. Simply change the configuration to match your client's DOT and get to work.

@ref ug_configurations in the @ref user_guide describes Configurations from the perspective of the configuration user (e.g. the design engineer). Here we will discuss Configurations from the perspective of the configuration creator.

We call the person that creates and manages configurations the Administrator. You can think of the Administrator more like a role than a specific person. A design engineer often fulfills the role of Administrator and works closely with IT staff to setup and publish configurations on a configuration server.

Understanding Configurations
---------------------------
There are four parts to a configuration; the Master Library, Project Templates, publishing instructions, and configuration documentation. 

The Master Library contains predefined objects such as girders, barriers, live loads, and project criteria. These objects are shared by many engineers and in many projects.

> NOTE: You can learn more about the Master Library in @ref ug_working_with_libraries in the @ref user_guide.

Project Templates define the default settings for new projects. PGSuper and PGSplice do not have any default values per se. New projects are always created by opening a Project Template and loading the supplied information. The information in the Project Template becomes the default settings. The information in Project Templates can be set to reflect an owner's design policies. @ref project_templates are discussed later in this guide.

Publishing instructions tell the configuration server important information about a configuration so that it is published and presented properly to the design engineer.

Configuration document is optional, but recommended. This documentation provides information about a configuration so the design engineer can make informed decisions about using it.

Configuration Servers
----------------------
The Master Library, Project Templates, publishing instructions and configuration documentation are packaged together to form a Configuration. After configurations have been developed, they are hosted on a Configuration Server. A Configuration Server can be your local file system, a network file system, an FTP server or an HTTP server. It is the Administrator's job to create configurations and host them on configuration servers. @ref configuration_servers are discussed later in this guide.
