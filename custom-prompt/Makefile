CFLAGS = -flto -O2 -std=c11 -Wall -Wextra
LDFLAGS = -flto -O2

MainSource = custom-prompt.c
MainBashObject = custom-bash-prompt.o
MainBashExecutable = $(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = $(MainZshObject:.o=)
GetActiveWindowIdObjects = get_active_window_id_linux.o get_active_window_id_windows.o

ifneq "$(OS)" "Windows_NT"
    UNAME = $(shell uname)
    ifeq "$(UNAME)" "Linux"
        CPPFLAGS += $(shell pkg-config --cflags glib-2.0 libnotify x11)
        LDLIBS += $(shell pkg-config --libs glib-2.0 libnotify x11)
    else
        LDLIBS += -framework Cocoa -framework CoreGraphics
        GetActiveWindowIdObjects += get_active_window_id_macos.o
    endif
endif

.PHONY: debug release

debug: $(MainBashExecutable) $(MainZshExecutable)

release: CPPFLAGS += -DNDEBUG
release: LDFLAGS += -s
release: debug

$(MainBashObject): CPPFLAGS += -DBASH
$(MainBashObject): $(MainSource)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(MainBashObject) $(GetActiveWindowIdObjects)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(MainZshObject) $(GetActiveWindowIdObjects)
