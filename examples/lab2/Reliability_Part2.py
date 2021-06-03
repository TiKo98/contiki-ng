import pandas as pd
import matplotlib.pyplot as plt 
import statistics

def reliability(data):
    border_indices = []
    for i in range(len(data)):
        if data["Output"][i].find("Broadcast ledId") >= 0:
            border_indices.append(i)

    broadcasts = []
    for i in range(1, len(border_indices)):
        x = border_indices[i] - border_indices[i - 1] - 1
        broadcasts.append(x)
    return broadcasts

colnames = ['Time', 'Node ID', 'Output']

part1_9_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 9 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
broad_9 = reliability(part1_9_data)

part1_25_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 25 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
broad_25 = reliability(part1_25_data)

part1_49_data = pd.read_csv(r"D:\Programmierung\contiki-ng\examples\lab2\lab2 part2 49 nodes log only data.txt", engine='python', sep="\t", header=None, names=colnames)
broad_49 = reliability(part1_49_data)

rel_9 = [x / 9 * 100 for x in broad_9]
rel_25 = [x / 25 * 100 for x in broad_25]
rel_49 = [x / 49 * 100 for x in broad_49]

means = [statistics.mean(x) for x in [rel_9, rel_25, rel_49]]
uncertainty = [statistics.stdev(x) for x in [rel_9, rel_25, rel_49]]

labels = ['9 Nodes', '25 Nodes', '49 Nodes']
plt.xticks(range(len(means)), labels)
plt.ylim([94, 104])
plt.xlabel('Number of Nodes')
plt.ylabel('Mean Reliability in %')
plt.title('Reliability')
plt.bar(range(len(means)), means, yerr=uncertainty, capsize=10, width=0.3) 
plt.show()
