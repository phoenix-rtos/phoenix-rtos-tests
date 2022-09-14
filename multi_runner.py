import subprocess
import os

print('start')
i = 0
ret = 0 
while not ret and i < 25:
    i+=1
    print(f'Run nr {i}')
    ret = os.system('python3 runner.py -Tarmv7m4-stm32l4x6-nucleo -l debug')
print('end')

