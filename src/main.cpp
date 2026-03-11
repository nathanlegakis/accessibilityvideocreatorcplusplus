#include <sapi.h>
#include <sphelper.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

namespace {

void PrintUsage() {
    std::wcout << L"Usage:\n"
               << L"  accessibility_video_creator --text \"Hello\" [--out output.wav] [--rate -10..10] [--volume 0..100]\n\n"
               << L"Examples:\n"
               << L"  accessibility_video_creator --text \"Welcome to Windows 11 TTS\"\n"
               << L"  accessibility_video_creator --text \"Save this speech\" --out speech.wav --rate 1 --volume 90\n";
}

std::wstring ToWide(const char* s) {
    if (!s) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring out(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out.data(), len);
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    std::wstring text;
    std::wstring outPath;
    int rate = 0;
    unsigned short volume = 100;

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

        if (arg == "--text") {
            text = ToWide(needValue(arg));
        } else if (arg == "--out") {
            outPath = ToWide(needValue(arg));
        } else if (arg == "--rate") {
            rate = std::stoi(needValue(arg));
            if (rate < -10 || rate > 10) {
                std::cerr << "--rate must be between -10 and 10\n";
                return 1;
            }
        } else if (arg == "--volume") {
            int v = std::stoi(needValue(arg));
            if (v < 0 || v > 100) {
                std::cerr << "--volume must be between 0 and 100\n";
                return 1;
            }
            volume = static_cast<unsigned short>(v);
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    if (text.empty()) {
        PrintUsage();
        return 1;
    }

    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "COM initialization failed: 0x" << std::hex << hr << "\n";
        return 1;
    }

    ISpVoice* voice = nullptr;
    hr = ::CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&voice));
    if (FAILED(hr) || !voice) {
        std::cerr << "Failed to create SAPI voice: 0x" << std::hex << hr << "\n";
        ::CoUninitialize();
        return 1;
    }

    voice->SetRate(rate);
    voice->SetVolume(volume);

    ISpStream* fileStream = nullptr;
    if (!outPath.empty()) {
        CSpStreamFormat format;
        format.AssignFormat(SPSF_44kHz16BitMono);

        hr = SPBindToFile(outPath.c_str(), SPFM_CREATE_ALWAYS, &fileStream, &format.FormatId(), format.WaveFormatExPtr());
        if (FAILED(hr) || !fileStream) {
            std::cerr << "Failed to open output file: 0x" << std::hex << hr << "\n";
            voice->Release();
            ::CoUninitialize();
            return 1;
        }

        hr = voice->SetOutput(fileStream, TRUE);
        if (FAILED(hr)) {
            std::cerr << "Failed to set file output: 0x" << std::hex << hr << "\n";
            fileStream->Release();
            voice->Release();
            ::CoUninitialize();
            return 1;
        }
    }

    hr = voice->Speak(text.c_str(), SPF_DEFAULT, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Speak failed: 0x" << std::hex << hr << "\n";
        if (fileStream) fileStream->Release();
        voice->Release();
        ::CoUninitialize();
        return 1;
    }

    if (fileStream) {
        fileStream->Close();
        fileStream->Release();
        std::wcout << L"Saved speech to: " << outPath << L"\n";
    }

    voice->Release();
    ::CoUninitialize();

    return 0;
}
