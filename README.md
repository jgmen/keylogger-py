# Keylogger

> ⚠️ **Warning:** This project is intended **for educational purposes only**.
> I am **not responsible** for any misuse of this code. Do **not use** it to capture data from others without permission.

---
<img width="512" height="512" alt="logo" src="https://github.com/user-attachments/assets/ab405688-7c34-477a-a03c-192df375d5e6" />

## Description

This is a simple Python/C example that demonstrates how to:

- Collect basic system information
- Capture global keyboard input
- Decode key presses into readable text

---

## Example Usage

```python
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

# Type something...
time.sleep(10)

# Get keylogger buffer and decode to text
codes = kl.get_buffer()
text = "".join(chr(c) for c in codes)
print("Captured Keys:")
print(text)

# Stop keylogger
kl.stop()
del kl
```


## Next Goals
- **External Output:** Explore ways to send captured data to external sources:
  - Save to files
  - Send via HTTP requests
- **Data Analysis:** Process and analyze keystroke data
- **Snapshot:** Capture periodic screenshots for study purposes

## How to Compile and Run (Windows Only)

> ⚠️ **Note:** This project currently supports **Windows only**.
> Make sure you have **Python installed** and **Visual Studio Build Tools** available.

Clone the repository and navigate to the folder:

```bash
git clone https://github.com/jgmen/keylogger-py.git
cd keylogger-py
nmake build
nmake run
