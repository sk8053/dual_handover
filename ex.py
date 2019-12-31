
# -*- coding: utf-8 -*-
 
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style
 
style.use('fivethirtyeight')
def getList(a):
	xs =[] ; ys =[]
	#a = a.split('\n')
	while True:
		line = a.readline()
		x,y = line.split(',')
		xs = x.append(x)
		ys = y.append(y)
		if not line: break;
	return xs,ys
	 
fig = plt.figure()
ax1 = fig.add_subplot(1,1,1)
 
def animate(i):
	graph_data = open('example.txt','r').read()
	xs,ys = getList(graph_data)
	ax1.clear()   
	ax1.plot(xs, ys)
 
# -- 실시간으로 텍스트 파일 변경 정보 가져와서 출력함
#    파일을 열고 계속 정보를 추가하면 그래프가 움직임.
ani = animation.FuncAnimation(fig, animate, interval=1000)
plt.show()
