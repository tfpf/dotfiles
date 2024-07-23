CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
CXXFLAGS = -std=c++17 -Wall -Wextra
LDLIBS = -lstdc++

MainSource = custom-prompt.cc
MainBashObject = custom-bash-prompt.o
MainBashExecutable = bin/$(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = bin/$(MainZshObject:.o=)
GetActiveWidObjects = get_active_wid_linux.o get_active_wid_windows.o

ifeq "$(OS)" "Windows_NT"
    # When building using something other than Visual Studio, the shared
    # libraries the executables are linked with may lie in folders Windows does
    # not typically search. Avoid the hassle of fixing that.
    LDFLAGS += -static
else
    UNAME = $(shell uname)
    ifeq "$(UNAME)" "Linux"
        CPPFLAGS += $(shell pkg-config --cflags glib-2.0 libnotify x11)
        LDLIBS += $(shell pkg-config --libs glib-2.0 libnotify x11)
    else
        LDLIBS += -framework Cocoa -framework CoreGraphics
        GetActiveWidObjects += get_active_wid_macos.o
    endif
endif

.PHONY: debug release

debug: $(MainBashExecutable) $(MainZshExecutable)

release: CPPFLAGS += -DNDEBUG
release: CFLAGS += -flto -O2
release: CXXFLAGS += -flto -O2
release: LDFLAGS += -flto -O2 -s
release: debug

$(MainBashObject): CPPFLAGS += -DBASH
$(MainBashObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(MainBashObject) $(GetActiveWidObjects)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(MainZshObject) $(GetActiveWidObjects)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)
