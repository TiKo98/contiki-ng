import pandas as pd
import datetime
import matplotlib.pyplot as plt 
import statistics

def insert_time(timestring):
    timestring = "20.05.2021 " + timestring
    timeobject = datetime.datetime.strptime(timestring, '%d.%m.%Y %M:%S.%f')
    milisecs = timeobject.timestamp() * 1000
    return milisecs

colnames = ['Time', 'Node ID', 'Output']
part1_9_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 9 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)

start_times_9 = []
end_times_9 = []

for i in range(len(part1_9_data)):
    if part1_9_data["Output"][i].find("Broadcast ledId") >= 0:
        start_times_9.append(insert_time(part1_9_data["Time"][i]))
    if part1_9_data["Output"][i].find("Broadcast ledId") >= 0 and i > 0:
        end_times_9.append(insert_time(part1_9_data["Time"][i - 1]))

end_times_9.append(insert_time(part1_9_data["Time"][602]))

time_differences_9 = []
for i in range(len(start_times_9)):
    d = end_times_9[i] - start_times_9[i]
    time_differences_9.append(d)

mean_time_9 = statistics.mean(time_differences_9)
stdev_time_9 = statistics.stdev(time_differences_9)

#############################################################

part1_25_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 25 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)

start_times_25 = []
end_times_25 = []

for i in range(len(part1_25_data)):
    if part1_25_data["Output"][i].find("Broadcast ledId") >= 0:
        start_times_25.append(insert_time(part1_25_data["Time"][i]))
    if part1_25_data["Output"][i].find("Broadcast ledId") >= 0 and i > 0:
        end_times_25.append(insert_time(part1_25_data["Time"][i - 1]))

end_times_25.append(insert_time(part1_25_data["Time"][1562]))

time_differences_25 = []
for i in range(len(start_times_25)):
    d = end_times_25[i] - start_times_25[i]
    time_differences_25.append(d)

mean_time_25 = statistics.mean(time_differences_25)
stdev_time_25 = statistics.stdev(time_differences_25)

#############################################################

part1_49_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 49 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)

start_times_49 = []
end_times_49 = []

for i in range(len(part1_49_data)):
    if part1_49_data["Output"][i].find("Broadcast ledId") >= 0:
        start_times_49.append(insert_time(part1_49_data["Time"][i]))
    if part1_49_data["Output"][i].find("Broadcast ledId") >= 0 and i > 0:
        end_times_49.append(insert_time(part1_49_data["Time"][i - 1]))

end_times_49.append(insert_time(part1_49_data["Time"][3035]))

time_differences_49 = []
for i in range(len(start_times_49)):
    d = end_times_49[i] - start_times_49[i]
    time_differences_49.append(d)

mean_time_49 = statistics.mean(time_differences_49)
stdev_time_49 = statistics.stdev(time_differences_49)

print(mean_time_9)
print(stdev_time_9)
print("")
print(mean_time_25)
print(stdev_time_25)
print("")
print(mean_time_49)
print(stdev_time_49)

means = [mean_time_9, mean_time_25, mean_time_49]
uncertainty = [stdev_time_9, stdev_time_25, stdev_time_49]

labels = ['9 Nodes', '25 Nodes', '49 Nodes']
plt.xticks(range(len(means)), labels)
plt.xlabel('Numer of Nodes')
plt.ylabel('Mean Latency in ms')
plt.title('Latency')
# plt.bar(range(len(means)), means, yerr=uncertainty, capsize=10, width=0.3) 
plt.plot(range(len(means)), means, 'bo') 
plt.show()
