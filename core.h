#pragma once
#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <WinSock2.h>
#include<detours/detours.h>
#include "nlohmann/json.hpp"



#include <vector>

#include "cache.h"

#include "singleton.h"
#include <map>

#include <basetsd.h>
#include "tinyxml2.h"


#include <iostream>
#include <TlHelp32.h>

#include <httplib.h>

#include <cstdint>


#include <fstream>

#include <codecvt>


//微信3.9.10.27
namespace dbj {
	namespace common {

		struct WeChatWString {

			wchar_t* buf = nullptr;
			int32_t     len = 0;
			int32_t     cap = 0;

			int64_t     c_ptr = 0;
			int32_t     c_len = 0;
			int32_t     c_cap = 0;

			WeChatWString();
			~WeChatWString();

			void Init();

			WeChatWString(const std::wstring& s);
			WeChatWString(const wchar_t* str);

			WeChatWString(const WeChatWString& other);

			WeChatWString(WeChatWString&& other) noexcept;

			WeChatWString& operator=(const WeChatWString& other);
			WeChatWString& operator=(WeChatWString&& other) noexcept;



		};

		//union buf[16] 应该就是std::string
		struct WeChatString {

			char* buf = nullptr;
			int64_t		r;
			int64_t		len;
			int64_t     cap;

			WeChatString();
			~WeChatString();

			void Init();

			WeChatString(const std::string& s);
			WeChatString(const char* str);

			WeChatString(const WeChatString& other);

			WeChatString(WeChatString&& other) noexcept;

			WeChatString& operator=(const WeChatString& other);
			WeChatString& operator=(WeChatString&& other) noexcept;

		};

	}
	//偏移
	namespace offset {
		const uint64_t kGetSendMessageMgr = 0x1B4F500;
		const uint64_t kSendTextMsg = 0x22C2070;
		const uint64_t kFreeChatMsg = 0x1B50D80;

		const uint64_t kGetContactMgr = 0x1B3CCD0;
		const uint64_t kGetContactList = 0x219A220;

		const uint64_t kDoAddMsg = 0x230A490;


	
	}

	//hook方法
	namespace function {


		typedef UINT64(*__GetSendMessageMgr)();
		typedef UINT64(*__SendTextMsg)(UINT64, UINT64, UINT64, UINT64, UINT64, UINT64,
			UINT64, UINT64);
		typedef UINT64(*__FreeChatMsg)(UINT64);

		typedef  uint64_t(*__DoAddMsg)(uint64_t, uint64_t, uint64_t);

	
		typedef UINT64(*__Free)();
		typedef UINT64(*__GetContactMgr)();
		typedef UINT64(*__GetContactList)(UINT64, UINT64);

	}

	//用到的属性类
	namespace model {
		struct ContactInner {
			std::string wxid;
			std::string custom_account;
			std::string encrypt_name;
			std::string nickname;
			std::string pinyin;
			std::string pinyin_all;
			std::string remark;
			std::string remark_pinyin;
			std::string remark_pinyin_all;
			std::string label_ids;
			int32_t type;
			int32_t verify_flag;
			int32_t reserved1;
			int32_t reserved2;
			ContactInner() {
				wxid = "";
				custom_account = "";
				encrypt_name = "";
				nickname = "";
				pinyin = "";
				pinyin_all = "";
				remark = "";
				remark_pinyin = "";
				remark_pinyin_all = "";
				label_ids = "";
				type = -1;
				verify_flag = -1;
				reserved1 = -1;
				reserved2 = -1;
			}
		};

		struct WeChatString {
			wchar_t* ptr;
			DWORD length;
			DWORD max_length;
			INT64 c_ptr = 0;
			DWORD c_len = 0;
			WeChatString() { WeChatString(NULL); }

			WeChatString(const std::wstring& s) {
				ptr = (wchar_t*)(s.c_str());
				length = static_cast<DWORD>(s.length());
				max_length = static_cast<DWORD>(s.length());
			}
			WeChatString(const wchar_t* pStr) { WeChatString((wchar_t*)pStr); }
			WeChatString(int tmp) {
				ptr = NULL;
				length = 0x0;
				max_length = 0x0;
			}
			WeChatString(wchar_t* pStr) {
				ptr = pStr;
				length = static_cast<DWORD>(wcslen(pStr));
				max_length = static_cast<DWORD>(wcslen(pStr));
			}
			void set_value(const wchar_t* pStr) {
				ptr = (wchar_t*)pStr;
				length = static_cast<DWORD>(wcslen(pStr));
				max_length = static_cast<DWORD>(wcslen(pStr) * 2);
			}
		};

		struct VectorInner {
#ifdef _DEBUG
			INT64 head;
#endif
			INT64 start;
			INT64 finsh;
			INT64 end;
		};


	}

	
	
	
	namespace baseHook {
		struct InnerMessageStruct {
			char* buffer;
			int64_t length;
			~InnerMessageStruct() {
				if (this->buffer != NULL) {
					delete[] this->buffer;
					this->buffer = NULL;
				}
			}
		};

		class BaseHook {
		public:
			BaseHook() :hook_flag_(false), origin_(nullptr), detour_(nullptr) {}
			BaseHook(void* origin, void* detour);
			int Hook();
			int Unhook();
			virtual ~BaseHook() {}

		protected:
			bool hook_flag_;
			void* origin_;
			void* detour_;
		};

	}

	namespace sync_msg_hook {
		namespace func = function;
		static func::__DoAddMsg kDoAddMsg = nullptr;
		class SyncMsgHook : public baseHook::BaseHook, public  Singleton<SyncMsgHook> {
		public:
			void Init();
		private:
			static void HandleSyncMsg(int64_t param1, int64_t param2, int64_t param3);
		};

	}


	namespace httpServer {
		// 线程函数，用于运行 HTTP 服务器
		void ServerThread();
	}
}

#endif // CACHE_H