/*
 * Copyright (c) Souldbminer, Lightos_ and Horizon OC Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <map>
#include <cstdint>
#include <string>

std::map<uint32_t, std::string> cpu_freq_label_m = {
    {612000000, "Сон"},
    {1020000000, "Сток"},
    {1224000000, "Dev OC"},
    {1785000000, "Буст"},
    {1963000000, "Безопасн. макс."},
    {2397000000, "Небезопасн. макс."},
    {2703000000, "Абс. макс."},
};

std::map<uint32_t, std::string> cpu_freq_label_m_uv = {
    {612000000, "Сон"},
    {1020000000, "Сток"},
    {1224000000, "Dev OC"},
    {1785000000, "Буст"},
    {2397000000, "Безопасн. макс."},
    {2499000000, "Небезопасн. макс."},
    {2703000000, "Абс. макс."},
};

std::map<uint32_t, std::string> cpu_freq_label_e = {
    {612000000, "Сон"},
    {1020000000, "Сток"},
    {1224000000, "Dev OC"},
    {1785000000, "Безопасн. макс."},
    {2091000000, "Небезопасн. макс."},
    {2397000000, "Абс. макс."},
};

std::map<uint32_t, std::string> cpu_freq_label_e_uv = {
    {612000000, "Сон"},
    {1020000000, "Сток"},
    {1224000000, "Dev OC"},
    {1785000000, "Буст"},
    {2091000000, "Безопасн. макс."},
    {2193000000, "Небезопасн. макс."},
    {2397000000, "Абс. макс."},
};


std::map<uint32_t, std::string> gpu_freq_label_e = {
    {76800000, "Буст"},
    {307200000, "Портатив"},
    {345600000, "Портатив"},
    {384000000, "Портатив"},
    {422400000, "Портатив"},
    {460800000, "Портатив макс."},
    {768000000, "Док"},
    {921600000, "Безопасн. макс."},
    {960000000, "Небезопасн. макс."},
    {1075200000, "Абс. макс."},
};

std::map<uint32_t, std::string> gpu_freq_label_e_uv = {
    {76800000, "Буст"},
    {307200000, "Портатив"},
    {345600000, "Портатив"},
    {384000000, "Портатив"},
    {422400000, "Портатив"},
    {460800000, "Портатив макс."},
    {768000000, "Док"},
    {960000000, "Безопасн. макс."},
    {1075200000, "Абс. макс."},
};

std::map<uint32_t, std::string> gpu_freq_label_m = {
    {76800000, "Буст"},
    {307200000, "Портатив"},
    {384000000, "Портатив"},
    {460800000, "Портатив"},
    {614400000, "Портатив макс."},
    {768000000, "Док"},
    {1075200000, "Безопасн. макс."},
    {1305600000, "Небезопасн. макс."},
    {1536000000, "Абс. макс."},
};

std::map<uint32_t, std::string> gpu_freq_label_m_slt = {
    {76800000, "Буст"},
    {307200000, "Портатив"},
    {384000000, "Портатив"},
    {460800000, "Портатив"},
    {614400000, "Портатив макс."},
    {768000000, "Док"},
    {1152200000, "Безопасн. макс."},
    {1305600000, "Небезопасн. макс."},
    {1536000000, "Абс. макс."},
};

std::map<uint32_t, std::string> gpu_freq_label_m_hiopt = {
    {76800000, "Буст"},
    {307200000, "Портатив"},
    {384000000, "Портатив"},
    {460800000, "Портатив"},
    {614400000, "Портатив макс."},
    {768000000, "Док"},
    {1228800000, "Безопасн. макс."},
    {1305600000, "Небезопасн. макс."},
    {1536000000, "Абс. макс."},
};

std::map<uint32_t, std::string>* marikoUV[3] {
    &gpu_freq_label_m,
    &gpu_freq_label_m_slt,
    &gpu_freq_label_m_hiopt,
};


std::map<uint32_t, std::string>* eristaUV[3] {
    &gpu_freq_label_e,
    &gpu_freq_label_e_uv,
    &gpu_freq_label_e_uv,
};