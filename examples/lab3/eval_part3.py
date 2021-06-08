import pandas as pd
import datetime
import matplotlib.pyplot as plt 
import statistics
import numpy as np

def insert_time(timestring):
    """Takes a cooja timestamp as string, returns time in milliseconds"""
    timestring = "20.05.2021 " + timestring
    timeobject = datetime.datetime.strptime(timestring, '%d.%m.%Y %M:%S.%f')
    milisecs = timeobject.timestamp() * 1000
    return milisecs

def evaluate(data, node_id):
    """Takes a dataframe and an integer to calculate reliability and Latency"""
    id_string = "ID:" + str(node_id)    # Node ID as it is in the df column
    counter = 0                         # 0 Packets send yet
    successes = 0                       # 0 Pacets received yet
    time_differences = []               # no Packets received or send - no time difference between sening and receiving

    for i in range(len(data)):          # go through data

        sendstring = "Send value " + str(counter)
        receivestring = "Logged value " + str(counter) + " from mobile node " + str(node_id)

        if data["Output"][i].find(sendstring) >= 0 and data["Node ID"][i] == id_string:
            counter += 1
            for j in range(1, 11):
                index = min((i + j, len(data) - 1))
                if data["Output"][index].find(receivestring) >= 0:
                    send_time = insert_time(data["Time"][i])
                    receive_time = insert_time(data["Time"][index])
                    time_differences.append(receive_time - send_time)
                    successes += 1

    success_rate = successes / (counter) * 100

    return success_rate, time_differences

def time_stats(times):
    mean = statistics.mean(times)
    deviation = statistics.stdev(times)
    return mean, deviation

def deep_evaluate(data, mobile_nodes):
    """
    Evaluate mulitple mobile nodes
    data: Data read from cooja log into pd dataframe
    mobile_nodes: number of mobile nodes
    """
    rates = []
    times = []

    for i in range(mobile_nodes):
        r, t = evaluate(data, i + 6)
        rates.append(r)
        times.append(t)

    mean_rates = statistics.mean(rates)
    dev_rates = statistics.stdev(rates)

    times = [j for sub in times for j in sub]

    mean_latency = statistics.mean(times)
    dev_latency = statistics.stdev(times)

    return (mean_rates, dev_rates), (mean_latency, dev_latency)

###################################################
# Load Data and evaluate
colnames = ['Time', 'Node ID', 'Output']

data_2_nodes = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part3 2 mobile nodes log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=70)
data_4_nodes = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part3 4 mobile nodes log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=90)
data_6_nodes = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part3 6 mobile nodes log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=110)

rate_2nodes, latency_2nodes = deep_evaluate(data_2_nodes, 2)
rate_4nodes, latency_4nodes = deep_evaluate(data_4_nodes, 2)
rate_6nodes, latency_6nodes = deep_evaluate(data_6_nodes, 2)

####################################################
# Plot stuff
labels = ['2', '4', '6']

mean_rates = [rate_2nodes[0], rate_4nodes[0], rate_6nodes[0]]
dev_rates = [rate_2nodes[1], rate_4nodes[1], rate_6nodes[1]]

mean_lats = [latency_2nodes[0], latency_4nodes[0], latency_6nodes[0]]
dev_lats = [latency_2nodes[1], latency_4nodes[1], latency_6nodes[1]]

plt.title('Latency')
plt.xlabel('Number of mobile nodes')
plt.ylabel('Mean Latency in ms')
plt.xticks(range(len(mean_lats)), labels)
plt.bar(range(len(mean_lats)), mean_lats, yerr=dev_lats, capsize=4, width=0.3) 
plt.show()

plt.title('Reliability')
plt.xlabel('Number of mobile nodes')
plt.ylabel('Reliability in %')
plt.xticks(range(len(mean_rates)), labels)
plt.ylim(0, 100)
plt.bar(range(len(mean_rates)), mean_rates, yerr=dev_rates, capsize=4, width=0.3) 
plt.show()