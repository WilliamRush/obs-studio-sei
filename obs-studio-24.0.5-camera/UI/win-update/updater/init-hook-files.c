#include <windows.h>
#include <strsafe.h>
#include <shlobj.h>
#include <stdbool.h>

static inline bool file_exists(const wchar_t *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE h = FindFirstFileW(path, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	FindClose(h);
	return true;
}

static LSTATUS get_reg(HKEY hkey, LPCWSTR sub_key, LPCWSTR value_name, bool b64)
{
	HKEY key;
	LSTATUS status;
	DWORD flags = b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
	DWORD size = sizeof(DWORD);
	DWORD val;

	status = RegOpenKeyEx(hkey, sub_key, 0, KEY_READ | flags, &key);
	if (status == ERROR_SUCCESS) {
		status = RegQueryValueExW(key, value_name, NULL, NULL,
					  (LPBYTE)&val, &size);
		RegCloseKey(key);
	}
	return status;
}

#define get_programdata_path(path, subpath)                        \
	do {                                                       \
		SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, \
				 SHGFP_TYPE_CURRENT, path);        \
		StringCbCatW(path, sizeof(path), L"\\");           \
		StringCbCatW(path, sizeof(path), subpath);         \
	} while (false)

#define make_filename(str, name, ext)                                \
	do {                                                         \
		StringCbCatW(str, sizeof(str), name);                \
		StringCbCatW(str, sizeof(str), b64 ? L"64" : L"32"); \
		StringCbCatW(str, sizeof(str), ext);                 \
	} while (false)

#define IMPLICIT_LAYERS L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers"
#define HOOK_LOCATION L"\\data\\obs-plugins\\win-capture\\"

static bool update_hook_file(bool b64)
{
	wchar_t temp[MAX_PATH];
	wchar_t src[MAX_PATH];
	wchar_t dst[MAX_PATH];
	wchar_t src_json[MAX_PATH];
	wchar_t dst_json[MAX_PATH];

	GetCurrentDirectoryW(_countof(src_json), src_json);
	StringCbCat(src_json, sizeof(src_json), HOOK_LOCATION);
	make_filename(src_json, L"obs-vulkan", L".json");

	GetCurrentDirectoryW(_countof(src), src);
	StringCbCat(src, sizeof(src), HOOK_LOCATION);
	make_filename(src, L"graphics-hook", L".dll");

	get_programdata_path(temp, L"obs-studio-hook\\");
	StringCbCopyW(dst_json, sizeof(dst_json), temp);
	StringCbCopyW(dst, sizeof(dst), temp);
	make_filename(dst_json, L"obs-vulkan", L".json");
	make_filename(dst, L"graphics-hook", L".dll");

	if (!file_exists(src)) {
		return false;
	}

	CreateDirectoryW(temp, NULL);
	CopyFileW(src, dst, false);
	CopyFileW(src_json, dst_json, false);
	return true;
}

static void update_vulkan_registry(bool b64)
{
	DWORD flags = b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
	wchar_t path[MAX_PATH];
	DWORD temp;
	LSTATUS s;
	HKEY key;

	get_programdata_path(path, L"obs-studio-hook\\");
	make_filename(path, L"obs-vulkan", L".json");

	s = get_reg(HKEY_CURRENT_USER, IMPLICIT_LAYERS, path, b64);
	if (s == ERROR_SUCCESS) {
		s = RegOpenKeyEx(HKEY_CURRENT_USER, IMPLICIT_LAYERS, 0,
				 KEY_WRITE | flags, &key);
		if (s == ERROR_SUCCESS) {
			RegDeleteValueW(key, path);
			RegCloseKey(key);
		}
	}

	s = get_reg(HKEY_LOCAL_MACHINE, IMPLICIT_LAYERS, path, b64);
	if (s == ERROR_SUCCESS) {
		return;
	}

	/* ------------------- */

	s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, IMPLICIT_LAYERS, 0, NULL, 0,
			    KEY_WRITE | flags, NULL, &key, &temp);
	if (s != ERROR_SUCCESS) {
		goto finish;
	}

	DWORD zero = 0;
	s = RegSetValueExW(key, path, 0, REG_DWORD, (const BYTE *)&zero,
			   sizeof(zero));
	if (s != ERROR_SUCCESS) {
		goto finish;
	}

finish:
	if (key)
		RegCloseKey(key);
}

void UpdateHookFiles(void)
{
	if (update_hook_file(true))
		update_vulkan_registry(true);
	if (update_hook_file(false))
		update_vulkan_registry(false);
}
