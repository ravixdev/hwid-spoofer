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

✅ **Disk Serial Spoofing** – Samsung/WD-like serials (e.g., `SAMSUNG_X7Y9Z1A2B3`)  
✅ **MAC Address Spoofing** – Randomized MACs (e.g., `1A:2B:3C:4D:5E:6F`)  
✅ **Memory-Based** – No permanent changes, revert on driver unload or restart  
✅ **Clean Console UI** – Cyan headers, green success, red errors  
✅ **Backup & Restore** – Saves original serials to `serial_backup.txt`  
✅ **Light Anti-Detection** – Inline hooks, no SSDT  

---

## ✅ Requirements  

- **Windows 11** (10.0.22631 Build 22631)  
- **Admin rights** to run & load drivers  
- **Visual Studio 2022** + Desktop C++ workload  
- **Windows SDK 10.0.22621+** + **WDK**  
- Test signing enabled →  
  ```cmd
  bcdedit /set testsigning on
