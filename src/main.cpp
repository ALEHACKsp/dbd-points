#include <windows.h>
#include <psapi.h>

#include <iostream>
#include <thread>

#include "process.h"
#include "memory.h"

#define TARGET_FILENAME "DeadByDaylight-Win64-Shipping.exe"

int main()
{
    using namespace Cheddar;

    std::cout << "Waiting for the game to launch..." << std::endl;

    auto handle = Process::GetHandle(TARGET_FILENAME, "Steam", PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION);
    auto baseAddress = (char*)Process::FindBaseAddress(handle, TARGET_FILENAME);

    std::cout << std::endl;
    std::cout << "Wait for the main menu and press enter." << std::endl;
    std::cout << std::endl;
    std::cin.get();

    std::cout << "Scanning..." << std::endl;

    Process::IterateMemory(handle, [&](void* ptr, const std::vector<char>& data)
    {
        uint64_t offset;
        if (Memory::FindPattern
        (
            data.data(),
            data.size(),
            "\x04\x00\x00\x00\x04\x00\x00\x00\x8c\x02\x00\x00\x00\x00\x00\x00\x07\x00\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxx????????xxxxxxxxxxxxxxxxxxxxxx",
            offset
        ))
        {
            auto address = (char*)ptr + offset;
            std::cout << "Pattern found @ " << (void*)address << ", offset: " << offset << std::endl;

            SIZE_T bytesWritten;
            uint32_t bp = 1000000;
            if (!WriteProcessMemory(handle, address + 32, &bp, sizeof(uint32_t), &bytesWritten))
            {
                std::cout << "Failed to write to process memory! Error: " << GetLastError() << std::endl;
            }
            else
            {
                std::cout << "Value written! Enjoy." << std::endl;
            }

            return true;
        }

        return true;
    });

    return 0;
}
