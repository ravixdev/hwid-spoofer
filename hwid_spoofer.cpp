#include <iostream>
#include <windows.h>
#include <winioctl.h>
#include <string>
#include <random>
#include <iomanip>
#include <winreg.h>
#include <fstream>
#include <iphlpapi.h>
#include <sstream>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define IOCTL_SET_SPOOFED_DISK_SERIAL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLEAR_SPOOFED_DISK_SERIAL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_SPOOFED_MAC_ADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CLEAR_SPOOFED_MAC_ADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Console color codes
#define COLOR_GREEN "\x1B[32m"
#define COLOR_RED "\x1B[31m"
#define COLOR_CYAN "\x1B[36m"
#define COLOR_RESET "\x1B[0m"

// Log file
const std::string LOG_FILE = "spoofer_log.txt";

// Function to log messages
void LogMessage(const std::string& message) {
    std::ofstream logFile(LOG_FILE, std::ios::app);
    SYSTEMTIME st;
    GetLocalTime(&st);
    logFile << "[" << st.wYear << "-" << st.wMonth << "-" << st.wDay << " "
        << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] "
        << message << "\n";
    logFile.close();
}

// Function to set console color
void SetConsoleColor(const char* color) {
    std::cout << color;
}

// Function to check if running as Administrator
bool IsElevated() {
    BOOL isElevated = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isElevated = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return isElevated;
}

// Function to check if driver is loaded
bool IsDriverLoaded() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\HWIDSpoofer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
        return true;
    }
    return false;
}

// Function to get disk serial number
std::string GetDiskSerialNumber(int driveNumber) {
    std::wstring drivePath = L"\\\\.\\PhysicalDrive" + std::to_wstring(driveNumber);
    HANDLE hDevice = CreateFileW(drivePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::string error = "Error: Cannot open disk " + std::to_string(driveNumber) + " (Run as Administrator)";
        LogMessage(error);
        return error;
    }

    STORAGE_PROPERTY_QUERY query = { StorageDeviceProperty, PropertyStandardQuery };
    STORAGE_DEVICE_DESCRIPTOR* descriptor = (STORAGE_DEVICE_DESCRIPTOR*)malloc(1024);
    if (!descriptor) {
        CloseHandle(hDevice);
        LogMessage("Error: Failed to allocate memory for descriptor");
        return "Error: Memory allocation failed";
    }

    DWORD bytesReturned;
    if (!DeviceIoControl(hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        descriptor,
        1024,
        &bytesReturned,
        NULL)) {
        free(descriptor);
        CloseHandle(hDevice);
        std::string error = "Error: Failed to query disk " + std::to_string(driveNumber) + " serial";
        LogMessage(error);
        return error;
    }

    std::string serialNumber;
    if (descriptor->SerialNumberOffset != 0) {
        serialNumber = std::string((char*)descriptor + descriptor->SerialNumberOffset);
    }
    else {
        serialNumber = "Unknown";
    }

    free(descriptor);
    CloseHandle(hDevice);
    LogMessage("Retrieved disk serial for Drive" + std::to_string(driveNumber) + ": " + serialNumber);
    return serialNumber;
}

// Function to get MAC address
std::string GetMacAddress() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufferSize = sizeof(adapterInfo);
    if (GetAdaptersInfo(adapterInfo, &bufferSize) != ERROR_SUCCESS) {
        LogMessage("Error: Failed to get MAC address");
        return "Error: Failed to get MAC address";
    }

    std::stringstream ss;
    for (int i = 0; i < (int)adapterInfo[0].AddressLength; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)adapterInfo[0].Address[i];
        if (i < (int)adapterInfo[0].AddressLength - 1) ss << ":";
    }
    std::string mac = ss.str();
    LogMessage("Retrieved MAC address: " + mac);
    return mac;
}

// Function to generate a random Samsung-compatible serial
std::string GenerateSamsungSerial() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> disAlpha(0, 25);

    std::string prefix = "SAMSUNG_";
    std::stringstream ss;

    for (int i = 0; i < 8; ++i) {
        if (i % 2 == 0) {
            ss << (char)('A' + disAlpha(gen));
        }
        else {
            ss << std::hex << dis(gen);
        }
    }

    std::string serial = prefix + ss.str();
    LogMessage("Generated Samsung serial: " + serial);
    return serial;
}

