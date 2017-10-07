#ifndef __MEMORY_H
#define __MEMORY_H

#include <windows.h>

#include <string>

namespace Cheddar
{

    namespace Memory
    {
        bool DataCompare(const char* data, const char* sig, const std::string& mask)
        {
            for (auto i = 0u; i < mask.length(); i++)
            {
                if (mask[i] == '?')
                {
                    continue;
                }

                if (data[i] != sig[i])
                {
                    return false;
                }
            }

            return true;
        }

        bool FindPattern(const char* address, size_t size, const char* sig, const std::string& mask, uint64_t& offset)
        {
            offset = 0;

            if (size < mask.length())
            {
                return false;
            }

            for (size_t i = 0; i < (size - mask.length()); i++)
            {
                if (DataCompare(address + i, sig, mask))
                {
                    offset = i;
                    return true;
                }
            }

            return false;
        }

        template <class InIter1, class InIter2, class OutIter>
        void FindValue(unsigned char *base, InIter1 buf_start, InIter1 buf_end, InIter2 pat_start, InIter2 pat_end, OutIter res)
        {
            for (InIter1 pos = buf_start; buf_end != (pos = std::search(pos, buf_end, pat_start, pat_end)); ++pos)
            {
                *res++ = base + (pos - buf_start);
            }
        }

        template <typename T>
        bool FindValue(void* base, size_t size, T value, uint64_t& offset)
        {
            auto ptr = (T*)base;
            auto count = size / sizeof(T);
            offset = 0u;

            for (auto i = 0u; i < count; i++)
            {
                if (*ptr == value)
                {
                    return true;
                }

                ptr += 1;
                offset += sizeof(T);
            }

            return false;
        }
    }
}

#endif
