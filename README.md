#  Dual-Connection based Handover Scheme for Ultra-Dense Millimeter-Wave Cellular Networks

Mobile users in an ultra-dense millimeter-wave cellular network experience handover events more frequently than in conventional networks, which results in increased service
interruption time and performance degradation due to blockages.
Multi-connectivity has been proposed to resolve this, and it also
extends the coverage of millimeter-wave communications. In this
paper, we propose a dual-connection based handover scheme for
mobile UEs in an environment where they are connected simultaneously with two millimeter-wave cells to overcome frequent
handover problems. This scheme allows a mobile UE to choose
its serving link between the two mmWave connections according
to the measured SINRs and then the corresponding base stations
may forward duplicate packets to the UE. We compare our dualconnection based scheme with a conventional single-connection
based scheme through ns-3 simulation. The simulation results
show that the proposed scheme significantly reduces handover
rate and delay. Therefore, we argue that the dual-connection
based scheme helps mobile users achieve performance goals they
require in ultra-dense cellular environments

* Seongjoon Kang, Siyoung Choi, Goodsol Lee, and Saewoong Bahk, https://arxiv.org/pdf/2107.04526.pdf
* This paper is published in IEEE Globecomm 2019

To confirm some results, put the bellow commands
./waf --run "scratch/mc-downlink -buildingTopology=building_topology.txt -nPacket=7142"

You can change the building density by text file, "building_topology.txt", which has 4000 blockages/𝑘𝑚^2.
The parameter, nPacktet, indicates the number of packets consisting of one file, and each packet size is 1400 byte. 
