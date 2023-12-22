#pragma once
#include <map>

namespace sdt {
	
	class Text {
		friend std::ostream& operator<<(std::ostream& cout, sdt::Text& text);
	public:
		typedef struct { char* str; int32_t sz, len; } text;
		uint8_t size;
		int16_t opcode;
		uint32_t length;
		text texts[5] = {};
		Text(int16_t op, text str);
		bool add(text str);
		void clear();
	};

	class BaseData {
	protected:
		void text_add(size_t& pos, Text& text);
		int32_t decenc(uint8_t* str, bool dec, int32_t length = 0);
	public:
		std::map<size_t, Text> texts;
		template <typename funcallback>
		void foreach(size_t count, funcallback callback);
		~BaseData();
	};

	class SDTHelper: public BaseData {
	private:
		bool text_parse(size_t&, uint8_t*, const size_t&);
	public:
		SDTHelper(uint8_t* buffer, size_t size);
	};

	class TextHelper: public BaseData {
	public:
		TextHelper(std::string path, int32_t CON_CP = 936);
		~TextHelper();
	};

	class AMCode {
	public:
		int32_t size,next;
		const int8_t* buf;
		AMCode& add(uint8_t byte);
		AMCode& add(Text::text& text);
		AMCode& add(void* am, int32_t len);
		AMCode(int8_t* buf, int32_t next);
	};

	class AMCodeMaker {
		int8_t* buf_ptr;
		int32_t buf_max;
		void buf_check(int32_t size);
	public:
		AMCodeMaker();
		AMCode MakeSelExSet(uint8_t* buf, Text& text);
		AMCode MakeSaveTitle(uint8_t* buf, Text::text& text);
		AMCode MakeMsgText(uint8_t* buf, Text::text& text);
		AMCode MakeMsgName(uint8_t* buf, Text::text& text);
		AMCode MakeGoto(int32_t hdr, int32_t cur, int32_t tar);
		~AMCodeMaker();
	};
}

namespace sdt::tool {
	extern int32_t DefaultCodePage;
	extern std::string CurrentSdtPath;
	extern bool read_config(std::string path);
	extern bool text_exporter(std::string in, std::string out);
	extern bool create_config(std::string in, std::string out);
	extern bool text_importer(std::string txt, std::string sdt, std::string out);
}