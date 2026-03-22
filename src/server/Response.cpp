#include "Server.hpp"
#include <sstream>
#include <dirent.h>


// mime types
static std::string getMimeType(const std::string& path) {
	size_t dot = path.rfind('.');
	if (dot == std::string::npos) return "application/octet-stream";
	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".ico") return "image/x-icon";
	if (ext == ".svg") return "image/svg+xml";
	if (ext == ".mp4") return "video/mp4";
	if (ext == ".pdf") return "application/pdf";
	if (ext == ".txt") return "text/plain";
	return "application/octet-stream";
}

// routing (longest prefix wins)

static LocationConfig* matchLocation(ServerConfig& srv, const std::string& uri) {
	LocationConfig* best = NULL;
	size_t longest = 0;

	for (size_t i = 0; i < srv.locations.size(); i++) {
		const std::string& lp = srv.locations[i].path;
		if (uri.find(lp) != 0) continue;
		size_t end_idx = lp.size();
		if (end_idx != uri.size() && uri[end_idx] != '/' && lp[lp.size() - 1] != '/')
			continue;
		if (lp.size() > longest) {
			longest = lp.size();
			best = &srv.locations[i];
		}
	}
	return best;
}


