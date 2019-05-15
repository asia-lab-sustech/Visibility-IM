import numpy as np
import matplotlib.pyplot as plt



network_distance = [371.7, 370.3, 372.8, 385.6, 383, 384.7]
network_visibility = [348.8, 351.1, 355.1, 367.5, 368.1, 371.4]

match_distance = [0.056099, 0.028301, 0.022702, 0.016797, 0.014998, 0.016101]
match_visibility = [0.030998, 0.018798, 0.016101, 0.013698, 0.010401, 0.011198]

size = [50000, 100000, 150000, 200000, 250000, 300000]
count = [453, 222, 148, 115, 89, 75]

plt.xlabel('Tile Count')
plt.ylabel('Interest Matching Time (ms)')

plt.plot(count, match_distance, '-o', label="tile distance")
plt.plot(count, match_visibility, '-^', label="triangle visibility-based")
plt.legend(loc='upper left')

plt.show()
