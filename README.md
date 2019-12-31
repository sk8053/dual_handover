This is ns-3 code for implementing dual-connection based handover scheme in ultra-dense millimeter wave networks. The paper pertaining to this ns-3 code is "A Dual-Connection based Handover Scheme for Ultra-Dense Millimeter-Wave Cellular Networks", which is published in Globecomm 2019.

To confirm the results of the paper, put the bellow commands

./waf --run "scratch/mc-downlink -buildingTopology=building_topology.txt -nPacket=7142"

You can change the building density by text file, "building_topology.txt", which has 4000 blockages/𝑘𝑚^2.
The parameter, nPacktet, indicates the number of packets consisting of one file, and each packet size is 1400 byte. 

In order to get ns-3 code for previous handover scheme used in the paper, M. Polese, M. Giordani, M. Mezzavilla, S. Rangan, and M. Zorzi, “Improved handover through dual connectivity in 5G mmWave mobile networks,” IEEE J. Sel. Areas Commun., vol. 35, no. 9, pp. 2069–2084, 2017., use the link, https://github.com/sk8053/previous_handover.  The scratch file is used, and all settings including parameters and simulation scenarios are the same as above ones. 

