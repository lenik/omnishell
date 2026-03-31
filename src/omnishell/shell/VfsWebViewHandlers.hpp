#ifndef OMNISHELL_SHELL_VFS_WEB_VIEW_HANDLERS_HPP
#define OMNISHELL_SHELL_VFS_WEB_VIEW_HANDLERS_HPP

#include <string>

class VolumeManager;
class wxWebView;

namespace os {

/**
 * Register vol:// and asset:// wxWebViewHandler implementations that use the same
 * DirIndex / FileController / AssetController logic as HttpDaemon (see src/daemon/).
 * Directory listing HTML is post-processed so in-page links stay on the custom scheme.
 *
 * @param httpBase running daemon base URL (e.g. http://127.0.0.1:4380) for /volume/ index links
 */
void RegisterDaemonWebViewHandlers(wxWebView* web, VolumeManager* vm, const std::string& httpBase);

} // namespace os

#endif
