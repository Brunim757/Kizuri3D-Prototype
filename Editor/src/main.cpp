#include "EditorApp.h"
#include <cstdio>
#include <exception>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif
static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
  char message[256];
  if (ep != nullptr && ep->ExceptionRecord != nullptr) {
    const EXCEPTION_RECORD* rec = ep->ExceptionRecord;
    std::snprintf(message, sizeof(message), "Kizuri3D encontrou uma falha.\n\nCodigo: 0x%08X\nEndereco: %p", rec->ExceptionCode, rec->ExceptionAddress);
  } else {
    std::snprintf(message, sizeof(message), "Kizuri3D encontrou uma falha desconhecida.");
  }
  MessageBoxA(nullptr, message, "Kizuri3D Editor", MB_OK | MB_ICONERROR);
  return EXCEPTION_EXECUTE_HANDLER;
}
static std::string Narrow(const wchar_t* value) {
  int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
  if (size <= 1) {
    return std::string();
  }
  std::string result(static_cast<size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
  result.resize(static_cast<size_t>(size - 1));
  return result;
}
static std::vector<std::string> CommandLineArgs() {
  int count = 0;
  LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
  std::vector<std::string> args;
  if (wide == nullptr) {
    return args;
  }
  for (int i = 0; i < count; ++i) {
    args.push_back(Narrow(wide[i]));
  }
  LocalFree(wide);
  return args;
}
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  SetUnhandledExceptionFilter(&CrashHandler);
  try {
    Kizuri::EditorApp app;
    const std::vector<std::string> args = CommandLineArgs();
    for (size_t i = 1; i < args.size(); ++i) {
      const std::string& arg = args[i];
      if (arg == "--no-audio") {
        app.SetNoAudio(true);
      } else if (arg == "--no-assets") {
        app.SetNoAssets(true);
      } else if (arg == "--no-physics") {
        app.SetNoPhysics(true);
      }
    }
    if (!app.Initialize()) {
      std::string detail = app.LastInitError();
      if (detail.empty()) {
        detail = "Falha desconhecida.";
      }
      std::string message = "Kizuri3D nao iniciou.\n" + detail + "\nVerifique DirectX 11 e os arquivos Shaders/ e Assets/ ao lado do executavel.";
      MessageBoxA(nullptr, message.c_str(), "Kizuri3D Editor", MB_OK | MB_ICONERROR);
      return 1;
    }
    const int code = app.Run();
    app.Shutdown();
    return code;
  } catch (const std::exception& e) {
    std::string message = std::string("Kizuri3D Editor\n\n") + e.what();
    MessageBoxA(nullptr, message.c_str(), "Kizuri3D Editor", MB_OK | MB_ICONERROR);
    return 1;
  } catch (...) {
    MessageBoxA(nullptr, "Kizuri3D Editor\n\nExcecao desconhecida.", "Kizuri3D Editor", MB_OK | MB_ICONERROR);
    return 1;
  }
}
