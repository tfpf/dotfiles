CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
CXXFLAGS = -std=c++17 -Wall -Wextra

MainSource = custom-prompt.c
MainBashObject = custom-bash-prompt.o
MainBashExecutable = bin/$(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = bin/$(MainZshObject:.o=)
GetGitInfoSource = get_git_info.cc
GetGitInfoBashObject = get_bash_git_info.o
GetGitInfoZshObject = get_zsh_git_info.o
GetActiveWidObjects = get_active_wid_linux.o get_active_wid_windows.o

ifeq "$(OS)" "Windows_NT"
    # MSYS2 places the C++ standard library file in a folder Windows doesn't
    # search for DLL files, so just link statically.
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
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(GetGitInfoBashObject): CPPFLAGS += -DBASH
$(GetGitInfoBashObject): $(GetGitInfoSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(MainBashObject) $(GetActiveWidObjects) $(GetGitInfoBashObject)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(GetGitInfoZshObject): CPPFLAGS += -DZSH
$(GetGitInfoZshObject): $(GetGitInfoSource)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(MainZshObject) $(GetActiveWidObjects) $(GetGitInfoZshObject)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) $(OUTPUT_OPTION)
