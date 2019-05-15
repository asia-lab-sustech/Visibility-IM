import psutil
p = psutil.Process(8012)
while True:
    print(p.cpu_percent())