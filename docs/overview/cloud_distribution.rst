Cloud Distribution
------------------

Naja SNL and DNL have been designed to be deployed in a cloud environment.

SNL relies on the following aspects:
* Unique identification of objects independent of their name allowing
to name from their object. It is also possible before transfer to not only anonymize
the netlist but to remove sensible information such as LUT masks for instance in the context
of FPGA flow, if ever the tools and algorithms operating on the data are not relying on this
information.
* Data on disk clarity: By relying on off-the-shelf technology Cap'n Proto, users can 
leverage Cap'n Proto API and tools to inspect their data and easily develop
sign-off tools independent of Naja that can assert the absence of sensible information before
transfering their data from on-premise to Cloud.

The following diagram shows the possible flow that can be used to transfer Naja data
structure from on-premise to Cloud under user's control.

.. image:: ../images/Naja-SNL-Cloud.png
   :alt: Naja transfer on the cloud.