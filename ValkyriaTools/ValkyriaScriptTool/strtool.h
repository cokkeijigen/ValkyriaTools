#pragma once

namespace strtool {

    void converts(std::string& str, int32_t o_cdpg, int32_t n_cdpg) {
        if (o_cdpg != n_cdpg) {
            int dwNum = MultiByteToWideChar(o_cdpg, 0, str.c_str(), -1, 0, 0);
            wchar_t* dst = new wchar_t[dwNum + 1];
            MultiByteToWideChar(o_cdpg, 0, str.c_str(), -1, dst, dwNum);
            dwNum = WideCharToMultiByte(n_cdpg, 0, dst, -1, 0, 0, 0, 0);
            char* psText = new char[dwNum + 1];
            WideCharToMultiByte(n_cdpg, 0, dst, -1, psText, dwNum, 0, 0);
            str.assign(psText);
            delete[]dst, delete[]psText;
        }
    }

    std::string converts(const char* str, int32_t o_cdpg, int32_t n_cdpg) {
        std::string result(str);
        converts(result, o_cdpg, n_cdpg);
        return result;
    }
}