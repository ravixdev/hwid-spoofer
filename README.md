HWID Spoofer for Windows 11
HWID Spoofer is a proof-of-concept tool for temporarily spoofing hardware identifiers (HWIDs) on Windows 11 (Version 10.0.22631 Build 22631). It spoofs disk serial numbers (Samsung and WD-compatible, e.g., for your Samsung SSD) and MAC addresses using a user-mode application (hwid_spoofer.exe) and a kernel-mode driver (HWIDSpoofer.sys). Changes are memory-based, reverting on driver unload or system restart, ensuring no persistent modifications that could disrupt Windows activation. The tool features a clean, color-coded console UI and is designed for educational purposes, such as testing or research.
⚠️ WARNING: This tool violates Microsoft’s terms of service and game anti-cheat policies, risking bans or service disruptions. Kernel-mode drivers can cause system instability (BSODs). Use only in controlled environments (e.g., virtual machines) and on non-critical accounts. The author is not responsible for any damage or consequences.
Features

Disk Serial Spoofing: Spoofs disk serials for PhysicalDriveX (e.g., SAMSUNG_X7Y9Z1A2B3 for Samsung SSDs).
MAC Address Spoofing: Spoofs the primary network adapter’s MAC address (e.g., 1A:2B:3C:4D:5E:6F).
Memory-Based: Changes are temporary, reverting on driver unload (sc stop HWIDSpoofer) or restart.
Clean UI: Console interface with cyan headers, green success messages, and red errors.
Error Handling: Robust checks for Administrator privileges, driver availability, and input validation, with logging to spoofer_log.txt.
Backup and Restore: Saves original disk serials to serial_backup.txt.
Anti-Detection: Uses lightweight inline hooking to minimize detection, though advanced anti-cheats (e.g., Vanguard, BattlEye) may detect the driver.

Prerequisites

Operating System: Windows 11.
Development Environment:
Visual Studio 2022 with Desktop Development with C++ workload.
Windows SDK (10.0.22621.0 or later).
Windows Driver Kit (WDK) for building HWIDSpoofer.sys.


Privileges: Administrator rights for running hwid_spoofer.exe and loading the driver.
Test Signing: Enabled via bcdedit /set testsigning on.
Optional: Virtual machine (e.g., VMware, VirtualBox) for safe testing.
Tools: DbgView for kernel logs.

Building the Project

Clone the Repository:
git clone https://github.com/ravixdev/hwid-spoofer.git
cd hwid-spoofer


Build User-Mode Application (hwid_spoofer.cpp):

Open hwid_spoofer.cpp in Visual Studio 2022.
Set configuration to x64 Release.
Link libraries:
Project Properties > Linker > Input > Additional Dependencies: advapi32.lib;iphlpapi.lib.


Build to generate hwid_spoofer.exe in the x64\Release folder.
Copy hwid_spoofer.exe to a writable directory (e.g., C:\HWIDSpoofer).


Build Kernel-Mode Driver (hwid_driver.c):

Install the WDK for Visual Studio 2022.
Create a new project: File > New > Project > Empty WDM Driver (name it HWIDSpoofer).
Add hwid_driver.c to Source Files.
Set configuration to x64 Release.
Configure:
Driver Settings > General > Target OS Version: Windows 10 and later (or Windows 11).
Driver Settings > Driver Signing > Sign Mode: Test Sign.
C/C++ > Language > C++ Language Standard: C++17 or later.


Build to generate HWIDSpoofer.sys in x64\Release.
Copy HWIDSpoofer.sys to C:\Windows\System32\drivers (or another directory, e.g., C:\HWIDSpoofer).



Installation

Enable Test Signing Mode:

Open an elevated Command Prompt (cmd as Administrator).
Run:bcdedit /set testsigning on


Restart your system.
Note: If Secure Boot is enabled, temporarily disable it in BIOS (re-enable after testing to avoid Microsoft service issues, e.g., Store, Xbox).


Load the Kernel Driver:

In an elevated Command Prompt, register the driver:sc create HWIDSpoofer binPath= "C:\Windows\System32\drivers\HWIDSpoofer.sys" type= kernel


Start the driver:sc start HWIDSpoofer


Verify:sc query HWIDSpoofer


If the driver fails to load, use DbgView to check kernel logs (e.g., “Failed to create device”).


Run the Application:

Right-click hwid_spoofer.exe > Run as administrator.
If the application closes immediately, check spoofer_log.txt (see Troubleshooting).



Usage

Launch the Application:

Run hwid_spoofer.exe as Administrator.
The UI displays:===================================================
HWID Spoofer Loader for Windows 11 (Build 22631)
===================================================
Instructions:
- Ensure test signing is enabled: 'bcdedit /set testsigning on' and restart.
- Load driver: 'sc create HWIDSpoofer binPath= "path\to\HWIDSpoofer.sys" type= kernel'
- Start driver: 'sc start HWIDSpoofer'
- Changes are memory-based and revert on restart.
- Use cautiously to avoid anti-cheat bans.
===================================================
1. Check Current HWIDs (Disk Serial & MAC)
2. Generate Spoofed HWIDs (Disk & MAC)
3. Apply Spoofed HWIDs
4. Clear Spoofed HWIDs
Enter choice (1-4):




