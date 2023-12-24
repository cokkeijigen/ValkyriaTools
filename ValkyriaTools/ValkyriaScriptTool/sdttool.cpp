#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include "filetool.h"
#include "sdttool.h"
#include "strtool.h"

namespace sdt {

	Text::Text(int16_t op, text str): opcode(op), size(0), length(0) {
		if (!str.sz || !str.len || !str.str) return;
		this->texts[size++] = str;
		this->length = str.sz;
	}

	void Text::clear() {
		for (uint8_t i = 0; i < this->size; i++) {
			if(this->texts[i].str) delete[] this->texts[i].str;
			this->texts[i] = { nullptr, 0x00, 0x00 };
		}
		this->size = 0x00, this->length = 0x00;
	}

	std::ostream& operator<<(std::ostream& cout, sdt::Text& text) {
		printf("0x%04X[\n", (int)text.opcode);
		for (uint8_t i = 0; i < text.size; i++) {
			if (text.texts[i].str) {
				printf("  0x%02X:0x%02X\"%s\"", text.texts[i].sz,
					text.texts[i].len, text.texts[i].str
				);
			}
			if (i != text.size - 1) cout << ",";
			cout << std::endl;
		}
		cout << "]";
		return cout;
	}

	bool Text::add(text str) {
		if(this->size == 0x05) return false;
		this->texts[this->size++] = str;
		this->length += str.sz;
		return true;
	}

	BaseData::~BaseData() { this->texts.clear(); }

	void BaseData::text_add(size_t& pos, Text& text) {
		if (this->texts.find(pos) != this->texts.end()) {
			this->texts.at(pos).add({ text.texts[0].str, 
				text.texts[0].sz, text.texts[0].len
			});
		}
		else {
			this->texts.insert(std::make_pair(pos, text));
		}
	}

	template <typename funcallback>
	void BaseData::foreach(size_t count, funcallback callback) {
		for (const std::pair<size_t, Text>& pair : this->texts) {
			callback((size_t&)pair.first, (Text&)pair.second, (size_t&)count);
			count++;
		}
	}

	int32_t BaseData::decenc(uint8_t* str, bool dec, int32_t length) {
		if (dec) do { length++; } while (*str++ ^= 0xff);
		else do { *str ^= 0xFF, length++; } while (*str++ != 0xFF);
		return length;
	}

	SDTHelper::SDTHelper(uint8_t* buffer, size_t size) {
		size_t cur_pos = *(uint32_t*)buffer;
		while(this->text_parse(cur_pos, buffer, size));
	}

	bool SDTHelper::text_parse(size_t& pos, uint8_t* buf, const size_t& max) {
		if (pos + 4 >= max) return false;
		switch (*(uint32_t*)(buf + pos)) {
		case 0x090B170A: { // 标题
			uint8_t* text = &buf[pos + 0x09];
			int32_t size = this->decenc(text, true);
			Text aText(0x0A6F, { (char*)text, size, size });
			this->text_add((size_t&)(pos -= 0x01), aText);
			return pos = pos + size + 0x0C;
		}
		case 0x867E0E00: { // 人名
			uint8_t* text = &buf[pos + 0x02];
			int32_t size = this->decenc(text, true);
			Text aText(0x0E00, { (char*)text, size, size });
			this->text_add((size_t&)pos, aText);
			return pos = pos + size + 0x02;
		}
		case 0x11110E01: { // 文本
			uint8_t* text = &buf[pos + 0x08];
			int32_t size = this->decenc(text, true);
			Text aText(0x0E01, { (char*)text, size, size });
			this->text_add((size_t&)pos, aText);
			return pos = pos + size + 0x08;
		}
		case 0x00000E1C: { // 选项
			size_t cur_pos = pos + 0x06;
			Text aText(0x0E1C, { });
		__re:
			uint8_t sig = buf[cur_pos];
			if (sig == 0x08) {
				uint8_t* text = &buf[++cur_pos];
				int32_t size = this->decenc(text, true);
				aText.add({ (char*)text, size, size });
				cur_pos += size;
				goto __re;
			}
			else if (sig == 0xFF) {
				this->text_add((size_t&)pos, aText);
				++cur_pos;
			}
			return pos = cur_pos;
		}
		default: return ++pos;
		}
	}

