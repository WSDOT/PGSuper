Selecting a Configuration {#ug_dialogs_configure}
==============================================
This step in the BridgeLink configuration wizard is used to select a Configuration. A configuation must be selected so that project templates and libraries are installed. Project templates and libraries are used to create new project files.

> NOTE: You can use configurations for agencies other that WSDOT or TxDOT. Check with the agency to see if they publish a configuration. Several agency specific configurations are available from third party developers. You can also create and publish your own configurations.

Configuration Information
------------------
There are two basic configuration options. 

Option | Description
------|--------------
Use a configuration published on a Configuration Server | These configurations are shared by users over a network such as the Internet. Using this type of configuration is the most common option. See Shared Configurations below for more information.
Use the default configuration | A default configuration was installed with the BridgeLink. This configuration does not change and can become out of date over time. However, it provides a configuration when online Configuration Servers are not available.

### Shared Configurations ###
Shared configurations are "in the cloud" and are shared by many engineers. Use the drop down list to select a Configuration Server. Then select a specific configuration from the list. For example, the WSDOT Configuration Server provides a WSDOT configuration and an AASHTO configuration.

Use [Manage] to create or modify your list of available @ref ug_dialogs_configuration_servers.

For more information about a configuration, click the "More about this configuration..." hyperlink. This will open your web browser and go to an information page for the selected configuration.

### Check for configuration updates ###
Use the drop down list to specify the frequency with which the configuration server is checked for updates.

> TIP: Once you've selected a configuration, you aren't "locked in". You can change at any time.

> TIP: Once a configuration has been installed, you do not need to be connected to your local network or the Internet. The configuration information is stored on your computer. Reconnect to the configuration server on a regular basis to check for updates.