Options:

Option 1: Check Current HWIDs:
Enter a drive number (e.g., 0 for your Samsung SSD).
Displays disk serial (e.g., S4XJ12345678) and MAC address (e.g., 00:1A:2B:3C:4D:5E).
Backs up serial to serial_backup.txt.


Option 2: Generate Spoofed HWIDs:
Generates Samsung (e.g., SAMSUNG_X7Y9Z1A2B3), WD (e.g., WD-WX9876543210), and MAC (e.g., 1A:2B:3C:4D:5E:6F) addresses.
Saves to spoofed_serials.txt.


Option 3: Apply Spoofed HWIDs:
Select a disk serial (Samsung recommended) and MAC spoofing option.
Applies via driver. Verify with Option 1 or wmic diskdrive get serialnumber (disk) and ipconfig /all (MAC).


Option 4: Clear Spoofed HWIDs:
Restores original HWIDs. Verify with Option 1.




Verification:

Disk Serial: Run wmic diskdrive get serialnumber to confirm spoofed serial.
MAC Address: Run ipconfig /all to verify spoofed MAC.
Reversion: Unload driver (sc stop HWIDSpoofer) or restart, then verify original HWIDs.



Technical Details

User-Mode Application (hwid_spoofer.cpp):

Written in C++ with Windows API.
Communicates with \\.\HWIDSpoofer via DeviceIoControl.
Generates realistic Samsung/WD serials and MAC addresses.
Logs errors to spoofer_log.txt for debugging.
Backs up registry serials to serial_backup.txt.


Kernel-Mode Driver (hwid_driver.c):

Written in C using WDK.
Hooks disk.sys for IOCTL_STORAGE_QUERY_PROPERTY (disk serials) and NDIS for OID_802_3_PERMANENT_ADDRESS (MAC addresses, simplified).
Uses inline hooking for lightweight, modern implementation.
Changes are memory-based, reverting on unload/restart.
Minimal anti-detection (no SSDT hooks) to reduce detection risk.


Anti-Detection:

Lightweight hooks minimize footprint.
No persistent changes to protect Windows activation.
Note: Advanced anti-cheats may detect the driver. Consider commercial spoofers for production use.



Troubleshooting

Application Closes Immediately:

Check spoofer_log.txt: Located in the same directory as hwid_spoofer.exe. Look for:[2025-07-17 19:49:12] Error: Not running as Administrator
[2025-07-17 19:49:12] Warning: Driver not loaded


Run as Administrator: Right-click hwid_spoofer.exe > Run as administrator.
Ensure Driver Is Loaded:
Verify test signing: bcdedit /set testsigning on, restart.
Load driver: sc create HWIDSpoofer binPath= "C:\Windows\System32\drivers\HWIDSpoofer.sys" type= kernel, then sc start HWIDSpoofer.
Check: sc query HWIDSpoofer.
Use DbgView for kernel logs if the driver fails.


Verify Build:
Ensure advapi32.lib and iphlpapi.lib are linked.
Target Windows SDK 10.0.22621.0 or later.
Build for x64 Release.


Invalid Input: Enter numeric values (e.g., 1 for menu, 0 for drive number).
Check Event Viewer: Open eventvwr > Windows Logs > Application/System for errors.


Driver Fails to Load:

Ensure HWIDSpoofer.sys is in C:\Windows\System32\drivers.
Verify test signing is enabled (bcdedit /enum).
Check DbgView for errors (e.g., “Failed to create device”).
Rebuild hwid_driver.c with correct WDK settings.


MAC Spoofing Not Working:

The NDIS hook is simplified and may not work for all network adapters. Adapter-specific logic may be needed.
Verify with ipconfig /all.



Safety Precautions

Test in a VM: Use a virtual machine (e.g., VMware with Windows 11) to avoid risking your main system.
Backup Windows License: Save your key: wmic path SoftwareLicensingService get OA3xOriginalProductKey.
Secure Boot: Re-enable after testing (msinfo32 to check).
Registry Backup: Use serial_backup.txt for manual restoration.
Anti-Cheat Caution: Test on a secondary game account to avoid bans.

Limitations

NDIS Hooking: Simplified MAC address spoofing may not work for all adapters. Full implementation requires adapter-specific logic.
Anti-Cheat Detection: The driver may be detected by advanced anti-cheats. Commercial spoofers with driver cloaking are more robust.
Stability: Kernel hooks risk BSODs. Test thoroughly.
Driver Signing: Requires test signing mode. Production use needs an EV certificate.

Legal Disclaimer
This project is for educational purposes only. Spoofing HWIDs violates Microsoft’s terms and game policies, risking bans or service disruptions. Use at your own risk, preferably in a controlled environment. The author is not responsible for any damage, bans, or legal consequences.
Contributing
Contributions are welcome! Please submit pull requests or issues on GitHub. Ensure changes are tested in a VM and maintain the memory-based, non-persistent approach to avoid system instability.
License
This project is licensed under the MIT License. See LICENSE for details.
