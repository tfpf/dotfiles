CC = gcc
CPPFLAGS = $(shell pkg-config --cflags libgit2)
CFLAGS = -fstrict-aliasing -std=c11 -Wall -Wextra -Wno-unused-parameter
CXXFLAGS = -fstrict-aliasing -std=c++17 -Wall -Wextra -Wno-unused-parameter
LDLIBS = -lstdc++ $(shell pkg-config --libs libgit2)

MainSource = custom-prompt.cc
MainBashObject = custom-bash-prompt.o
MainBashExecutable = bin/$(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = bin/$(MainZshObject:.o=)
GetActiveWidObjects = get_active_wid_linux.o get_active_wid_windows.o

ifeq "$(OS)" "Windows_NT"
    LDLIBS += -lws2_32 -lwinhttp -lcrypt32 -lrpcrt4 -lole32
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
release: LDFLAGS += -flto -O2
release: debug

$(MainBashObject): CPPFLAGS += -DBASH
$(MainBashObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(GetActiveWidObjects) $(MainBashObject)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(GetActiveWidObjects) $(MainZshObject)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)
