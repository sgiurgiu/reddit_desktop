#include "reddit_extensions.h"
#include "registry.h"
#include "plugin.h"
#include "spoiler.h"

static int reddit_extensions_registration(cmark_plugin *plugin)
{
    cmark_plugin_register_syntax_extension(plugin, create_spoiler_extension());
    return 1;
}

void reddit_extensions_ensure_registered(void)
{
    static int reddit_registered = 0;
    if (!reddit_registered) {
      cmark_register_plugin(reddit_extensions_registration);
      reddit_registered = 1;
    }
}
