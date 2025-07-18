# HWID Spoofer for Windows 11

![Windows](https://img.shields.io/badge/Windows-11-blue?logo=windows)  
![Build](https://img.shields.io/badge/Build-22631-lightgrey)  
![License](https://img.shields.io/badge/License-MIT-green)

> **Temporary, memory-based HWID spoofing for research & educational use.**

⚠️ **WARNING**  
This tool violates Microsoft’s terms and most game anti-cheat policies. It can cause instability (BSODs) and may trigger bans.  
**Use only in virtual machines or controlled environments.**

---

## 📌 Overview

HWID Spoofer is a **proof-of-concept** for spoofing hardware identifiers (**disk serials & MAC addresses**) on Windows 11.

- ✅ **Disk Serial Spoofing** – Samsung/WD-like serials (e.g., `SAMSUNG_X7Y9Z1A2B3`)  
- ✅ **MAC Address Spoofing** – Randomized MACs (e.g., `1A:2B:3C:4D:5E:6F`)  
- ✅ **Memory-Based** – No permanent changes, revert on driver unload or restart  
- ✅ **Clean Console UI** – Cyan headers, green success, red errors  
- ✅ **Backup & Restore** – Saves original serials to `serial_backup.txt`  
- ✅ **Light Anti-Detection** – Inline hooks, no SSDT

---

## ✅ Requirements

- **Windows 11** (10.0.22631 Build 22631)  
- **Administrator rights** to run & load drivers  
- **Visual Studio 2022** + Desktop C++ workload  
- **Windows SDK 10.0.22621+** + **WDK**  
- Test signing enabled:

```cmd
bcdedit /set testsigning on
```

- *(Optional)* Virtual machine for safer testing  
- **DbgView** for kernel logs

---

## 🛠 Building

### 1️⃣ Clone the Repository

```cmd
git clone https://github.com/ravixdev/hwid-spoofer.git
cd hwid-spoofer
```

---

### 2️⃣ Build the User-Mode Application

1. Open `hwid_spoofer.cpp` in **Visual Studio 2022**  
2. Set build configuration → **x64 Release**  
3. Link required libraries:

```
advapi32.lib; iphlpapi.lib
```

4. Build → Output will be in:

```
x64\Release\hwid_spoofer.exe
```

✅ **Copy** `hwid_spoofer.exe` to a writable folder, e.g.:

```
C:\HWIDSpoofer
```

---

### 3️⃣ Build the Kernel-Mode Driver

1. Install **WDK for Visual Studio 2022**  
2. Create a new project → **Empty WDM Driver**  
3. Add `hwid_driver.c` under **Source Files**  
4. Set build configuration → **x64 Release**  
5. Configure:
   - **Target OS Version** → Windows 10+ (or Windows 11)  
   - **Driver Signing** → Test Sign Mode  
   - **C++ Standard** → C++17 or later

✅ **Output:**

```
x64\Release\HWIDSpoofer.sys
```

✅ **Copy** `HWIDSpoofer.sys` to:

```
C:\Windows\System32\drivers
```

---

### 4️⃣ Final File Layout

```
C:\HWIDSpoofer\
  ├── hwid_spoofer.exe
  └── spoofer_log.txt (created at runtime)

C:\Windows\System32\drivers\
  └── HWIDSpoofer.sys
```

---

## ▶️ Usage

### 1️⃣ Enable Test Signing Mode

```cmd
bcdedit /set testsigning on
```

*(If Secure Boot is enabled, temporarily disable it in BIOS. Re-enable after testing.)*

---

### 2️⃣ Load the Driver

```cmd
sc create HWIDSpoofer binPath= "C:\Windows\System32\drivers\HWIDSpoofer.sys" type= kernel
sc start HWIDSpoofer
```

✅ Verify driver status:

```cmd
sc query HWIDSpoofer
```

---

### 3️⃣ Run the Spoofer

```cmd
hwid_spoofer.exe   (Run as Administrator)
```

---

### 📖 Menu

```
===================================================
HWID Spoofer Loader for Windows 11 (Build 22631)
===================================================
1. Check Current HWIDs
2. Generate Spoofed HWIDs
3. Apply Spoofed HWIDs
4. Clear Spoofed HWIDs
Enter choice (1-4):
```

✅ **Verification**

- Disk:

```cmd
wmic diskdrive get serialnumber
```

- MAC:

```cmd
ipconfig /all
```

- Revert changes:

```cmd
sc stop HWIDSpoofer
```

*(or restart your system)*

---

## 🔧 Troubleshooting

**App closes instantly?**
- Check `spoofer_log.txt`
- Run as **Administrator**

**Driver fails to load?**
- Ensure **test signing is ON**
- Verify driver path & rebuild with correct WDK settings
- Use **DbgView** for kernel logs

**MAC spoofing not working?**
- Some adapters require additional logic for spoofing

---

## 🔍 Technical Details

- **User-mode (`hwid_spoofer.cpp`)**
  - Written in C++17 + WinAPI
  - Communicates with `\\.\HWIDSpoofer` via `DeviceIoControl`
  - Generates realistic Samsung/WD serials & MACs

- **Kernel-mode (`hwid_driver.c`)**
  - Hooks `IOCTL_STORAGE_QUERY_PROPERTY` (disk serials)
  - Hooks NDIS `OID_802_3_PERMANENT_ADDRESS` for MAC spoofing (simplified)
  - **Memory-only changes** revert on driver unload or restart

---

## ⚠️ Limitations

- Simplified MAC spoofing → may not work on all adapters  
- Advanced anti-cheats (e.g., Vanguard, BattlEye) **can detect** the driver  
- Kernel hooks may cause **BSODs** → test in a **VM**  
- Test signing required → production use would need an **EV certificate**

---

## 📜 Legal & Safety

- **Educational use only.**
- HWID spoofing violates Microsoft & game anti-cheat policies.
- Use at your own risk. The author is **not responsible** for bans, damage, or legal consequences.

---

## 🤝 Contributing

Pull requests & issues welcome!

- Please test changes in a VM
- Keep the **memory-based (non-persistent)** approach to avoid Windows activation issues

---

## 📄 License

This project is licensed under the **MIT License**.  
See [LICENSE](LICENSE) for details.

---

**Made by [ravix.dev](https://ravix.dev)**
