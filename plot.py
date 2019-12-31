import matplotlib.pyplot as plt
#import pandas as pd
import matplotlib.animation as animation
from matplotlib import style

def getList(a):
	x = []; y =[] 
	while True:
		line = a.readline()
		line = line.split()
		if not line : break
		x.append(float(line[0]))
		y.append(float(line[1]))
	return x,y

fig = plt.figure(figsize = (8,6))
ax1 = fig.add_subplot(2,2,1)
ax2 = fig.add_subplot(2,2,2)
ax3 = fig.add_subplot(2,2,3)
ax4 = fig.add_subplot(2,2,4)
def animate(i):
	a1 = open ("tcp_cwnd_ue1.txt", 'r')
	a2 = open ("tcp_rtt_ue1.txt", 'r')
	a3 = open ("rlcUmTx_queue_senb3_ue1_bearer1.txt",'r')
	a3_1 = open("rlcUmLowLatTx_queue_menb_ue1_bearer1.txt",'r')
	
	a4_1 = open ( "rlc_Tput_menb_ue1_bearer_1.txt", 'r')
	a4_2 = open ( "rlc_Tput_senb_ue1_bearer_1.txt",'r')
	a4_3 = open ("tcp_throughput_ue1.txt",'r')
	
	x = []; y = []
	x1 = []; y1= []
	x2 = []; y2 =[]
	x2_1 = [] ; y2_1 = []
	x3_1 = [] ; y3_1 = []
	x3_2 = [] ; y3_2 = []
	x3_3 = [] ; y3_3 = []
	
	(x,y) = getList(a1)
	(x1,y1) = getList(a2)

	(x2,y2) = getList(a3)
	(x2_1,y2_1) = getList(a3_1)
	
	(x3_1,y3_1) = getList(a4_1)
	(x3_2,y3_2) = getList(a4_2)
	(x3_3,y3_3) = getList(a4_3)

	ax1.clear()
	ax1.plot(x,y,label="TCP Cwnd")
	ax1.grid()
	ax1.legend(loc=2)
	#ax1.set_xlabel("time(s)")
	ax1.set_ylabel("TCP Cwnd Size(Byte)")
	ax1.set_title ("TCP Cwnd Variation")
	
	ax2.clear()
	ax2.plot(x1,y1, label="TCP RTT")
	ax2.legend(loc=2)
	ax2.grid()
	ax2.set_title ("TCP RTT")
	#ax2.set_xlabel("time(s)")
	ax2.set_ylabel("TCP RTT(s)")


	ax3.clear()
	ax3.plot(x2,y2,'r-',label = "SeNB RLC Queue Size")
	ax3.plot (x2_1,y2_1,'g:', label = "MeNB RLC Queue Size")
	ax3.legend(loc=2)
	ax3.grid()
	ax3.set_title("RLC Queue Size Variation")
	ax3.set_xlabel("time(s)")
	ax3.set_ylabel("Queue Size (byte)")
	
	
	ax4.clear()
	ax4.plot(x3_1,y3_1,label = "LTE Throughput" )
	ax4.plot(x3_2, y3_2, label = "mmWave Throughput")
	ax4.plot(x3_3, y3_3, label = "TCP Thoughput")
	ax4.legend(loc=1)
	ax4.grid()
	ax4.set_title("Total and Path Throughput")
	ax4.set_xlabel("time(s)")
	ax4.set_ylabel("Throughput(Mbps)")



ani = animation.FuncAnimation(fig, animate, interval=1000)

plt.show()