	TextHelper::TextHelper(std::string path, int32_t CP) {
		files::textrbfr textrbfr(path.c_str(), false);
		if(!textrbfr.count() || textrbfr.empty()) return;
		struct { int pos, op, sz; const char* str; } info {};
		textrbfr.foreach([&](int, const char* str) -> void {
			//std::cout << str << std::endl;
			if (sscanf(str, "#0x%X:0x%X:0x%X", &info.pos, &info.op, &info.sz)) return;
			if (info.pos == 0x00 && info.op == 0x00 && info.sz == 0x00) return;
			if ((info.str = strstr(str, u8"◎★")) && strncmp(info.str + 6, "//", 2)) {
				std::string text = strtool::converts(info.str + 6, CP_UTF8, CP);
				if(text.size() % 2 != 0) text.append(" "); // 字符串长度一定要是偶数
				int32_t size = this->decenc((uint8_t*)text.c_str(), false);
				char* const buf = new char[size];
				memcpy(buf, text.c_str(),  size);
				Text nText((int16_t)info.op, { });
				nText.add({ buf, info.sz, size });
				this->text_add((size_t&)info.pos, nText);
				info = { 0x00, 0x00, 0x00, 0x00 };
			}
		});
	}

	TextHelper::~TextHelper() {
		for (const std::pair<size_t, Text>& pair : this->texts) {
			((Text*)&pair.second)->clear();
		}
	}

	AMCode::AMCode(int8_t* buf, int32_t next): buf(buf), size(0), next(next) {
	}

	AMCode& AMCode::add(void* am, int32_t len) {
		memcpy((void*)(buf + size), am, len);
		this->size = size + len;
		return *this;
	}

	AMCode& AMCode::add(Text::text& text) {
		return this->add(text.str, text.len);
	}

	AMCode& AMCode::add(uint8_t byte) {
		return this->add(&byte, 0x01);
	}

	AMCodeMaker::AMCodeMaker(): buf_ptr(nullptr), buf_max(0) {
	}

	AMCodeMaker::~AMCodeMaker() {
		if(buf_ptr) delete[] buf_ptr;
		buf_ptr = nullptr;
	}

	void AMCodeMaker::buf_check(int32_t size) {
		if (size <= buf_max) return;
		if (buf_ptr) delete[] buf_ptr;
		buf_ptr = new int8_t[size];
		buf_max = size;
	}

	AMCode AMCodeMaker::MakeSelExSet(uint8_t* buf, Text& text) {
		this->buf_check(text.length + text.size + 0x07);
		AMCode amCode(buf_ptr, 0x09);
		amCode.add(buf, 0x06);
		for (uint8_t i = 0; i < text.size; i++) {
			Text::text& str = text.texts[i];
			amCode.add(0x08).add(str);
			amCode.next += str.sz;
		}
		return amCode.add(0xFF);
	}

	AMCode AMCodeMaker::MakeSaveTitle(uint8_t* buf, Text::text& text) {
		this->buf_check(text.len + 0x0C);
		AMCode amCode(buf_ptr, 0x0C + text.sz);
		amCode.add(buf, 0x0A).add(text.str, text.len);
		return amCode.add(buf + 0x0A + text.sz, 0x02);
	}

	AMCode AMCodeMaker::MakeMsgName(uint8_t* buf, Text::text& text) {
		this->buf_check(text.len + 0x02);
		AMCode amCode(buf_ptr, text.sz + 0x02);
		return amCode.add(buf, 0x02).add(text);
	}

	AMCode AMCodeMaker::MakeMsgText(uint8_t* buf, Text::text& text) {
		this->buf_check(text.len + 0x10);
		AMCode amCode(buf_ptr, text.sz);
		amCode.add(buf, 0x08).add(text);
		uint8_t* end = buf + 8 + text.sz;
		if (*(int16_t*)(end) == 0x0E04) {
			amCode.next += 0x10;
			amCode.add(end, 0x08);
		}
		else { 
			amCode.next += 0x08; 
		}
		return amCode;
	}

	AMCode AMCodeMaker::MakeGoto(int32_t hdr, int32_t cur, int32_t tar) {
			uint8_t am[10] = { 0x42, 0x0A };
		if (hdr && cur && tar) {
			this->buf_check(10);
			*(int32_t*)(am + 2) = cur - 0x0E;
			*(int32_t*)(am + 6) = tar - hdr;
			return AMCode(buf_ptr, 10).add(am, 10);
		}
		else if (tar){
			this->buf_check(6);
			uint8_t am[6] = { 0x42, 0x0A };
			*(int32_t*)(am + 2) = tar - 0x14;
			return AMCode(buf_ptr, 6).add(am, 6);
		}
		return AMCode(nullptr, 0x00);
	}
}

