import time
import wmi

wmiInterface = wmi.WMI()

process_info = {}
Statistics = [0.0, 0]
time.sleep(10)
last = time.clock()
while True:  # Change the looping condition
    ps = wmiInterface.Win32_Process(name="UE4Editor.exe")
    for process in wmiInterface.Win32_Process(name="UE4Editor.exe"):
        id = process.ProcessID
        for p in wmiInterface.Win32_PerfRawData_PerfProc_Process(IDProcess=id):
            n1, d1 = int(p.PercentProcessorTime), int(p.Timestamp_Sys100NS)
            if id not in process_info:
                process_info[id] = (n1, d1)
                continue
            n0, d0 = process_info.get(id, (0, 0))
            try:
                percent_processor_time = (float(n1 - n0) / float(d1 - d0)) * 100.0
            except ZeroDivisionError:
                percent_processor_time = 0.0
            process_info[id] = (n1, d1)
            Statistics[0] = (Statistics[0] * Statistics[1] + percent_processor_time) / (Statistics[1] + 1)
            Statistics[1] += 1
            print(Statistics[1], Statistics[0], str(percent_processor_time))
            last = time.clock()
