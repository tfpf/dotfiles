import platform

from kitty.tab_bar import DrawData, draw_tab_with_powerline

_modifier_key = "⌘" if platform.system() == "Darwin" else "⎇"
_title_template = (
    "{fmt.fg.red}{activity_symbol or ' '}{fmt.fg.darkmagenta}" + _modifier_key + " {index}{fmt.fg.tab} {title}"
)


def draw_tab(draw_data: DrawData, *args) -> int:
    # The private-looking method I call here is documented, so I assume it is
    # not actually private.
    return draw_tab_with_powerline(draw_data._replace(title_template=_title_template), *args)
