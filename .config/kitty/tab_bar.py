import platform

from kitty.fast_data_types import Screen
from kitty.tab_bar import DrawData, ExtraData, TabBarData, draw_tab_with_powerline

_modifier_key = "⌘" if platform.system() == "Darwin" else "⎇"
_title_template = (
    "{fmt.fg.red}{activity_symbol or ' '}{fmt.fg.darkmagenta}" + _modifier_key + " {index}{fmt.fg.tab} {title}"
)


def draw_tab(
    draw_data: DrawData,
    screen: Screen,
    tab: TabBarData,
    before: int,
    max_title_length: int,
    index: int,
    is_last: bool,
    extra_data: ExtraData,
) -> int:
    return draw_tab_with_powerline(
        draw_data._replace(title_template=_title_template),
        screen,
        tab,
        before,
        max_title_length,
        index,
        is_last,
        extra_data,
    )
