#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <CoreGraphics/CGWindow.h>

extern "C++" long long unsigned get_active_wid(void)
{
    NSArray *windows = (NSArray *)CGWindowListCopyWindowInfo(
        kCGWindowListExcludeDesktopElements | kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    for (NSDictionary *window in windows)
    {
        int windowLayer = [[window objectForKey: (NSString *)kCGWindowLayer] intValue];
        if (windowLayer == 0)
        {
            return [[window objectForKey: (NSString *)kCGWindowNumber] intValue];
        }
    }
    return 0;
}
#endif
