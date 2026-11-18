CXX ?= c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude
LDFLAGS :=

LIB_SOURCES := src/Text.cpp src/Date.cpp src/Models.cpp src/Storage.cpp src/Services.cpp src/Application.cpp
APP_SOURCES := $(LIB_SOURCES) src/main.cpp
TEST_SOURCES := $(LIB_SOURCES) tests/test_main.cpp

.PHONY: all test dashboard-build dashboard-test dashboard-start test-all clean

all: bin/expirybud

bin/expirybud: $(APP_SOURCES)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(APP_SOURCES) $(LDFLAGS) -o $@

bin/expirybud_tests: $(TEST_SOURCES)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) $(LDFLAGS) -o $@

test: bin/expirybud_tests
	./bin/expirybud_tests

dashboard-build:
	npm run build

dashboard-test:
	npm test

dashboard-start: dashboard-build
	npm start

test-all: test dashboard-test

clean:
	rm -f bin/expirybud bin/expirybud_tests
	rm -rf dist
