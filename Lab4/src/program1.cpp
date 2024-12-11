#include <iostream>
#include <string>
#include <sstream>

#include "derivative.h"
#include "square.h"

int main() {
    std::cout << "Program1 (Linked at compile time with libderivative1 and libsquare1)\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        int cmd;
        iss >> cmd;
        if (!iss) continue;

        if (cmd == 0) {
            std::cout << "Program1: Switching contracts is not supported here.\n";
        } else if (cmd == 1) {
            float A, deltaX;
            iss >> A >> deltaX;
            if (iss) {
                float res = Derivative(A, deltaX);
                std::cout << "Derivative(" << A << ", " << deltaX << ") = " << res << "\n";
            } else {
                std::cout << "Invalid input for cmd=1\n";
            }
        } else if (cmd == 2) {
            float A, B;
            iss >> A >> B;
            if (iss) {
                float res = Square(A, B);
                std::cout << "Square(" << A << ", " << B << ") = " << res << "\n";
            } else {
                std::cout << "Invalid input for cmd=2\n";
            }
        } else {
            std::cout << "Unknown command.\n";
        }
    }
    return 0;
}
