import pandas as pd
import datetime
import matplotlib.pyplot as plt 
import statistics
import numpy as np

def insert_time(timestring):
    timestring = "20.05.2021 " + timestring
    timeobject = datetime.datetime.strptime(timestring, '%d.%m.%Y %M:%S.%f')
    milisecs = timeobject.timestamp() * 1000
    return milisecs

def evaluate(data):
    counter = 0
    successes = 0
    time_differences = []

    for i in range(len(data)):

        sendstring = "Send value " + str(counter)
        receivestring = "Logged value " + str(counter) + " from mobile node"

        if data["Output"][i].find(sendstring) >= 0:
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


colnames = ['Time', 'Node ID', 'Output']

data_speed_15 = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part2 speed 15 log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=60)
data_speed_30 = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part2 speed 30 log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=60)
data_speed_40 = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part2 speed 40 log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=60)
data_speed_50 = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab3\lab3 part2 speed 50 log.txt", engine='python', sep="\t", header=None, names=colnames, skiprows=60)

rate_15, times_15 = evaluate(data_speed_15)
mean_15, dev_15 = time_stats(times_15)
rate_30, times_30 = evaluate(data_speed_30)
mean_30, dev_30 = time_stats(times_30)
rate_40, times_40 = evaluate(data_speed_40)
mean_40, dev_40 = time_stats(times_40)
rate_50, times_50 = evaluate(data_speed_50)
mean_50, dev_50 = time_stats(times_50)

labels = ['15', '30', '40', '50']

means = [mean_15, mean_30, mean_40, mean_50]
uncertainty = [dev_15, dev_30, dev_40, dev_50]
rates = [rate_15, rate_40, rate_30, rate_50]

# plt.xticks(range(len(means)), labels)
# plt.xlabel('Mobile Node Speed')
# plt.ylabel('Mean Latency in ms')
# plt.title('Latency')
# # plt.ylim(0, 100)
# plt.bar(range(len(means)), means, yerr=uncertainty, capsize=4, width=0.3) 
# # plt.plot(range(len(means)), means, 'bo') 
# plt.show()

plt.xticks(range(len(rates)), labels)
plt.xlabel('Mobile Node Speed')
plt.ylabel('Reliability in %')
plt.title('Reliability')
plt.ylim(0, 100)
plt.bar(range(len(rates)), rates, capsize=4, width=0.3) 
# plt.plot(range(len(means)), means, 'bo') 
plt.show()

# X_axis = np.arange(len(labels))

# plt.bar(X_axis - 0.2, rates, 0.4, label = 'Reliability in %')
# plt.bar(X_axis + 0.2, means, yerr=uncertainty, capsize=4, width=0.4, label = 'Latency in ms')
  
# plt.xticks(X_axis, labels)
# plt.xlabel("Mobile Node Speed")
# plt.ylabel("Latency in ms and Reliability in %")
# plt.title("Evaluation")
# plt.legend()
# plt.show()