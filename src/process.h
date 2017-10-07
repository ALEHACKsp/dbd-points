#ifndef __GET_HANDLE_H
#define __GET_HANDLE_H

#include <string>
#include <filesystem>
#include <functional>

#include <psapi.h>
#include <windows.h>

namespace Cheddar
{

    namespace Process
    {
        namespace Internal
        {
            std::string g_TargetFilename;
            HANDLE g_TargetHandle = nullptr;
            DWORD g_TargetPerms = PROCESS_ALL_ACCESS;
        }

        void HandleReceiver(HANDLE *io_port)
        {
            DWORD nOfBytes;
            ULONG_PTR cKey;
            LPOVERLAPPED pid;
            char buffer[MAX_PATH];

            while (GetQueuedCompletionStatus(*io_port, &nOfBytes, &cKey, &pid, -1))
            {
                if (nOfBytes != 6)
                {
                    continue;
                }

                auto handle = OpenProcess(Internal::g_TargetPerms, false, (DWORD)pid);
                GetModuleFileNameExA(handle, 0, buffer, MAX_PATH);

                auto path = std::experimental::filesystem::path(buffer);
                std::cout << "[INFO] Found " << path << std::endl;

                if (path.filename() == Internal::g_TargetFilename)
                {
                    Internal::g_TargetHandle = handle;
                    break;
                }

                CloseHandle(handle);
            }
        }

        HANDLE GetHandle(const std::string& processName, const std::string& parentProcessName = "", DWORD perms = PROCESS_ALL_ACCESS)
        {
            Internal::g_TargetFilename = processName;
            Internal::g_TargetPerms = perms;

            auto pid = 0UL;
            HWND desk_hwnd = nullptr;

            if (parentProcessName.length() == 0)
            {
                desk_hwnd = GetShellWindow();
            }
            else
            {
                desk_hwnd = FindWindow(0, "Steam");
            }

            auto ret = GetWindowThreadProcessId(desk_hwnd, &pid);
            auto exp_handle = OpenProcess(PROCESS_ALL_ACCESS, true, pid);
            auto io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
            auto job_object = CreateJobObjectW(0, 0);
            auto job_io_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT{ 0, io_port };
            auto result = SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation, &job_io_port, sizeof(job_io_port));
            result = AssignProcessToJobObject(job_object, exp_handle);

            auto threadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HandleReceiver, &io_port, 0, 0);
            WaitForSingleObject(threadHandle, -1);
            CloseHandle(exp_handle);

            return Internal::g_TargetHandle;
        }

        void* FindBaseAddress(HANDLE process, const std::string& baseImageName)
        {
            HMODULE modules[4096];

            DWORD bytesTotal;
            if (!EnumProcessModules(process, modules, sizeof(modules), &bytesTotal))
            {
                return nullptr;
            }

            auto count = bytesTotal / sizeof(HMODULE);

            for (auto i = 0u; i < count; i++)
            {
                char fName[1024];
                if (!GetModuleFileNameEx(process, modules[i], fName, 1024))
                {
                    continue;
                }

                auto path = std::experimental::filesystem::path(fName);

                if (baseImageName == path.filename())
                {
                    return modules[i];
                }
            }

            return nullptr;
        }

        void IterateMemory(HANDLE process, std::function<bool(void*, const std::vector<char>& data)> callback)
        {
            unsigned char *p = nullptr;
            MEMORY_BASIC_INFORMATION info;

            for (p = nullptr; VirtualQueryEx(process, p, &info, sizeof(info)) == sizeof(info); p += info.RegionSize)
            {
                std::vector<char> buffer;
                std::vector<char>::iterator pos;

                if (info.State == MEM_COMMIT && (info.Type == MEM_MAPPED || info.Type == MEM_PRIVATE))
                {
                    SIZE_T bytesRead;
                    buffer.resize(info.RegionSize);
                    ReadProcessMemory(process, p, &buffer[0], info.RegionSize, &bytesRead);
                    buffer.resize(bytesRead);

                    if (bytesRead == 0)
                    {
                        continue;
                    }

                    if (!callback(p, buffer))
                    {
                        return;
                    }
                }
            }
        }
    }

}

#endif
