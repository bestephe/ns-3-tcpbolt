ns-3-tcpbolt
========

Compute the flow completion times by simulation for both short flows modeled after an On-Line Data Intensive (OLDI) application and medium-sized background flows.  The simulations are performed with a modified version of ns-3, and this program simulations different combinations of DCTCP and QBB, in addition to simulating the recently proposed pFabric.

This project is poorly documented.  However, the primary files of interest are ``scratch/oldi_sim.cc`` and  ``scratch/pFabric_sim.cc``.  These files run the workloads on configurable variants of DCTCP, QBB, and pFabric.  For an example on how to use the programs, see the ``run_exp.sh`` and ``run_pFabric.sh`` scripts.

If this program is used to generate results referenced in
publication, the authors ask that the publication cites ``Practical DCB for Improved Data Center Networks'', Infocom '14.  This program was originally used
to build the figures presented in that paper.
