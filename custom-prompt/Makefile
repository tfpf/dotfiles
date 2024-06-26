CFLAGS = -std=c11 -Wall -Wextra

MainSource = custom-prompt.c
MainBashObject = custom-bash-prompt.o
MainBashExecutable = bin/$(MainBashObject:.o=)
MainZshObject = custom-zsh-prompt.o
MainZshExecutable = bin/$(MainZshObject:.o=)
GetActiveWidObjects = get_active_wid_linux.o get_active_wid_windows.o

ifneq "$(OS)" "Windows_NT"
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
release: LDFLAGS += -flto -O2 -s
release: debug

$(MainBashObject): CPPFLAGS += -DBASH
$(MainBashObject): $(MainSource)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(MainBashExecutable): $(MainBashObject) $(GetActiveWidObjects)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)

$(MainZshObject): CPPFLAGS += -DZSH
$(MainZshObject): $(MainSource)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(MainZshExecutable): $(MainZshObject) $(GetActiveWidObjects)
	$(LINK.o) $^ $(LDLIBS) $(OUTPUT_OPTION)
