CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -fPIC
LDFLAGS = 
LIBS = 
DLFLAGS = -ldl

# Имя целевых файлов
DERIV1_LIB = libderivative1.so
DERIV2_LIB = libderivative2.so
SQUARE1_LIB = libsquare1.so
SQUARE2_LIB = libsquare2.so

PROG1 = program1
PROG2 = program2

all: $(DERIV1_LIB) $(DERIV2_LIB) $(SQUARE1_LIB) $(SQUARE2_LIB) $(PROG1) $(PROG2)

# Правила сборки динамических библиотек
$(DERIV1_LIB): derivative1.cpp derivative.h
	$(CXX) $(CXXFLAGS) -shared -o $@ derivative1.cpp -lm

$(DERIV2_LIB): derivative2.cpp derivative.h
	$(CXX) $(CXXFLAGS) -shared -o $@ derivative2.cpp -lm

$(SQUARE1_LIB): square1.cpp square.h
	$(CXX) $(CXXFLAGS) -shared -o $@ square1.cpp

$(SQUARE2_LIB): square2.cpp square.h
	$(CXX) $(CXXFLAGS) -shared -o $@ square2.cpp

# Сборка программ
# program1 линкуется с первой реализацией библиотек (derivative1 и square1)
$(PROG1): program1.cpp derivative.h square.h $(DERIV1_LIB) $(SQUARE1_LIB)
	$(CXX) $(CXXFLAGS) -o $(PROG1) program1.cpp -L. -lderivative1 -lsquare1

# program2 использует dlopen, поэтому линкуем с -ldl
$(PROG2): program2.cpp derivative.h square.h $(DERIV1_LIB) $(DERIV2_LIB) $(SQUARE1_LIB) $(SQUARE2_LIB)
	$(CXX) $(CXXFLAGS) -o $(PROG2) program2.cpp $(DLFLAGS)

clean:
	rm -f $(PROG1) $(PROG2) $(DERIV1_LIB) $(DERIV2_LIB) $(SQUARE1_LIB) $(SQUARE2_LIB)

.PHONY: all clean
