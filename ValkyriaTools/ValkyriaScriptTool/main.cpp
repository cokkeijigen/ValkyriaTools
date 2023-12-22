#define _CRT_SECURE_NO_WARNINGS
#define _VERSION_NAME "1.00"
#include <iostream>
#include <windows.h>
#include <filesystem>
#include "sdttool.h"

namespace fsys {

	using namespace std::filesystem;
	using dir_ety = directory_entry;
	using dir_ite = directory_iterator;

	static std::string change(const path& p, std::string ext) noexcept {
		return p.stem().string().append(ext);
	}

	static bool make(const path& p) noexcept {
		return exists(p) || create_directory(p);
	}

	static std::string pcat(path p1, path p2) noexcept {
		return (p1 /= p2).string();
	}

	static void pcat(path p1, path p2, std::string& out) noexcept {
		out.assign(fsys::pcat(p1, p2));
	}

	static std::string extension(const path& p) noexcept {
		std::string ext(p.extension().string());
		transform(ext.begin(), ext.end(), ext.begin(), tolower);
		return ext;
	}

	static std::string parent(const path& p) noexcept {
		return p.parent_path().string();
	}

	static bool del(const path& p) noexcept {
		return !exists(p) || remove_all(p);
	}

	static std::string name(const path& p) noexcept {
		return p.filename().string();
	}
}

namespace worker {

	static fsys::path OUT_PATH, IN_PATH;
	static std::string config(".ValkyriaScriptTool");

	inline static void init(fsys::path& out, fsys::path& in) noexcept {
		OUT_PATH.assign(out.parent_path()), IN_PATH.assign(in);
	}
	
	static bool read_config_if_exists() {
		if (sdt::tool::read_config(fsys::pcat(IN_PATH, config))) {
			std::string& path = sdt::tool::CurrentSdtPath;
			if (!fsys::exists(path) || !fsys::is_directory(path))
				throw std::exception(
					"The input sdt path does not exist."
				);
			return true;
		};
		return false;
	}

	static void exports_as_afile(fsys::path path) {
		if (fsys::extension(path) != ".sdt") return;
		if (!fsys::make(OUT_PATH)) {
			throw std::exception(
				"Failed to make output directory."
			);
		}
		std::string name(fsys::change(path, ".txt"));
		std::string out(fsys::pcat(OUT_PATH, name));
		if (sdt::tool::text_exporter(path.string(), out)) {
			std::cout << name << ": saved." << std::endl;
		}
		else {
			std::cout << fsys::name(path);
			std::cout << ": ignored." << std::endl;
		}
	}

	static void exports_as_multifile()  {
		for (const fsys::dir_ety & file : fsys::dir_ite(IN_PATH)) {
			exports_as_afile(file.path());
		}
	}

	static void run_text_imports() {
		if (!fsys::make(OUT_PATH)) {
			throw std::exception("Failed to make output directory.");
		}
		for (const fsys::dir_ety& file : fsys::dir_ite(IN_PATH)) {
			if(fsys::extension(file) != ".txt") continue;
			std::string& path = sdt::tool::CurrentSdtPath;
			std::string name(fsys::change(file, ".sdt"));
			std::string out(fsys::pcat(OUT_PATH, name));
			std::string txt(file.path().string());
			std::string sdt(fsys::pcat(path, name));
			if (sdt::tool::text_importer(txt, sdt, out)) {
				std::cout << name << ": saved." << std::endl;
			}
			else {
				std::cout << fsys::name(file);
				std::cout << ": ignored." << std::endl;
			}
		}
	}

	static void start(fsys::path path1, fsys::path path2) noexcept {
		try {
			worker::init(path1, path2);
			if (worker::read_config_if_exists()) {
				fsys::del(OUT_PATH.append("seen_sdt"));
				worker::run_text_imports();
			}
			else if (fsys::is_directory(IN_PATH)) {
				fsys::del(OUT_PATH.append("seen_txt"));
				worker::exports_as_multifile();
				std::string out(fsys::pcat(OUT_PATH, config));
				std::string in(IN_PATH.string());
				sdt::tool::create_config(in, out);
			}
			else if (fsys::exists(IN_PATH)) {
				fsys::del(OUT_PATH.append("seen_txt"));
				worker::exports_as_afile(IN_PATH);
				std::string out(fsys::pcat(OUT_PATH, config));
				std::string in(fsys::parent(IN_PATH));
				sdt::tool::create_config(in, out);
			}
		} catch (std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}

};

int main(int argc, char* argv[]) {
	if (argc == 2) {
		worker::start(argv[0], argv[1]);
		system("pause");
	}
	return 0;
}