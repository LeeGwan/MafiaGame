> English | [한국어](./README.kor.md)
#Flect Security Module: Kernel-Mode Security Learning Project
Flect Security Module is a personal learning project where I explored how common user-mode security concepts can be translated into kernel-mode implementations. It consists of a simple kernel driver, an Unreal Engine client interface, and a basic server communication module.

🛠 Architecture Overview
The goal of this project was to understand the fundamentals of kernel-level development and protection.

Kernel Driver: Implements basic Ring 0 protection and hardware information collection.

Client Core: Manages communication between the game engine and the kernel driver.

Server: A simple validation interface for receiving integrity logs and hardware identifiers.

🛡 Key Concepts Implemented
1. Process Protection (Handle Stripping)
ObRegisterCallbacks: Implemented basic callbacks to monitor handle creation and duplication requests.

Access Control: Stripped sensitive permissions (such as PROCESS_VM_WRITE) from handle requests at the kernel level to demonstrate the concept of process protection.

2. Code Integrity Verification
CRC32 Memory Scanning: Segmented memory into 256-byte chunks to generate CRC32 hashes and periodically verify them.

Continuous Monitoring: Created a background system thread to perform periodic memory integrity checks, serving as a practice implementation for detection logic.

3. Hardware Fingerprinting (HWID)
Direct Firmware Access: Learned to map physical memory (MmMapIoSpace) to parse SMBIOS tables, providing a deeper understanding of hardware-level data retrieval.

💻 Project Components
Kernel-Mode Driver
ProcessGuard: Basic handle access control implementation.

Integrity Engine: CRC32-based memory monitoring system.

Telemetry Manager: SMBIOS parsing logic.

Client Integration (Unreal Engine)
Security Dispatcher: Manages communication between user-mode and kernel-mode using DeviceIoControl.

Server
A basic interface designed to receive and validate integrity reports and hardware IDs.

🚀 Development Philosophy & Achievements
Hands-on Learning: Developed the entire project independently to understand Windows kernel internals, driver development, and defensive programming.

Experimentation: Successfully translated standard user-mode protection concepts into kernel-mode routines.

System Stability: Focused on essential stability practices, including kernel synchronization (FastMutex, SpinLock) and exception handling.

"This project provided me with a strong foundation in Windows kernel development and a practical understanding of how security logic operates at the system level."
