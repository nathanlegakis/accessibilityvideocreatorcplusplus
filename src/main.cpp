#include <sapi.h>
#include <sphelper.h>
#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "sqlite3.h"

namespace fs = std::filesystem;

namespace {

struct Options {
    std::wstring text;
    std::wstring outPath;
    int rate = 0;
    unsigned short volume = 100;

    std::string dbPath;
    std::string importFolder;
    bool list = false;
    bool speakNext = false;
};

void PrintUsage() {
    std::wcout
        << L"Usage:\n"
        << L"  accessibility_video_creator --text \"Hello\" [--out out.wav] [--rate -10..10] [--volume 0..100]\n"
        << L"  accessibility_video_creator --db queue.db --import <folder>\n"
        << L"  accessibility_video_creator --db queue.db --list\n"
        << L"  accessibility_video_creator --db queue.db --speak-next [--out out.wav]\n";
}

std::wstring ToWide(const char* s) {
    if (!s) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring out(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
    return out;
}

std::string ToUtf8(const std::wstring& ws) {
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len, nullptr, nullptr);
    return out;
}

bool Speak(const std::wstring& text, const std::wstring& outPath, int rate, unsigned short volume) {
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return false;

    ISpVoice* voice = nullptr;
    hr = ::CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&voice));
    if (FAILED(hr) || !voice) {
        ::CoUninitialize();
        return false;
    }

    voice->SetRate(rate);
    voice->SetVolume(volume);

    ISpStream* fileStream = nullptr;
    if (!outPath.empty()) {
        CSpStreamFormat format;
        format.AssignFormat(SPSF_44kHz16BitMono);
        hr = SPBindToFile(outPath.c_str(), SPFM_CREATE_ALWAYS, &fileStream, &format.FormatId(), format.WaveFormatExPtr());
        if (FAILED(hr) || !fileStream) {
            voice->Release();
            ::CoUninitialize();
            return false;
        }
        hr = voice->SetOutput(fileStream, TRUE);
        if (FAILED(hr)) {
            fileStream->Release();
            voice->Release();
            ::CoUninitialize();
            return false;
        }
    }

    hr = voice->Speak(text.c_str(), SPF_DEFAULT, nullptr);

    if (fileStream) {
        fileStream->Close();
        fileStream->Release();
    }

    voice->Release();
    ::CoUninitialize();
    return SUCCEEDED(hr);
}

bool InitDb(sqlite3* db) {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS documents ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "path TEXT UNIQUE,"
        "content TEXT NOT NULL,"
        "status TEXT NOT NULL DEFAULT 'pending',"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "spoken_at TEXT"
        ");";
    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool ImportFolder(sqlite3* db, const std::string& folder) {
    if (!fs::exists(folder)) {
        std::cerr << "Folder not found: " << folder << "\n";
        return false;
    }

    const char* sql = "INSERT OR IGNORE INTO documents(path, content, status) VALUES(?, ?, 'pending');";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;

        std::ifstream in(entry.path(), std::ios::binary);
        if (!in) continue;
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        std::string pathStr = entry.path().string();
        sqlite3_bind_text(stmt, 1, pathStr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, content.c_str(), static_cast<int>(content.size()), SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_DONE) ++count;
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_finalize(stmt);
    std::cout << "Imported/checked .txt files: " << count << "\n";
    return true;
}

bool ListPending(sqlite3* db) {
    const char* sql = "SELECT id, path FROM documents WHERE status='pending' ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::cout << "Pending items:\n";
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::cout << "  [" << id << "] " << (path ? path : "") << "\n";
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SpeakNextFromDb(sqlite3* db, const Options& opt) {
    const char* selectSql = "SELECT id, content, path FROM documents WHERE status='pending' ORDER BY id LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        std::cout << "No pending text found.\n";
        sqlite3_finalize(stmt);
        return true;
    }

    int id = sqlite3_column_int(stmt, 0);
    const unsigned char* content = sqlite3_column_text(stmt, 1);
    const unsigned char* path = sqlite3_column_text(stmt, 2);
    std::string textUtf8 = content ? reinterpret_cast<const char*>(content) : "";
    std::wstring text = ToWide(textUtf8.c_str());

    sqlite3_finalize(stmt);

    std::wcout << L"Speaking item #" << id << L" from " << ToWide(reinterpret_cast<const char*>(path)) << L"\n";
    if (!Speak(text, opt.outPath, opt.rate, opt.volume)) {
        std::cerr << "TTS failed for item #" << id << "\n";
        return false;
    }

    const char* updateSql = "UPDATE documents SET status='spoken', spoken_at=CURRENT_TIMESTAMP WHERE id=?;";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](const std::string& flag) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << flag << "\n";
                PrintUsage();
                std::exit(1);
            }
            return argv[++i];
        };

        if (arg == "--text") opt.text = ToWide(needValue(arg));
        else if (arg == "--out") opt.outPath = ToWide(needValue(arg));
        else if (arg == "--rate") opt.rate = std::stoi(needValue(arg));
        else if (arg == "--volume") opt.volume = static_cast<unsigned short>(std::stoi(needValue(arg)));
        else if (arg == "--db") opt.dbPath = needValue(arg);
        else if (arg == "--import") opt.importFolder = needValue(arg);
        else if (arg == "--list") opt.list = true;
        else if (arg == "--speak-next") opt.speakNext = true;
        else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        } else {
            std::cerr << "Unknown arg: " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    if (!opt.dbPath.empty()) {
        sqlite3* db = nullptr;
        if (sqlite3_open(opt.dbPath.c_str(), &db) != SQLITE_OK || !db) {
            std::cerr << "Failed to open DB: " << opt.dbPath << "\n";
            if (db) sqlite3_close(db);
            return 1;
        }

        bool ok = InitDb(db);
        if (ok && !opt.importFolder.empty()) ok = ImportFolder(db, opt.importFolder);
        if (ok && opt.list) ok = ListPending(db);
        if (ok && opt.speakNext) ok = SpeakNextFromDb(db, opt);

        sqlite3_close(db);
        return ok ? 0 : 1;
    }

    if (opt.text.empty()) {
        PrintUsage();
        return 1;
    }

    if (!Speak(opt.text, opt.outPath, opt.rate, opt.volume)) {
        std::cerr << "TTS failed\n";
        return 1;
    }

    return 0;
}
