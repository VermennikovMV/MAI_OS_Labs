#include <iostream>
#include <string>
#include <sstream>
#include <dlfcn.h>

int main() {
    std::cout << "Program2 (Runtime loading)\n";

    std::string derivativeLibName = "./libderivative1.so";
    std::string squareLibName = "./libsquare1.so";
    
    typedef float (*DerivativeFunc)(float, float);
    typedef float (*SquareFunc)(float, float);

    auto loadLibraries = [&](const std::string &dName, const std::string &sName,
                             void*& dHandle, void*& sHandle,
                             DerivativeFunc &dFunc, SquareFunc &sFunc) {
        dHandle = dlopen(dName.c_str(), RTLD_LAZY);
        if (!dHandle) {
            std::cerr << "Error loading " << dName << ": " << dlerror() << "\n";
            return false;
        }

        sHandle = dlopen(sName.c_str(), RTLD_LAZY);
        if (!sHandle) {
            std::cerr << "Error loading " << sName << ": " << dlerror() << "\n";
            dlclose(dHandle);
            return false;
        }

        dlerror(); // очистка ошибок
        dFunc = (DerivativeFunc)dlsym(dHandle, "Derivative");
        const char *err = dlerror();
        if (err) {
            std::cerr << "Error dlsym Derivative: " << err << "\n";
            dlclose(dHandle);
            dlclose(sHandle);
            return false;
        }

        dlerror(); // очистка ошибок
        sFunc = (SquareFunc)dlsym(sHandle, "Square");
        err = dlerror();
        if (err) {
            std::cerr << "Error dlsym Square: " << err << "\n";
            dlclose(dHandle);
            dlclose(sHandle);
            return false;
        }

        return true;
    };

    void* dHandle = nullptr;
    void* sHandle = nullptr;
    DerivativeFunc DerivativePtr = nullptr;
    SquareFunc SquarePtr = nullptr;

    // Загрузка начальных библиотек
    if (!loadLibraries(derivativeLibName, squareLibName, dHandle, sHandle, DerivativePtr, SquarePtr)) {
        return 1;
    }

    std::string line;
    bool useFirstSet = true; // true - первая пара, false - вторая пара

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        int cmd;
        iss >> cmd;
        if (!iss) continue;

        if (cmd == 0) {
            // Переключаем реализацию
            dlclose(dHandle);
            dlclose(sHandle);

            useFirstSet = !useFirstSet;
            if (useFirstSet) {
                derivativeLibName = "./libderivative1.so";
                squareLibName = "./libsquare1.so";
                std::cout << "Switched to first set of libraries.\n";
            } else {
                derivativeLibName = "./libderivative2.so";
                squareLibName = "./libsquare2.so";
                std::cout << "Switched to second set of libraries.\n";
            }

            if (!loadLibraries(derivativeLibName, squareLibName, dHandle, sHandle, DerivativePtr, SquarePtr)) {
                return 1; // ошибка при повторной загрузке
            }

        } else if (cmd == 1) {
            float A, deltaX;
            iss >> A >> deltaX;
            if (iss) {
                float res = DerivativePtr(A, deltaX);
                std::cout << "Derivative(" << A << ", " << deltaX << ") = " << res << "\n";
            } else {
                std::cout << "Invalid input for cmd=1\n";
            }

        } else if (cmd == 2) {
            float A, B;
            iss >> A >> B;
            if (iss) {
                float res = SquarePtr(A, B);
                std::cout << "Square(" << A << ", " << B << ") = " << res << "\n";
            } else {
                std::cout << "Invalid input for cmd=2\n";
            }
        } else {
            std::cout << "Unknown command.\n";
        }
    }

    // Освобождение ресурсов
    dlclose(dHandle);
    dlclose(sHandle);

    return 0;
}