namespace sdt::tool {

	std::string CurrentSdtPath;
	int32_t DefaultCodePage = 936;
	static AMCodeMaker CodeMaker;

	bool read_config(std::string in) {
		files::textrbfr trbfr(in.c_str(), false);
		if(trbfr.empty() || !trbfr.count()) return false;
		int32_t read_flag = 0x00, res = 0x00;
		trbfr.foreach([&](int, char* str) -> void {
			if (!strcmp(str, "#UseCodePage:")) {
				read_flag = 0x01;
			} 
			else if (!strcmp(str, "#InputPath:")) {
				read_flag = 0x02;
			}
			else if (read_flag == 0x01 && strlen(str)) {
				res = sscanf(str, "%d", &DefaultCodePage);
			}
			else if (read_flag == 0x02 && strlen(str)) {
				CurrentSdtPath.assign(str);
			}
		});
		return true;
	}

	bool create_config(std::string in, std::string out) {
		files::writebuffer writebuffer;
		writebuffer.format_write(
			"#InputPath:\n%s\n\n", 0x11 + in.size(),
			in.c_str()
		);
		writebuffer.format_write(
			"#UseCodePage:\n%d\n", 0x20, DefaultCodePage
		);
		return writebuffer.save(out.c_str());
	}

	bool text_exporter(std::string in, std::string out) {
		files::data data = files::tool::read(in.c_str());
		if (!data.buffer || !data.size) return false;
		SDTHelper sdt(data.buffer, data.size);
		if(sdt.texts.size() <= 0x00) return false;
		files::writebuffer writebuffer(1024, 1024);
		sdt.foreach(0x01, [&](size_t& pos, Text& text, size_t& count) -> void {
			//std::cout << text << std::endl;
			for (uint8_t i = 0; i < text.size; i++) {
				std::string nText(text.texts[i].str);
				strtool::converts(nText, 932, CP_UTF8);
				writebuffer.format_write(
					"#0x%X:0x%04X:0x%02X\n", 0x40, (int)pos,
					(int)text.opcode, (int)text.texts[i].sz
				);
				writebuffer.format_write(
					u8"★◎  %03d  ◎★//%s\n", 0x20 + nText.size(),
					(int)(count += i), (const char*)nText.c_str()
				);
				writebuffer.format_write(
					u8"★◎  %03d  ◎★%s\n\n", 0x20 + nText.size(),
					(int)count, (const char*)nText.c_str()
				);
			}
		});
		return writebuffer.save(out.c_str());
	}

	static inline void make_check_data(files::data& oldSdt, files::writebuffer& wtBuf) {
		size_t hdr_off = *(uint32_t*)(oldSdt.buffer + 0x00);
		size_t cdt_off = *(uint32_t*)(oldSdt.buffer + 0x10);
		uint8_t* check = oldSdt.buffer + cdt_off;
		size_t length = hdr_off - cdt_off - 1;
		uint8_t oldSz = (uint8_t)oldSdt.size;
		uint8_t newSz = (uint8_t)wtBuf.size();
		do { 
			*check = (*check - oldSz) + newSz;
		} while(*(++check));
		wtBuf.overwrite(cdt_off, check - length, length);
	}