// Function to generate a random WD-compatible serial
std::string GenerateWDSerial() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::string prefix = "WD-WX";
    std::stringstream ss;

    ss << std::hex << std::setw(10) << std::setfill('0') << dis(gen) * 0x100000000 + dis(gen);

    std::string serial = prefix + ss.str();
    LogMessage("Generated WD serial: " + serial);
    return serial;
}

// Function to generate a random MAC address
std::string GenerateMacAddress() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 6; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i < 5) ss << ":";
    }
    std::string mac = ss.str();
    LogMessage("Generated MAC address: " + mac);
    return mac;
}

// Function to backup registry serial
bool BackupRegistrySerial(int driveNumber, std::string& originalSerial, const std::string& backupFile) {
    HKEY hKey;
    std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\disk\\Enum";
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);

    if (result != ERROR_SUCCESS) {
        std::string error = "Error: Cannot open registry key (Run as Administrator)";
        LogMessage(error);
        std::cout << COLOR_RED << error << COLOR_RESET << "\n";
        return false;
    }

    char buffer[256];
    DWORD bufferSize = sizeof(buffer);
    std::string valueName = std::to_string(driveNumber);
    result = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)buffer, &bufferSize);

    if (result == ERROR_SUCCESS) {
        originalSerial = std::string(buffer);
        std::ofstream outFile(backupFile, std::ios::app);
        outFile << "Drive" << driveNumber << ": " << originalSerial << "\n";
        outFile.close();
        LogMessage("Backed up registry serial for Drive" + std::to_string(driveNumber) + ": " + originalSerial);
    }
    else {
        originalSerial = "Unknown";
        LogMessage("Failed to back up registry serial for Drive" + std::to_string(driveNumber));
    }

    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

// Function to apply spoofed disk serial via driver
bool ApplySpoofedDiskSerial(const std::string& spoofedSerial) {
    HANDLE hDevice = CreateFileW(L"\\\\.\\HWIDSpoofer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::string error = "Error: Cannot open driver (Ensure HWIDSpoofer.sys is loaded)";
        LogMessage(error);
        std::cout << COLOR_RED << error << COLOR_RESET << "\n";
        return false;
    }

    DWORD bytesReturned;
    bool success = DeviceIoControl(hDevice,
        IOCTL_SET_SPOOFED_DISK_SERIAL,
        (LPVOID)spoofedSerial.c_str(),
        spoofedSerial.length() + 1,
        NULL,
        0,
        &bytesReturned,
        NULL);

    CloseHandle(hDevice);
    if (success) {
        LogMessage("Applied spoofed disk serial: " + spoofedSerial);
        std::cout << COLOR_GREEN << "Spoofed disk serial applied: " << spoofedSerial << COLOR_RESET << "\n";
        return true;
    }
    else {
        LogMessage("Failed to apply spoofed disk serial: " + spoofedSerial);
        std::cout << COLOR_RED << "Error: Failed to apply spoofed disk serial" << COLOR_RESET << "\n";
        return false;
    }
}

// Function to apply spoofed MAC address via driver
bool ApplySpoofedMacAddress(const std::string& spoofedMac) {
    HANDLE hDevice = CreateFileW(L"\\\\.\\HWIDSpoofer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::string error = "Error: Cannot open driver (Ensure HWIDSpoofer.sys is loaded)";
        LogMessage(error);
        std::cout << COLOR_RED << error << COLOR_RESET << "\n";
        return false;
    }

    DWORD bytesReturned;
    bool success = DeviceIoControl(hDevice,
        IOCTL_SET_SPOOFED_MAC_ADDRESS,
        (LPVOID)spoofedMac.c_str(),
        spoofedMac.length() + 1,
        NULL,
        0,
        &bytesReturned,
        NULL);

    CloseHandle(hDevice);
    if (success) {
        LogMessage("Applied spoofed MAC address: " + spoofedMac);
        std::cout << COLOR_GREEN << "Spoofed MAC address applied: " << spoofedMac << COLOR_RESET << "\n";
        return true;
    }
    else {
        LogMessage("Failed to apply spoofed MAC address: " + spoofedMac);
        std::cout << COLOR_RED << "Error: Failed to apply spoofed MAC address" << COLOR_RESET << "\n";
        return false;
    }
}

