#include "Server.hpp"
#include <sstream>

// normalize the path (hna y9dr ikon path traversal o tssd9 mkhli server yfot root so each .. ghadi nrj3o lor 7ta nwsslo root o n7bsso)

static	std::string normalizePath(const std::string& path) {
	if (path.empty() || path[0] != '/') return "";

	std::vector<std::string> segments;
	std::istringstream ss(path);
	std::string seg;

	while (std::getline(ss, seg, '/')) {
		if (seg.empty() || seg == ".") continue;
		if (seg == "..") {
			if (!segments.empty()) segments.pop_back();
			else return "";
		} else {
			segments.push_back(seg);
		}
	}
	std::string result = "/";
	for (size_t i = 0; i < segments.size(); i++) {
		if (i > 0) result += "/";
		result += segments[i];
	}
	if (path[path.size() - 1] == '/' && result != "/")
		result += "/";
	return result;
}


