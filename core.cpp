#pragma once
#include "core.h"




using namespace std;


namespace dbj {

    namespace baseHook {

        BaseHook::BaseHook(void* origin, void* detour)
            : origin_(origin), detour_(detour) {}

        int BaseHook::Hook() {
            if (hook_flag_) {
                return 2;
            }
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach((PVOID*)origin_, detour_);
            LONG ret = DetourTransactionCommit();
            if (ret == NO_ERROR) {
                hook_flag_ = true;
            }

            return ret;
        }

        int BaseHook::Unhook() {
            if (!hook_flag_) {
                //SPDLOG_INFO("hook already called");
                return NO_ERROR;
            }
            UINT64 base = Cache::getInstance().getBaseAddr();
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach((PVOID*)origin_, detour_);
            LONG ret = DetourTransactionCommit();
            if (ret == NO_ERROR) {
                hook_flag_ = false;
            }
            return ret;
        }

    }  // namespace hook

    namespace util {
        using json = nlohmann::json;
        std::string WstringToUtf8(const std::wstring& wstr) {
            // 使用标准库的wstring_convert进行转换
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.to_bytes(wstr);
        }

        std::wstring ReadWstring(INT64 addr) {
            DWORD len = *(DWORD*)(addr + 0x8);
            if (len == 0) {
                return std::wstring();
            }
            wchar_t* str = *(wchar_t**)(addr);
            if (str == NULL) {
                return std::wstring();
            }
            return std::wstring(str, len);
        }

        std::string ReadWstringThenConvert(INT64 addr) {
            std::wstring wstr = ReadWstring(addr);
            return WstringToUtf8(wstr);
        }

        std::string Bytes2Hex(const BYTE* bytes, const int length) {
            if (bytes == NULL) {
                return "";
            }
            std::string buff;
            const int len = length;
            for (int j = 0; j < len; j++) {
                int high = bytes[j] / 16, low = bytes[j] % 16;
                buff += (high < 10) ? ('0' + high) : ('a' + high - 10);
                buff += (low < 10) ? ('0' + low) : ('a' + low - 10);
            }
            return buff;
        }

        std::string WstringToAnsi(const std::wstring& input, INT64 locale) {
            int char_len = WideCharToMultiByte(locale, 0, input.c_str(), -1, 0, 0, 0, 0);
            if (char_len > 0) {
                std::vector<char> temp(char_len);
                WideCharToMultiByte(locale, 0, input.c_str(), -1, &temp[0], char_len, 0, 0);
                return std::string(&temp[0]);
            }
            return std::string();
        }

        std::string WstringToUTF8Ansi(const std::wstring& str) {
            return WstringToAnsi(str, CP_UTF8);
        }


        std::string ReadWeChatStr(INT64 addr) {
            INT64 len = *(INT64*)(addr + 0x10);
            if (len == 0) {
                return std::string();
            }
            INT64 max_len = *(INT64*)(addr + 0x18);
            if ((max_len | 0xF) == 0xF) {
                return std::string((char*)addr, len);
            }
            char* char_from_user = *(char**)(addr);
            return std::string(char_from_user, len);
        }

        template <typename T>
        static T* WxHeapAlloc(size_t n) {
            return (T*)HeapAlloc(GetProcessHeap(), 0, n);
        }

        std::wstring utf8_to_utf16(const std::string& utf8str) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(utf8str);
        }