// Function to clear spoofed serials
bool ClearSpoofedSerials() {
    HANDLE hDevice = CreateFileW(L"\\\\.\\HWIDSpoofer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        std::string error = "Error: Cannot open driver (Ensure HWIDSpoofer.sys is loaded)";
        LogMessage(error);
        std::cout << COLOR_RED << error << COLOR_RESET << "\n";
        return false;
    }

    DWORD bytesReturned;
    bool success1 = DeviceIoControl(hDevice,
        IOCTL_CLEAR_SPOOFED_DISK_SERIAL,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL);

    bool success2 = DeviceIoControl(hDevice,
        IOCTL_CLEAR_SPOOFED_MAC_ADDRESS,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL);

    CloseHandle(hDevice);
    if (success1 && success2) {
        LogMessage("Cleared spoofed disk and MAC serials");
        std::cout << COLOR_GREEN << "Spoofed serials cleared" << COLOR_RESET << "\n";
        return true;
    }
    else {
        LogMessage("Failed to clear spoofed serials");
        std::cout << COLOR_RED << "Error: Failed to clear spoofed serials" << COLOR_RESET << "\n";
        return false;
    }
}

// Function to save generated serials
void SaveSerialsToFile(const std::string& samsungSerial, const std::string& wdSerial, const std::string& macAddress, const std::string& filename) {
    std::ofstream outFile(filename);
    outFile << "Generated Samsung Serial: " << samsungSerial << "\n";
    outFile << "Generated WD Serial: " << wdSerial << "\n";
    outFile << "Generated MAC Address: " << macAddress << "\n";
    outFile.close();
    LogMessage("Saved serials to " + filename);
    std::cout << COLOR_GREEN << "Serials saved to " << filename << COLOR_RESET << "\n";
}

// Function to validate numeric input
bool GetNumericInput(int& value) {
    std::string input;
    std::getline(std::cin, input);
    std::stringstream ss(input);
    if (ss >> value && ss.eof()) {
        return true;
    }
    LogMessage("Invalid numeric input: " + input);
    return false;
}

