import pandas as pd
import datetime
import matplotlib.pyplot as plt 
import statistics
import re

def insert_time(timestring):
    """Returns the time as millisecond float"""
    timestring = "20.05.2021 " + timestring
    timeobject = datetime.datetime.strptime(timestring, '%d.%m.%Y %M:%S.%f')
    milisecs = timeobject.timestamp() * 1000
    return milisecs

def received_number(data_string, max_Num):
    """Returns how many messages have been received in % of maximum possible"""
    return int(re.search(r'\d+', data_string).group()) / max_Num *100

def do_stuff(data, central_ID):
    id_string = "ID:" + str(central_ID)
    only_central = data[data["Node ID"] == id_string]
    only_central = only_central.reset_index(drop=True)

    reset_string = "Reset numMessagesRecievedInCurrentRound"
    start_string = "Thus received 1 values in this round"
    last_received_times = []
    first_received_times = []
    rel = []

    for i in range(len(only_central)):

        x = only_central["Output"][i]
        if x.find(reset_string) >= 0:
            last_received_times.append(insert_time(only_central["Time"][i - 2]))
            rel.append(received_number(only_central["Output"][i - 1], central_ID * 2 - 1))
        if x.find(start_string) >= 0:
            first_received_times.append(insert_time(only_central["Time"][i - 1]))

    first_received_times = first_received_times[0:60]

    time_differences = []
    for i in range(len(last_received_times)):
        d = last_received_times[i] - first_received_times[i]
        time_differences.append(d)
    
    mean_latency = statistics.mean(time_differences)
    # print("Mittelwert Latenz: " + str(mean_latency))
    stdev_latency = statistics.stdev(time_differences)
    # print("Unsicherheit Latenz: " + str(stdev_latency))
    mean_reliability = statistics.mean(rel)
    # print("Mittelwert Reliabilität: " + str(mean_reliability))
    stdev_reliability = statistics.stdev(rel)
    # print("Unsicherheit Reliabilität: " + str(stdev_reliability))

    return mean_latency, stdev_latency, mean_reliability, stdev_reliability


colnames = ['Time', 'Node ID', 'Output']

part3_9_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part4 9 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
latency_9, uncer_lat_9, reliability_9, uncer_rel_9 = do_stuff(part3_9_data, 5)
# print(latency_9)
# print(uncer_lat_9)
# print("")
# print(reliability_9)
# print(uncer_rel_9)

part3_25_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part4 25 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
latency_25, uncer_lat_25, reliability_25, uncer_rel_25 = do_stuff(part3_25_data, 13)

part3_49_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part4 49 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
latency_49, uncer_lat_49, reliability_49, uncer_rel_49 = do_stuff(part3_49_data, 25)

# means = [latency_9, latency_25, latency_49]

# labels = ['9 Nodes', '25 Nodes', '49 Nodes']
# plt.xticks(range(len(means)), labels)
# plt.xlabel('Numer of Nodes')
# plt.ylabel('Mean Latency in ms')
# plt.title('Latency')
# plt.bar(range(len(means)), means, width=0.3) 
# plt.show()

means = [reliability_9, reliability_25, reliability_49]
uncertainty = [uncer_rel_9, uncer_rel_25, uncer_rel_49]

labels = ['9 Nodes', '25 Nodes', '49 Nodes']
plt.xticks(range(len(means)), labels)
# plt.ylim([94, 104])
plt.xlabel('Number of Nodes')
plt.ylabel('Mean Reliability in %')
plt.title('Reliability')
plt.bar(range(len(means)), means, yerr=uncertainty, capsize=10, width=0.3) 
plt.show()
