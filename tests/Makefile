# Host tests for source/DisplayConfig.hpp. The binary stays out of the repo.
CXX ?= g++
CXXFLAGS ?= -std=c++23 -O1 -Wall -Wextra -Werror -pedantic -fno-exceptions
OUT ?= /tmp/fpslocker-display-config-test

.PHONY: test
test:
	$(CXX) $(CXXFLAGS) -I../source -o $(OUT) display_config_test.cpp
	$(OUT)