int main() {
    // Clear log file at start
    std::ofstream logFile(LOG_FILE, std::ios::trunc);
    logFile.close();
    LogMessage("HWID Spoofer started");

    // Check Administrator privileges
    if (!IsElevated()) {
        LogMessage("Error: Not running as Administrator");
        SetConsoleColor(COLOR_RED);
        std::cout << "Error: This program must be run as Administrator.\n";
        SetConsoleColor(COLOR_RESET);
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    // Check driver availability
    bool driverLoaded = IsDriverLoaded();
    if (!driverLoaded) {
        LogMessage("Warning: Driver not loaded");
        SetConsoleColor(COLOR_RED);
        std::cout << "Warning: HWIDSpoofer.sys not loaded. Some features may not work.\n";
        SetConsoleColor(COLOR_RESET);
    }

    // Display UI
    SetConsoleColor(COLOR_CYAN);
    std::cout << "===================================================\n";
    std::cout << "HWID Spoofer Loader for Windows 11 (Build 22631)\n";
    std::cout << "===================================================\n";
    SetConsoleColor(COLOR_RESET);
    std::cout << "Instructions:\n";
    std::cout << "- Ensure test signing is enabled: 'bcdedit /set testsigning on' and restart.\n";
    std::cout << "- Load driver: 'sc create HWIDSpoofer binPath= \"path\\to\\HWIDSpoofer.sys\" type= kernel'\n";
    std::cout << "- Start driver: 'sc start HWIDSpoofer'\n";
    std::cout << "- Changes are memory-based and revert on restart.\n";
    std::cout << "- Use cautiously to avoid anti-cheat bans.\n";
    std::cout << "===================================================\n";
    std::cout << "1. Check Current HWIDs (Disk Serial & MAC)\n";
    std::cout << "2. Generate Spoofed HWIDs (Disk & MAC)\n";
    std::cout << "3. Apply Spoofed HWIDs\n";
    std::cout << "4. Clear Spoofed HWIDs\n";
    std::cout << "Enter choice (1-4): ";

    int choice;
    if (!GetNumericInput(choice)) {
        LogMessage("Invalid choice input");
        std::cout << COLOR_RED << "Invalid input. Please enter a number." << COLOR_RESET << "\n";
        std::cout << "Press Enter to exit...";
        std::cin.ignore();
        std::cin.get();
        return 1;
    }

    static std::string originalSerials[10];
    static std::string spoofedSamsungSerial;
    static std::string spoofedWDSerial;
    static std::string spoofedMacAddress;
    const std::string backupFile = "serial_backup.txt";
    const std::string outputFile = "spoofed_serials.txt";

    try {
        if (choice == 1) {
            SetConsoleColor(COLOR_CYAN);
            std::cout << "\n[Checking HWIDs]\n" << COLOR_RESET;
            std::cout << "Enter drive number (0 for primary drive, try 0-3 for others): ";
            int driveNumber;
            if (!GetNumericInput(driveNumber)) {
                throw std::runtime_error("Invalid drive number input");
            }
            std::cout << "Disk Serial (API) for Drive" << driveNumber << ": " << GetDiskSerialNumber(driveNumber) << "\n";
            if (BackupRegistrySerial(driveNumber, originalSerials[driveNumber], backupFile)) {
                std::cout << "Disk Serial (Registry) for Drive" << driveNumber << ": " << originalSerials[driveNumber] << "\n";
                std::cout << COLOR_GREEN << "Backed up to " << backupFile << COLOR_RESET << "\n";
            }
            std::cout << "MAC Address: " << GetMacAddress() << "\n";
        }
        else if (choice == 2) {
            SetConsoleColor(COLOR_CYAN);
            std::cout << "\n[Generating Spoofed HWIDs]\n" << COLOR_RESET;
            spoofedSamsungSerial = GenerateSamsungSerial();
            spoofedWDSerial = GenerateWDSerial();
            spoofedMacAddress = GenerateMacAddress();
            std::cout << "Generated Samsung Serial: " << spoofedSamsungSerial << "\n";
            std::cout << "Generated WD Serial: " << spoofedWDSerial << "\n";
            std::cout << "Generated MAC Address: " << spoofedMacAddress << "\n";
            SaveSerialsToFile(spoofedSamsungSerial, spoofedWDSerial, spoofedMacAddress, outputFile);
            std::cout << "Use Option 3 to apply via driver.\n";
        }
        else if (choice == 3) {
            if (spoofedSamsungSerial.empty() && spoofedWDSerial.empty() && spoofedMacAddress.empty()) {
                throw std::runtime_error("No spoofed HWIDs generated");
            }
            if (!driverLoaded) {
                throw std::runtime_error("Driver not loaded for spoofing");
            }
            SetConsoleColor(COLOR_CYAN);
            std::cout << "\n[Applying Spoofed HWIDs]\n" << COLOR_RESET;
            std::cout << "Choose disk serial to apply:\n";
            std::cout << "1. Samsung Serial: " << spoofedSamsungSerial << "\n";
            std::cout << "2. WD Serial: " << spoofedWDSerial << "\n";
            std::cout << "Enter choice (1 or 2): ";
            int serialChoice;
            if (!GetNumericInput(serialChoice) || (serialChoice != 1 && serialChoice != 2)) {
                throw std::runtime_error("Invalid disk serial choice");
            }
            std::string selectedSerial = (serialChoice == 1) ? spoofedSamsungSerial : spoofedWDSerial;

            if (originalSerials[0].empty()) {
                if (!BackupRegistrySerial(0, originalSerials[0], backupFile)) {
                    std::cout << COLOR_RED << "Warning: Could not backup original serial." << COLOR_RESET << "\n";
                }
            }

            std::cout << "Apply MAC address (" << spoofedMacAddress << ")? (1 for Yes, 0 for No): ";
            int macChoice;
            if (!GetNumericInput(macChoice) || (macChoice != 0 && macChoice != 1)) {
                throw std::runtime_error("Invalid MAC choice");
            }

            if (ApplySpoofedDiskSerial(selectedSerial)) {
                std::cout << "Disk serial applied. Verify with Option 1.\n";
            }
            if (macChoice == 1 && ApplySpoofedMacAddress(spoofedMacAddress)) {
                std::cout << "MAC address applied. Verify manually (e.g., 'ipconfig /all').\n";
            }
            std::cout << COLOR_GREEN << "Changes revert on driver unload or restart." << COLOR_RESET << "\n";
        }
        else if (choice == 4) {
            if (!driverLoaded) {
                throw std::runtime_error("Driver not loaded for clearing spoofed HWIDs");
            }
            SetConsoleColor(COLOR_CYAN);
            std::cout << "\n[Clearing Spoofed HWIDs]\n" << COLOR_RESET;
            if (ClearSpoofedSerials()) {
                std::cout << "Verify with Option 1 to confirm original HWIDs.\n";
            }
        }
        else {
            throw std::runtime_error("Invalid menu choice");
        }
    }
    catch (const std::exception& e) {
        LogMessage("Exception: " + std::string(e.what()));
        std::cout << COLOR_RED << "Error: " << e.what() << COLOR_RESET << "\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.ignore();
    std::cin.get();
    LogMessage("HWID Spoofer exited");
    SetConsoleColor(COLOR_RESET);
    return 0;
}