	bool text_importer(std::string txt, std::string sdt, std::string out) {
		sdt::TextHelper texts(txt.c_str(), DefaultCodePage);
		if (!texts.texts.size()) return false;
		files::data data = files::tool::read(sdt.c_str());
		if (!data.buffer || !data.size) return false;
		files::writebuffer writebuffer(data.size + 1024);
		writebuffer.write(data.buffer, data.size);
		size_t hdr_off = *(int32_t*)data.buffer;
		texts.foreach(NULL, [&](size_t& pos, sdt::Text& text, size_t) -> void {
			uint8_t* bufptr = data.buffer + pos;
			size_t cur_pos = writebuffer.size();
			switch (text.opcode) {
			case 0x0A6F: { // 标题
				Text::text& str = text.texts[0];
				AMCode am = CodeMaker.MakeSaveTitle(bufptr, str);
				if (str.len <= str.sz) {
					writebuffer.overwrite(pos, (void*)am.buf, am.size);
					if (str.len == str.sz) return;
					writebuffer.overwrite(
						pos + am.size, (uint8_t)0x00, str.sz - str.len
					);
				}
				else {
					size_t end = writebuffer.write(am.buf, am.size).size();
					AMCode gtc = CodeMaker.MakeGoto(hdr_off, pos, cur_pos);
					writebuffer.overwrite(pos, (void*)gtc.buf, gtc.size);
					size_t begin = pos + gtc.size, next = pos + am.next;
					writebuffer.overwrite(begin, next, (uint8_t)0x00);
					gtc = CodeMaker.MakeGoto(hdr_off, end, next);
					writebuffer.write(gtc.buf, gtc.size);
				}
				break;
			}
			case 0x0E00: { // 人名
				Text::text& str = text.texts[0];
				AMCode am = CodeMaker.MakeMsgName(bufptr, str);
				if (str.len <= str.sz) {
					writebuffer.overwrite(pos, (void*)am.buf, am.size);
					if (str.len == str.sz) return;
					writebuffer.overwrite(
						pos + am.size, (uint8_t)0x00, str.sz - str.len
					);
				} else {
					size_t end = writebuffer.write(am.buf, am.size).size();
					// 可能不够写入完整的goto指令，把相对跳转偏移写到尾部
					writebuffer.write(&(cur_pos -= hdr_off), 0x04);
					// goto指令只需要6个字节，op + 文件相对偏移地址
					AMCode gtc = CodeMaker.MakeGoto(0x00, 0x00, end);
					writebuffer.overwrite(pos, (void*)gtc.buf, gtc.size);
					size_t begin = pos + gtc.size, next = pos + am.next;
					gtc = CodeMaker.MakeGoto(hdr_off, end + 0x04, next);
					writebuffer.write(gtc.buf, gtc.size);
					if (next == pos + 0x06 || begin > next) return;
					writebuffer.overwrite(begin, next, (uint8_t)0x00);
				}
				break;
			}
			case 0x0E01: { // 文本
				Text::text& str = text.texts[0];
				AMCode am = CodeMaker.MakeMsgText(bufptr, str);
				if (str.len <= str.sz) { // 如果小于等于原来的长度，直接覆盖
					writebuffer.overwrite(pos, (void*)am.buf, am.size);
					if (str.len == str.sz) return;
					writebuffer.overwrite( // 小于原来长度需要填充占位符
						pos + am.size, (uint8_t)0x00, str.sz - str.len
					);
				} else { // 长度大于原来的则跳转到尾部写入数据
					size_t end = writebuffer.write(am.buf, am.size).size();
					AMCode gtc = CodeMaker.MakeGoto(hdr_off, pos, cur_pos);
					writebuffer.overwrite(pos, (void*)gtc.buf, gtc.size);
					// 填充占位符（实际上没必要，个人强迫症而已）
					size_t begin = pos + gtc.size, next = pos + am.next;
					writebuffer.overwrite(begin, next, (uint8_t)0x00);
					// 在尾部写入跳转回下一个op的位置
					gtc = CodeMaker.MakeGoto(hdr_off, end, next);
					writebuffer.write(gtc.buf, gtc.size); 
				}
				break;
			}
			case 0x0E1C: { // 选项
				AMCode am = CodeMaker.MakeSelExSet(bufptr, text);
				int32_t old_len = am.next - 0x09, new_len = text.length;
				if (new_len <= old_len) {
					writebuffer.overwrite(pos, (void*)am.buf, am.size);
					if (new_len == old_len) return;
					writebuffer.overwrite(
						pos + am.size, (uint8_t)0x00, old_len - new_len
					);
				} else {
					size_t end = writebuffer.write(am.buf, am.size).size();
					AMCode gtc = CodeMaker.MakeGoto(hdr_off, pos, cur_pos);
					writebuffer.overwrite(pos, (void*)gtc.buf, gtc.size);
					size_t begin = pos + gtc.size, next = pos + am.next;
					writebuffer.overwrite(begin, next, (uint8_t)0x00);
					gtc = CodeMaker.MakeGoto(hdr_off, end, next);
					writebuffer.write(gtc.buf, gtc.size);
				}
				break;
			}
			default: break;
			}
		});
		sdt::tool::make_check_data(data, writebuffer);
		return writebuffer.save(out.c_str());
	}
}