        std::wstring AnsiToWstring(const std::string& input, INT64 locale) {
            int wchar_len = MultiByteToWideChar(locale, 0, input.c_str(), -1, NULL, 0);
            if (wchar_len > 0) {
                std::vector<wchar_t> temp(wchar_len);
                MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &temp[0], wchar_len);
                return std::wstring(&temp[0]);
            }
            return std::wstring();
        }

        std::wstring Utf8ToWstring(const std::string& str) {
            return AnsiToWstring(str, CP_UTF8);
        }

        template <typename T1, typename T2>
        std::vector<T1> split(T1 str, T2 letter) {
            std::vector<T1> arr;
            size_t pos;
            while ((pos = str.find_first_of(letter)) != T1::npos) {
                T1 str1 = str.substr(0, pos);
                arr.push_back(str1);
                str = str.substr(pos + 1, str.length() - pos - 1);
            }
            arr.push_back(str);
            return arr;
        }

        std::wstring GetWStringParam(json data, std::string key) {
            return Utf8ToWstring(data.at(key).get<std::string>());
        }

        std::vector<std::wstring> GetArrayParam(json data, std::string key) {
            std::vector<std::wstring> result;
            std::wstring param = GetWStringParam(data, key);
            result = split(param, L',');
            return result;
        }


        bool IsTextUtf8(const char* str, INT64 length) {
            char endian = 1;
            bool littlen_endian = (*(char*)&endian == 1);

            size_t i;
            int bytes_num;
            unsigned char chr;

            i = 0;
            bytes_num = 0;
            while (i < length) {
                if (littlen_endian) {
                    chr = *(str + i);
                }
                else {  // Big Endian
                    chr = (*(str + i) << 8) | *(str + i + 1);
                    i++;
                }

                if (bytes_num == 0) {
                    if ((chr & 0x80) != 0) {
                        while ((chr & 0x80) != 0) {
                            chr <<= 1;
                            bytes_num++;
                        }
                        if ((bytes_num < 2) || (bytes_num > 6)) {
                            return false;
                        }
                        bytes_num--;
                    }
                }
                else {
                    if ((chr & 0xC0) != 0x80) {
                        return false;
                    }
                    bytes_num--;
                }
                i++;
            }
            return (bytes_num == 0);
        }
        model::WeChatString* BuildWechatString(const std::wstring& ws) {
            model::WeChatString* p =
                util::WxHeapAlloc<model::WeChatString>(
                    sizeof(model::WeChatString));
            wchar_t* p_chat_room_id =
                util::WxHeapAlloc<wchar_t>((ws.size() + 1) * 2);
            wmemcpy(p_chat_room_id, ws.c_str(), ws.size() + 1);
            p->ptr = p_chat_room_id;
            p->length = static_cast<DWORD>(ws.size());
            p->max_length = static_cast<DWORD>(ws.size());
            p->c_len = 0;
            p->c_ptr = 0;
            return p;
        }

        std::string ReadSKBuiltinString(INT64 addr) {
            INT64 inner_string = *(INT64*)(addr + 0x8);
            if (inner_string == 0) {
                return std::string();
            }
            return ReadWeChatStr(inner_string);
        }

        std::string ReadSKBuiltinBuffer(INT64 addr) {
            INT64 len = *(INT64*)(addr + 0x10);
            if (len == 0) {
                return std::string();
            }
            INT64 inner_string = *(INT64*)(addr + 0x8);
            if (inner_string == 0) {
                return std::string();
            }
            return ReadWeChatStr(inner_string);
        }



    }

    namespace common {


        WeChatWString::WeChatWString() {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString " << std::hex << ":" << this;

        }

        void WeChatWString::Init() {
            buf = nullptr;
            len = 0;
            cap = 0;
            c_ptr = 0;
            c_len = 0;
            c_cap = 0;
        }

        WeChatWString::~WeChatWString() {

            std::stringstream ss;
            ss << "WeChatWString::~WeChatWString " << std::hex << ":" << this << ":" << buf;

            if (buf != nullptr) {
                delete[] buf;
                buf = nullptr;
                len = 0;
                cap = 0;
            }
            if (c_ptr) {
                delete[](void*)c_ptr;
                c_ptr = 0;
                c_len = 0;
                c_cap = 0;
            }
        }

        WeChatWString::WeChatWString(const std::wstring& s) {

            std::stringstream ss;
            ss << "WeChatWString::WeChatWString const std::wstring " << std::hex << ":" << this;

            Init();
            if (!s.empty()) {
                buf = new wchar_t[s.length() + 1];
                wcscpy_s(buf, (s.length() + 1), s.c_str());
                len = static_cast<int32_t>(s.length());
                cap = static_cast<int32_t>(s.length());
                c_ptr = 0;
                c_len = 0;
                c_cap = 0;
            }
        }

        WeChatWString::WeChatWString(const wchar_t* str) {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString const wchar_t " << std::hex << ":" << this;

            Init();
            if (wcslen(str) > 0) {
                buf = new wchar_t[wcslen(str) + 1];
                wcscpy_s(buf, (wcslen(str) + 1), str);
                len = static_cast<int32_t>(wcslen(str));
                cap = static_cast<int32_t>(wcslen(str));
                c_ptr = 0;
                c_len = 0;
                c_cap = 0;
            }
        }

        WeChatWString::WeChatWString(const WeChatWString& other) {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString WeChatWString " << std::hex << ":" << this;

            Init();
            if (other.len > 0) {
                buf = new wchar_t[size_t(other.len) + 1];
                wcscpy_s(buf, uint64_t(other.len) + 1, other.buf);
                len = other.len;
                cap = other.cap;
                c_ptr = other.c_ptr;
                c_len = other.c_len;
                c_cap = other.c_cap;
            }
        }


        WeChatWString::WeChatWString(WeChatWString&& other) noexcept :
            buf(other.buf), len(other.len), cap(other.cap),
            c_ptr(other.c_ptr), c_len(other.c_ptr), c_cap(other.c_cap) {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString WeChatWString&& " << std::hex << ":" << this << ":" << &other;

            ss.clear();
            ss << "WeChatWString::WeChatWString WeChatWString&& " << std::hex << buf << ":" << len;

            other.buf = nullptr;
            other.len = 0;
            other.cap = 0;
            other.c_ptr = 0;
            other.c_len = 0;
            other.c_cap = 0;
        }

        WeChatWString& WeChatWString::operator=(const WeChatWString& other) {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString operator=& " << std::hex << ":" << this;

            if (this == &other) {
                return *this;
            }
            Init();
            if (other.len > 0) {
                buf = new wchar_t[sizeof(other.len) + 1];
                wcscpy_s(buf, uint64_t(other.len) + 1, other.buf);
                len = other.len;
                cap = other.cap;
                c_ptr = other.c_ptr;
                c_len = other.c_len;
                c_cap = other.c_cap;
            }
            return *this;
        }

        WeChatWString& WeChatWString::operator=(WeChatWString&& other) noexcept {
            std::stringstream ss;
            ss << "WeChatWString::WeChatWString operator=&& " << std::hex << ":" << this;

            if (this == &other) {
                return *this;
            }
            if (buf != nullptr) {
                delete[] buf;
            }
            if (c_ptr) {
                delete[](void*)c_ptr;
            }
            buf = other.buf;
            len = other.len;
            cap = other.cap;
            c_ptr = other.c_ptr;
            c_len = other.c_len;
            c_cap = other.c_cap;

            other.buf = nullptr;
            other.len = 0;
            other.cap = 0;
            other.c_ptr = 0;
            other.c_len = 0;
            other.c_cap = 0;
            return *this;
        }




        WeChatString::WeChatString() :buf(nullptr), r(0), len(0), cap(0) {}

        WeChatString::~WeChatString() {

            std::stringstream ss;
            ss << "WeChatString::~WeChatString :" << std::hex << (void*)buf << ":" << this;

            buf = nullptr;
            len = 0;
            cap = 0;
            r = 0;
        }

        void WeChatString::Init() {
            buf = nullptr;
            len = 0;
            cap = 0;
            r = 0;
        }

        WeChatString::WeChatString(const std::string& s) {

            Init();
            if (!s.empty()) {
                buf = (char*)s.data();
                len = s.length();
                cap = s.length() + 1;
                r = 0;

            }
        }

        WeChatString::WeChatString(const char* str) {
            std::stringstream ss;
            ss << "WeChatString::WeChatString  const char:" << std::hex << (void*)buf << ":" << this;

            Init();
            if (strlen(str) > 0) {
                buf = (char*)str;
                len = strlen(str);
                cap = len + 1;
                r = 0;
            }
        }

        WeChatString::WeChatString(const WeChatString& other) {
            std::stringstream ss;
            ss << "WeChatString::WeChatString  const WeChatString:" << std::hex << (void*)buf << ":" << this;

            Init();
            if (other.len > 0) {
                buf = other.buf;
                len = other.len;
                cap = other.cap;
                r = other.r;
            }
        }


        WeChatString::WeChatString(WeChatString&& other) noexcept :
            buf(other.buf), len(other.len), cap(other.cap),
            r(other.r) {
            std::stringstream ss;
            ss << "WeChatString::WeChatString  WeChatString&&:" << std::hex << (void*)buf << ":" << this;

            other.buf = nullptr;
            other.len = 0;
            other.cap = 0;
            other.r = 0;
        }


        WeChatString& WeChatString::operator=(const WeChatString& other) {
            std::stringstream ss;
            ss << "WeChatString::WeChatString  operator=:" << std::hex << (void*)buf << ":" << this;

            if (this == &other) {
                return *this;
            }
            Init();
            if (other.len > 0) {
                buf = other.buf;
                len = other.len;
                cap = other.cap;
                r = other.r;
            }
            return *this;
        }

        WeChatString& WeChatString::operator=(WeChatString&& other) noexcept {
            std::stringstream ss;
            ss << "WeChatString::WeChatString  operator=&&:" << std::hex << (void*)buf << ":" << this;

            if (this == &other) {
                return *this;
            }
            buf = other.buf;
            len = other.len;
            cap = other.cap;
            r = other.r;
            other.buf = nullptr;
            other.len = 0;
            other.cap = 0;
            other.r = 0;
            return *this;
        }

    }
  
    namespace sync_msg_hook {

        void SyncMsgHook::Init() {
            int64_t addr = Cache::getInstance().getBaseAddr() + offset::kDoAddMsg;
            kDoAddMsg = (func::__DoAddMsg)addr;
            origin_ = &kDoAddMsg;
            detour_ = &HandleSyncMsg;
            hook_flag_ = false;
        }

        void ShowUnicodeString(wchar_t* unicodeStrPtr) {
            // 检查指针是否为空
            if (unicodeStrPtr == NULL) {
                MessageBox(NULL, L"Pointer is NULL", L"Error", MB_OK | MB_ICONERROR);
                return;
            }

            // 显示字符串内容
            MessageBox(NULL, unicodeStrPtr, L"Unicode String", MB_OK | MB_ICONINFORMATION);
        }

        std::wstring stringToWString(const std::string& str) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        void ShowMessageBox(const std::string& jstr) {
            std::wstring wjstr = stringToWString(jstr);
            MessageBox(NULL, wjstr.c_str(), L"Message", MB_OK | MB_ICONINFORMATION);
        }

        void SaveStringToFile(const std::string& str, const std::string& filename) {
            std::ofstream outFile(filename);
            if (outFile.is_open()) {
                outFile << str;
                outFile.close();
                std::cout << "File saved successfully to " << filename << std::endl;
            }
            else {
                std::cerr << "Error: Unable to open file for writing: " << filename << std::endl;
                // 打印出具体的错误原因
                std::perror("Error opening file");
            }
        }

        // 将wchar_t*转换为std::string的辅助函数
        std::string wcharToString(const wchar_t* wstr) {
            // 获取转换后需要的缓冲区大小
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
            // 创建缓冲区
            std::string str(bufferSize, 0);
            // 执行转换
            WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], bufferSize, nullptr, nullptr);
            return str;
        }

        void SyncMsgHook::HandleSyncMsg(int64_t param1, int64_t param2, int64_t param3) {
            nlohmann::json msg;

            msg["pid"] = GetCurrentProcessId();
            msg["fromUser"] = util::ReadSKBuiltinString(*(int64_t*)(param2 + 0x18));
            msg["toUser"] = util::ReadSKBuiltinString(*(int64_t*)(param2 + 0x28));
            msg["content"] = util::ReadSKBuiltinString(*(int64_t*)(param2 + 0x30));
            msg["signature"] = util::ReadWeChatStr(*(int64_t*)(param2 + 0x48));
            msg["msgId"] = *(int64_t*)(param2 + 0x60);
            msg["msgSequence"] = *(DWORD*)(param2 + 0x5C);
            msg["createTime"] = *(DWORD*)(param2 + 0x58);
            msg["displayFullContent"] = util::ReadWeChatStr(*(int64_t*)(param2 + 0x50));
            DWORD type = *(DWORD*)(param2 + 0x24);
            msg["type"] = type;
            if (type == 3) {
                std::string img = util::ReadSKBuiltinBuffer(*(int64_t*)(param2 + 0x40));
                //SPDLOG_INFO("encode size:{}", img.size());
                msg["base64Img"] = base64_encode(img);
            }
            std::string jstr = msg.dump() + '\n';
            /*baseHook::InnerMessageStruct* inner_msg = new baseHook::InnerMessageStruct;
            inner_msg->buffer = new char[jstr.size() + 1];
            memcpy(inner_msg->buffer, jstr.c_str(), jstr.size() + 1);
            inner_msg->length = jstr.size();*/


            Cache::getInstance().enqueueMessage(jstr);

            if (kDoAddMsg == nullptr) {
                int64_t addr = Cache::getInstance().getBaseAddr() + offset::kDoAddMsg;
                kDoAddMsg = (function::__DoAddMsg)addr;
            }
            kDoAddMsg(param1, param2, param3);
        }

      
    }  

    namespace hook {

        namespace func = dbj::function;
        namespace model = dbj::model;

        namespace util = dbj::util;




       

        //获取联系人
        std::string GetContacts() {
            std::vector<dbj::model::ContactInner> vec;
            int64_t success = -1;
            int64_t base_addr = Cache::getInstance().getBaseAddr();
            uint64_t get_contact_mgr_addr = base_addr + dbj::offset::kGetContactMgr;
            uint64_t get_contact_list_addr = base_addr + dbj::offset::kGetContactList;
            func::__GetContactMgr get_contact_mgr = (func::__GetContactMgr)get_contact_mgr_addr;
            func::__GetContactList get_contact_list = (func::__GetContactList)get_contact_list_addr;

            uint64_t mgr = get_contact_mgr();
            uint64_t contact_vec[3] = { 0, 0, 0 };
            success = get_contact_list(mgr, reinterpret_cast<uint64_t>(&contact_vec));

            if (success != 1) {
                nlohmann::json ret_data = { {"code", success}, {"data", {}}, {"msg", "failed to get contacts"} };
                return ret_data.dump();
            }

            uint64_t start = contact_vec[0];
            uint64_t end = contact_vec[2];
            while (start < end) {
                model::ContactInner temp;
                temp.wxid = util::ReadWstringThenConvert(start + 0x10);
                temp.custom_account = util::ReadWstringThenConvert(start + 0x30);
                temp.encrypt_name = util::ReadWstringThenConvert(start + 0x50);
                temp.remark = util::ReadWstringThenConvert(start + 0x80);
                temp.remark_pinyin = util::ReadWstringThenConvert(start + 0x148);
                temp.remark_pinyin_all = util::ReadWstringThenConvert(start + 0x168);
                temp.label_ids = util::ReadWstringThenConvert(start + 0xc0);
                temp.nickname = util::ReadWstringThenConvert(start + 0xA0);
                temp.pinyin = util::ReadWstringThenConvert(start + 0x108);
                temp.pinyin_all = util::ReadWstringThenConvert(start + 0x128);
                temp.verify_flag = *(int32_t*)(start + 0x70);
                temp.type = *(int32_t*)(start + 0x74);
                temp.reserved1 = *(int32_t*)(start + 0x1F0);
                temp.reserved2 = *(int32_t*)(start + 0x1F4);
                vec.push_back(temp);
                start += 0x6A8;
            }

            nlohmann::json ret_data = { {"code", success}, {"data", {}}, {"msg", "success"} };
            for (unsigned int i = 0; i < vec.size(); i++) {
                nlohmann::json item = {
                    {"customAccount", vec[i].custom_account},
                    {"encryptName", vec[i].encrypt_name},
                    {"type", vec[i].type},
                    {"verifyFlag", vec[i].verify_flag},
                    {"wxid", vec[i].wxid},
                    {"nickname", vec[i].nickname},
                    {"pinyin", vec[i].pinyin},
                    {"pinyinAll", vec[i].pinyin_all},
                    {"reserved1", vec[i].reserved1},
                    {"reserved2", vec[i].reserved2},
                    {"remark", vec[i].remark},
                    {"remarkPinyin", vec[i].remark_pinyin},
                    {"remarkPinyinAll", vec[i].remark_pinyin_all},
                    {"labelIds", vec[i].label_ids},
                };
                ret_data["data"].push_back(item);
            }

            return ret_data.dump();
        }

      

        //发送消息
        INT64 SendTextMsg(const std::wstring& wxid, const std::wstring& msg) {
            INT64 success = -1;
            model::WeChatString to_user(wxid);
            model::WeChatString text_msg(msg);

            UINT64 base_addr_ = Cache::getInstance().getBaseAddr();

            UINT64 send_message_mgr_addr = base_addr_ + offset::kGetSendMessageMgr;
            UINT64 send_text_msg_addr = base_addr_ + offset::kSendTextMsg;
            UINT64 free_chat_msg_addr = base_addr_ + offset::kFreeChatMsg;
            char chat_msg[0x460] = { 0 };
            UINT64 temp[3] = { 0 };
            func::__GetSendMessageMgr mgr;
            mgr = (func::__GetSendMessageMgr)send_message_mgr_addr;
            func::__SendTextMsg send;
            send = (func::__SendTextMsg)send_text_msg_addr;
            func::__FreeChatMsg free;
            free = (func::__FreeChatMsg)free_chat_msg_addr;
            mgr();
            send(reinterpret_cast<UINT64>(&chat_msg), reinterpret_cast<UINT64>(&to_user),
                reinterpret_cast<UINT64>(&text_msg), reinterpret_cast<UINT64>(&temp), 1,
                1, 0, 0);
            free(reinterpret_cast<UINT64>(&chat_msg));
            success = 1;
            return success;
        }

      
      
     
  
    

      
    }

    namespace httpServer {

        using namespace httplib;

        using json = nlohmann::json;

        // 将 std::wstring 写入文件
        std::string writeWStringToFile(const std::wstring& wstr) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string utf8Str = converter.to_bytes(wstr);

            return utf8Str;
        }

        std::wstring utf8_to_wstring(const std::string& str) {
            if (str.empty()) return std::wstring();
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        std::string utf8_to_gb2312(const std::string& utf8_str) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wstr = converter.from_bytes(utf8_str);

            int size_needed = WideCharToMultiByte(936, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
            std::string gb2312_str(size_needed, 0);
            WideCharToMultiByte(936, 0, wstr.c_str(), (int)wstr.size(), &gb2312_str[0], size_needed, NULL, NULL);

            return gb2312_str;
        }

        // 将 std::wstring 保存到文件中
        void save_wstring_to_file(const std::wstring& wstr, const std::wstring& filename) {
            std::wofstream wof(filename, std::ios::out | std::ios::binary);
            wof.imbue(std::locale(wof.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
            if (wof.is_open()) {
                wof << wstr;
                wof.close();
            }
            else {
                MessageBoxW(NULL, L"Unable to open file for writing.", L"Error", MB_OK);
            }
        }


        // 判断字符是否为中文字符
        bool isChinese(char c) {
            return (c >= 0x4E00 && c <= 0x9FFF);
        }

        // 将中文字符转换为 Unicode 转义序列
        std::string chineseToUnicode(const std::string& input) {
            std::string output;
            for (char c : input) {
                if (isChinese(c)) {
                    output += "\\u" + std::to_string((unsigned int)c);
                }
                else {
                    output += c;
                }
            }
            return output;
        }


        std::vector<std::wstring> parseWxidsFromRequest(const nlohmann::json& json_data) {
            std::vector<std::wstring> wxids;
            for (const auto& wxid : json_data) {
                wxids.push_back(util::Utf8ToWstring(wxid.get<std::string>()));
            }
            return wxids;
        }

        // 线程函数，用于运行 HTTP 服务器
        void ServerThread() {
            Server svr;

            //获取联系人
            svr.Get("/GetContacts", [](const Request& req, Response& res) {

                std::string str = dbj::hook::GetContacts();
                res.set_content(str, "text/plain; charset=UTF-8");
                });

     
         
            //发送文本
            svr.Post("/SendTextMsg", [](const Request& req, Response& res) {
                try {
                    // 解析 JSON 数据
                    json j = json::parse(req.body);

                    // 提取 JSON 数据中的字段
                    std::string wxid = j.at("wxid").get<std::string>();
                    std::string msg = j.at("msg").get<std::string>();

                    INT64 success = dbj::hook::SendTextMsg(util::utf8_to_utf16(wxid), util::utf8_to_utf16(msg));

                    // 返回处理结果
                    nlohmann::json ret_data = {
                {"code", success}, {"msg", "success"}, {"data", {}} };

                    res.set_content(ret_data.dump(), "text/plain; charset=UTF-8");
                }
                catch (const std::exception& e) {

                    // 返回处理结果
                    nlohmann::json ret_data = { {"code", 2}, {"data", {e.what()}}, {"msg", "error"} };

                    res.set_content(ret_data.dump(), "text/plain; charset=UTF-8");
                }
                });

       
       
            //接收消息
            svr.Get("/Recive_Msg", [](const Request& req, Response& res) {

                std::string str;

                bool flag = Cache::getInstance().popMessage(str);
                if (flag) {
                    res.set_content(str, "text/plain; charset=UTF-8");
                }
                else {
                    // 返回处理结果
                    nlohmann::json ret_data = { {"code", 2}, {"data", {}}, {"msg", "error"} };

                    res.set_content(ret_data.dump(), "text/plain; charset=UTF-8");
                }


                });

     
      
         

           

          



            svr.listen("127.0.0.1", 1234);
        }
    }
}