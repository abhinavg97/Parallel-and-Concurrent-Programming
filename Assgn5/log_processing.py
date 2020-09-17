import os
import subprocess
import string

f = open("runs.log", "r")

lines = f.readlines()

dict={}
dict2={}
count={}

for i in range(len(lines)):
	line = lines[i].split()

	if(i%2==0):
		threads = line[1]
		type1 = line[2]

		if(type1 not in dict):
			dict[type1] = {}
			count[type1] = {}

		if(threads not in dict[type1]):
			dict[type1][threads] = int(line[3])
			count[type1][threads] = 1
			
		else:
			dict[type1][threads] += int(line[3])
			count[type1][threads] += 1

	else:
		if(type1 not in dict2):
			dict2[type1] = {}
		if(threads not in dict2[type1]):
			dict2[type1][threads] = int(line[2])
		else:
			dict2[type1][threads] += int(line[2])

f.close()

for type1 in dict:
	for threads in dict[type1]:
		dict[type1][threads] = dict[type1][threads]/count[type1][threads] 

for type1 in dict2:
	for threads in dict2[type1]:
		dict2[type1][threads] = dict2[type1][threads]/count[type1][threads] 


f = open("results.log", "w")



for type1 in sorted(dict.keys()):
	f.write("type = "+str(type1)+"\n")
	for threads in sorted(dict[type1].keys()):
		f.write(str(threads) + "\t" + str(dict[type1][threads]) + "\n")

f.write("\n\nWorst case average:\n\n")
for type1 in sorted(dict2.keys()):
	f.write("type = "+str(type1)+"\n")
	for threads in sorted(dict2[type1].keys()):
		f.write(str(threads) + "\t" + str(dict2[type1][threads]) + "\n")

f.close()