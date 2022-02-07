import pandas as pd 
import matplotlib.pyplot as plt

data = pd.read_csv('data.csv')
print(data.shape)

data = data.to_numpy()[1:,1:]

Probe=2000
Range=800

plt.imshow(data[Probe:Probe+Range].T, cmap='gray_r')
plt.savefig('plot.png')