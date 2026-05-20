#include "Server.hpp"
#include <sstream>
#include <dirent.h>
#include "CGI.hpp"
#include <sys/wait.h>

// ═══════════════════════════════════════════════════════════════════
// MIME TYPE MANAGEMENT - C++98 Compatible
// ═══════════════════════════════════════════════════════════════════
bool Server::loadMimeTypes(const std::string& filepath) {
    std::ifstream file(filepath.c_str());
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not load mime.types from: " << filepath << std::endl;
        // Fallbacks
        _mimeTypeCache[".html"] = "text/html";
        _mimeTypeCache[".css"] = "text/css";
        _mimeTypeCache[".js"] = "application/javascript";
        _mimeTypeCache[".json"] = "application/json";
        _mimeTypeCache[".png"] = "image/png";
        _mimeTypeCache[".jpg"] = "image/jpeg";
        _mimeTypeCache[".jpeg"] = "image/jpeg";
        _mimeTypeCache[".pdf"] = "application/pdf";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find("types") != std::string::npos)
            continue;
        if (line == "}")
            break;

        size_t start = line.find_first_not_of(" \t");
        size_t end = line.find_last_not_of(" \t;");
        if (start == std::string::npos || end == std::string::npos)
            continue;
            
        line = line.substr(start, end - start + 1);

        std::istringstream iss(line);
        std::string mimeType;
        std::string extension;
        
        if (!(iss >> mimeType))
            continue;

        while (iss >> extension) {
            if (!extension.empty() && extension[extension.size() - 1] == ';')
                extension = extension.substr(0, extension.size() - 1);
            if (extension[0] != '.')
                extension = "." + extension;
            _mimeTypeCache[extension] = mimeType;
        }
    }
    file.close();

    if (_mimeTypeCache.empty()) {
        std::cerr << "[WARNING] mime.types file is empty, using fallback types" << std::endl;
        return false;
    }

    std::cout << "[INFO] Loaded " << _mimeTypeCache.size() << " MIME types from " << filepath << std::endl;
    return true;
}

std::string Server::getMimeType(const std::string& path) {
    if (path.empty())
        return "application/octet-stream";
    
    size_t slash = path.rfind('/');
    size_t dot = path.rfind('.');
    
    // Ensure the dot is actually part of the filename, not a directory name
    // e.g., /my.folder/file -> no extension
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return "application/octet-stream";
    
    std::string ext = path.substr(dot);
    
    // Lowercase the extension for safe lookup (e.g., .JPG -> .jpg)
    for (size_t i = 0; i < ext.size(); i++) {
        if (ext[i] >= 'A' && ext[i] <= 'Z')
            ext[i] = ext[i] - 'A' + 'a';
    }
    
    std::map<std::string, std::string>::const_iterator it = _mimeTypeCache.find(ext);
    if (it != _mimeTypeCache.end())
        return it->second;
    
    return "application/octet-stream";
}

// ═══════════════════════════════════════════════════════════════════
// LOCATION MATCHING (longest prefix wins)
// ═══════════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════════
// PATH RESOLUTION (convert URI to filesystem path)
// ═══════════════════════════════════════════════════════════════════
static std::string resolvePath(const std::string& uri, LocationConfig* loc) {
    std::string root = loc->root;
    if (!root.empty() && root[root.size()-1] == '/')
        root.erase(root.size()-1);
    std::string remain = uri.substr(loc->path.size());
    if (remain.empty() || remain[0] != '/')
        remain = "/" + remain;
    return root + remain;
}

// ═══════════════════════════════════════════════════════════════════
// AUTOINDEX GENERATION
// ═══════════════════════════════════════════════════════════════════
static std::string buildAutoindex(const std::string& uri, const std::string& dir_path) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Index of " << uri << "</title></head>\n"
        << "<body><h1>Index of " << uri << "</h1><hr><pre>\n";

    DIR* dir = opendir(dir_path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name == ".") continue;
            bool is_dir = (entry->d_type == DT_DIR);
            html << "<a href=\"" << name << (is_dir ? "/" : "") << "\">"
                << name << (is_dir ? "/" : "") << "</a>\n";
        }
        closedir(dir);
    }
    html << "</pre><hr></body></html>\n";
    return html.str();
}

// ═══════════════════════════════════════════════════════════════════
// FILE UPLOAD
// ═══════════════════════════════════════════════════════════════════
static bool saveUpload(Client& c, LocationConfig* loc) {
    if (loc->upload_store.empty()) return false;

    std::string filename;
    std::map<std::string, std::string> hdrs = c.getHeader();
    
    // Look for filename in headers (Raw binary upload)
    if (hdrs.count("Content-Disposition")) {
        std::string cd = hdrs["Content-Disposition"];
        size_t fn = cd.find("filename=\"");
        if (fn != std::string::npos) {
            fn += 10;
            size_t fe = cd.find("\"", fn);
            if (fe != std::string::npos)
                filename = cd.substr(fn, fe - fn);
        }
    }

    // Fallback if no filename is found
    if (filename.empty()) {
        std::ostringstream ss;
        ss << "upload_" << (size_t)time(NULL) << ".bin";
        filename = ss.str();
    }

    // Resolve where to save the file
    std::string store = loc->upload_store;
    if (store.empty()) store = ".";
    if (store[store.size()-1] != '/') store += "/";

    std::string full_path = store + filename;

    std::ofstream out(full_path.c_str(), std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot open upload file: " << full_path << std::endl;
        return false;
    }
    
    out.write(c.getBody().c_str(), c.getBody().size());
    return out.good();
}

// ═══════════════════════════════════════════════════════════════════
// HTTP STATUS MESSAGES
// ═══════════════════════════════════════════════════════════════════
static const char* getStatusMsg(int code) {
    if (code == 200) return "OK";
    if (code == 201) return "Created";
    if (code == 204) return "No Content";
    if (code == 301) return "Moved Permanently";
    if (code == 302) return "Found";
    if (code == 307) return "Temporary Redirect";
    if (code == 308) return "Permanent Redirect";
    if (code == 400) return "Bad Request";
    if (code == 403) return "Forbidden";
    if (code == 404) return "Not Found";
    if (code == 405) return "Method Not Allowed";
    if (code == 413) return "Content Too Large";
    if (code == 421) return "Misdirected Request";
    if (code == 431) return "Request Header Fields Too Large";
    if (code == 500) return "Internal Server Error";
    if (code == 504) return "Gateway Timeout";
    return "Unknown";
}

bool Server::selectServerByHostname(
    int listen_fd,
    const std::string& host_header,
    ServerConfig*& selected_config
) {
    std::cout << "[DEBUG-VH-1] Called with Host: " << host_header << ", listen_fd: " << listen_fd << std::endl;

    std::map<int, std::vector<int> >::const_iterator it = _fd_to_configs.find(listen_fd);
    if (it == _fd_to_configs.end()) {
        std::cout << "[DEBUG-VH-2] listen_fd NOT found in _fd_to_configs!" << std::endl;
        selected_config = &_configs[0];
        return false;
    }

    const std::vector<int>& config_indices = it->second;
    std::cout << "[DEBUG-VH-3] Found " << config_indices.size() << " configs for this listen_fd" << std::endl;

    // Extract hostname from "Host: example.com:8080"
    std::string requested_hostname = host_header;
    size_t colon = requested_hostname.find(':');
    if (colon != std::string::npos)
        requested_hostname = requested_hostname.substr(0, colon);

    std::cout << "[DEBUG-VH-4] Requested hostname (without port): '" << requested_hostname << "'" << std::endl;

    // Check if ANY config has empty server_names (catch-all)
    bool has_catch_all = false;
    int catch_all_idx = -1;
    for (size_t i = 0; i < config_indices.size(); i++) {
        ServerConfig& srv = _configs[config_indices[i]];
        if (srv.server_names.empty()) {
            has_catch_all = true;
            catch_all_idx = i;
            std::cout << "[DEBUG-VH-4b] Found catch-all (no server_name) at config index " << i << std::endl;
            break;
        }
    }

    // Print all configs and their server_names
    for (size_t i = 0; i < config_indices.size(); i++) {
        int config_idx = config_indices[i];
        ServerConfig& srv = _configs[config_idx];
        std::cout << "[DEBUG-VH-5] Config " << i << " (index=" << config_idx << ") has " 
                  << srv.server_names.size() << " server_name(s): ";
        for (size_t j = 0; j < srv.server_names.size(); j++) {
            std::cout << "'" << srv.server_names[j] << "' ";
        }
        std::cout << std::endl;
    }

    // Step 1: Try exact hostname match
    std::cout << "[DEBUG-VH-6] Step 1: Looking for exact hostname match..." << std::endl;
    for (size_t i = 0; i < config_indices.size(); i++) {
        int config_idx = config_indices[i];
        ServerConfig& srv = _configs[config_idx];
        
        for (size_t j = 0; j < srv.server_names.size(); j++) {
            std::cout << "[DEBUG-VH-7] Checking if '" << srv.server_names[j] 
                      << "' == '" << requested_hostname << "'" << std::endl;
            
            if (srv.server_names[j] == requested_hostname) {
                std::cout << "[DEBUG-VH-8] ✓ EXACT MATCH FOUND at config " << config_idx << std::endl;
                selected_config = &srv;
                return true;
            }
            
            if (srv.server_names[j] == "*") {
                std::cout << "[DEBUG-VH-9] ✓ WILDCARD MATCH FOUND at config " << config_idx << std::endl;
                selected_config = &srv;
                return true;
            }
        }
    }

    // Step 2: If no exact match, check if there's a catch-all server
    if (has_catch_all) {
        std::cout << "[DEBUG-VH-11] No exact match, but catch-all exists. Using catch-all config." << std::endl;
        selected_config = &_configs[config_indices[catch_all_idx]];
        return false;
    }

    // Step 3: No exact match AND no catch-all = MISDIRECTED REQUEST
    std::cout << "[DEBUG-VH-12] ✗ NO MATCH and NO CATCH-ALL! This is a 421 error." << std::endl;
    selected_config = &_configs[config_indices[0]];  // Set to something valid, but we'll reject in buildResponse
    return false;  // Return false to signal error
}

// ═══════════════════════════════════════════════════════════════════
// BUILD RESPONSE (main handler)
// ═══════════════════════════════════════════════════════════════════
void Server::buildResponse(Client& c) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ BUILD RESPONSE - Virtual Hosting Debug" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    // Select server by hostname
    ServerConfig* srv_ptr = NULL;
    std::string host_header = c.getHeader()["Host"];
    std::cout << "[VH-START] Host header: '" << host_header << "'" << std::endl;
    
    bool matched = selectServerByHostname(c.getListenFd(), host_header, srv_ptr);
    ServerConfig& srv = *srv_ptr;
    
    std::cout << "[VH-RESULT] Server names in selected config: ";
    for (size_t i = 0; i < srv.server_names.size(); i++)
        std::cout << "'" << srv.server_names[i] << "' ";
    std::cout << std::endl;

    // ⭐ Check if this is a misdirected request (no match + all servers have explicit names)
    if (!matched && !srv.server_names.empty()) {
        std::cout << "[VH-ERROR] 421: Hostname '" << host_header << "' not configured!" << std::endl;
        std::string body = "<html><body><h1>421 Misdirected Request</h1>"
                           "<p>Hostname '" + host_header + "' not configured on this server</p></body></html>";
        std::ostringstream oss;
        oss << "HTTP/1.1 421 Misdirected Request\r\n"
            << "Server: Webserv/1.0\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n"
            << body;
        c.sendBuf() = oss.str();
        c.setFileSize(c.sendBuf().size());
        return;
    }

    // Handle early errors (400, 413, 431, etc)
    if (c.getErrorCode() != 0) {
        std::string msg = getStatusMsg(c.getErrorCode());
        c.sendBuf() = buildErrorResponse(c.getErrorCode(), msg, srv);
        c.setFileSize(c.sendBuf().size());
        return;
    }

    // =========================================================================
    // FIX: Extract Query String so we don't try to open "test.py?name=naar"
    // =========================================================================
    std::string full_path = c.getPath();
    std::string uri_path = full_path;
    std::string query_string = "";
    
    size_t q_pos = full_path.find('?');
    if (q_pos != std::string::npos) {
        uri_path = full_path.substr(0, q_pos);
        query_string = full_path.substr(q_pos + 1); // Strip the '?'
    }

    // Now use uri_path for matching and routing!
    LocationConfig* loc = matchLocation(srv, uri_path);

    // --- 1. REDIRECT ---
    if (loc && loc->return_url.first != 0) {
        std::ostringstream oss;
        int code = loc->return_url.first;
        oss << "HTTP/1.1 " << code << " " << getStatusMsg(code) << "\r\n"
            << "Location: " << loc->return_url.second << "\r\n"
            << "Content-Length: 0\r\n"
            << "Connection: close\r\n\r\n";
        c.sendBuf() = oss.str();
        c.setFileSize(c.sendBuf().size());
        std::cout << "[RESPONSE] " << code << " -> " << loc->return_url.second << std::endl;
        return;
    }

    // --- 2. METHOD VALIDATION ---
    if (loc && !loc->allowed_methods.empty()) {
        bool allowed = false;
        for (size_t i = 0; i < loc->allowed_methods.size(); ++i)
            if (loc->allowed_methods[i] == c.getMethod()) { allowed = true; break; }
        if (!allowed) {
            c.sendBuf() = buildErrorResponse(405, "Method Not Allowed", srv);
            c.setFileSize(c.sendBuf().size());
            std::cout << "[RESPONSE] 405: " << c.getMethod() << " not allowed" << std::endl;
            return;
        }
    }

    // No location or root -> 404
    if (!loc || loc->root.empty()) {
        c.sendBuf() = buildErrorResponse(404, "Not Found", srv);
        c.setFileSize(c.sendBuf().size());
        return;
    }

    std::string physical = resolvePath(uri_path, loc);
    std::cout << "[ROUTE] " << c.getMethod() << " " << uri_path << " -> " << physical << std::endl;

    // --- 3. CGI HANDLING ---
    if (!loc->cgi_pass.empty()) {
        size_t dot = physical.rfind('.');
        if (dot != std::string::npos) {
            std::string ext = physical.substr(dot);

            if (loc->cgi_pass.find(ext) != loc->cgi_pass.end()) {
                std::string interpreter = loc->cgi_pass[ext];

                if (access(physical.c_str(), F_OK) != 0) {
                    c.sendBuf() = buildErrorResponse(404, "Script Not Found", srv);
                    c.setFileSize(c.sendBuf().size());
                    return;
                }
                if (access(physical.c_str(), X_OK) != 0) {
                    c.sendBuf() = buildErrorResponse(403, "Script Not Executable", srv);
                    c.setFileSize(c.sendBuf().size());
                    return;
                }

                // Setup the CGI parser using the clean paths
                CGI cgi;
                cgi.setMethod(c.getMethod());
                cgi.setPath(physical);
                cgi.setQuery(query_string); // PASS EXTRACTED QUERY HERE!
                cgi.setBody(c.getBody());
                cgi.setContentType(c.getHeader().count("Content-Type") ? c.getHeader()["Content-Type"] : "");
                cgi.setHost(c.getHeader().count("Host") ? c.getHeader()["Host"] : "localhost");

                pid_t pid = -1;
                int pipe_fd = cgi.executeAsync(interpreter, pid);

                if (pipe_fd < 0) {
                    c.sendBuf() = buildErrorResponse(500, "Internal Server Error", srv);
                    c.setFileSize(c.sendBuf().size());
                    return;
                }

                c.cgi_fd = pipe_fd;
                c.cgi_pid = pid;
                c.cgi_raw_output.clear();

                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = pipe_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd, &ev);

                cgi_clients[pipe_fd] = &c;
                
                c.setState(PROCESS_CGI);
                std::cout << "[CGI] Async process started, PID: " << pid << " on FD: " << pipe_fd << std::endl;
                return;
            }
        }
    }

    // --- 4. DELETE ---
    if (c.getMethod() == "DELETE") {
        struct stat st;
        if (physical.empty() || stat(physical.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
            c.sendBuf() = buildErrorResponse(404, "Not Found", srv);
        } else if (remove(physical.c_str()) != 0) {
            c.sendBuf() = buildErrorResponse(403, "Forbidden", srv);
        } else {
            std::ostringstream oss;
            oss << "HTTP/1.1 204 No Content\r\n"
                << "Server: Webserv/1.0\r\n"
                << "Content-Length: 0\r\n"
                << "Connection: close\r\n\r\n";
            c.sendBuf() = oss.str();
            std::cout << "[RESPONSE] 204 Deleted: " << physical << std::endl;
        }
        c.setFileSize(c.sendBuf().size());
        return;
    }

    // --- 5. POST (file upload) ---
    if (c.getMethod() == "POST") {
        if (loc && !loc->upload_store.empty()) {
            if (saveUpload(c, loc)) {
                std::string body = "<html><body><h1>201 Created</h1></body></html>";
                std::ostringstream oss;
                oss << "HTTP/1.1 201 Created\r\n"
                    << "Server: Webserv/1.0\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n\r\n" << body;
                c.sendBuf() = oss.str();
                std::cout << "[RESPONSE] 201 Created" << std::endl;
            } else {
                c.sendBuf() = buildErrorResponse(500, "Internal Server Error", srv);
            }
        } else {
            std::ostringstream oss;
            oss << "HTTP/1.1 204 No Content\r\n"
                << "Server: Webserv/1.0\r\n"
                << "Content-Length: 0\r\n"
                << "Connection: close\r\n\r\n";
            c.sendBuf() = oss.str();
        }
        c.setFileSize(c.sendBuf().size());
        return;
    }

    // --- 6. GET ---
    struct stat st;

    if (stat(physical.c_str(), &st) != 0) {
        c.sendBuf() = buildErrorResponse(404, "Not Found", srv);
        c.setFileSize(c.sendBuf().size());
        std::cout << "[RESPONSE] 404: " << physical << std::endl;
        return;
    }

    // Directory handling
    if (S_ISDIR(st.st_mode)) {
        // Missing trailing slash -> redirect
        if (uri_path[uri_path.size()-1] != '/') {
            std::ostringstream oss;
            oss << "HTTP/1.1 301 " << getStatusMsg(301) << "\r\n"
                << "Location: " << uri_path << "/\r\n"
                << "Content-Length: 0\r\n"
                << "Connection: close\r\n\r\n";
            c.sendBuf() = oss.str();
            c.setFileSize(c.sendBuf().size());
            return;
        }

        bool index_found = false;
        if (!loc->index.empty()) {
            std::string index_path = physical;
            if (index_path[index_path.size()-1] != '/') index_path += "/";
            index_path += loc->index;
            struct stat ist;
            if (stat(index_path.c_str(), &ist) == 0 && S_ISREG(ist.st_mode)) {
                physical = index_path;
                st = ist;
                index_found = true; 
            }
        }

        if (!index_found) {
            if (loc->autoindex) {
                std::string body = buildAutoindex(uri_path, physical);
                std::ostringstream oss;
                oss << "HTTP/1.1 200 " << getStatusMsg(200) << "\r\n"
                    << "Server: Webserv/1.0\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Connection: " << (c.isKeepAlive() ? "keep-alive" : "close") << "\r\n\r\n"
                    << body;
                c.sendBuf() = oss.str();
                c.setFileSize(c.sendBuf().size());
                std::cout << "[RESPONSE] 200 Autoindex" << std::endl;
                return;
            }

            c.sendBuf() = buildErrorResponse(403, "Forbidden", srv);
            c.setFileSize(c.sendBuf().size());
            return;
        }
    }

    if (!S_ISREG(st.st_mode)) {
        c.sendBuf() = buildErrorResponse(404, "Not Found", srv);
        c.setFileSize(c.sendBuf().size());
        return;
    }

    c.openFile(physical);
    if (!c.file_stream.is_open()) {
        c.sendBuf() = buildErrorResponse(403, "Forbidden", srv);
        c.setFileSize(c.sendBuf().size());
        return;
    }

    c.setFileSize(st.st_size);
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 200 " << getStatusMsg(200) << "\r\n"
            << "Server: Webserv/1.0\r\n"
            << "Content-Length: " << st.st_size << "\r\n"
            << "Content-Type: " << getMimeType(physical) << "\r\n"
            << "Connection: " << (c.isKeepAlive() ? "keep-alive" : "close") << "\r\n\r\n";
        c.sendBuf() = oss.str();
    }
    std::cout << "[RESPONSE] 200 OK: " << physical << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// SEND RESPONSE (streaming)
// ═══════════════════════════════════════════════════════════════════
void Server::handleResponse(int fd) {
    Client& c = *clients[fd];
    char chunk[8192];

    // Phase 1: send headers (and full body for non-file responses)
    if (!c.headerSent()) {
        ssize_t n = send(fd, c.sendBuf().c_str(), c.sendBuf().size(), 0);
	if (n > 0) {
		c.sendBuf().erase(0, n);
		if (c.sendBuf().empty())
			c.setHeaderSent(true);
	} else if (n < 0) {
		c.setState(CLOSED);
		return;
	}
        if (!c.headerSent()) return;
    }

    // Phase 2: stream file body (only for real file responses)
    if (c.file_stream.is_open()) {
        if (c.getBytesSent() < c.getFileSize()) {
            c.file_stream.clear();
            c.file_stream.seekg(static_cast<std::streamoff>(c.getBytesSent()));

            std::streamsize actual = c.readFile(chunk, sizeof(chunk));
            if (actual <= 0) { c.setState(CLOSED); return; }

            ssize_t n = send(fd, chunk, static_cast<size_t>(actual), 0);
            if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                c.setState(CLOSED);
                return;
            }
            if (n > 0) c.setBytesSent(static_cast<size_t>(n));
            return;
        }
    }

    // Phase 3: done
    bool file_done = !c.file_stream.is_open() ||
                     (c.getBytesSent() >= c.getFileSize());

    if (c.sendBuf().empty() && file_done) {
        std::cout << "[DONE] fd=" << fd
                  << (c.isKeepAlive() ? " keep-alive" : " close") << std::endl;
        if (c.isKeepAlive()) {
            c.reset();
            modifyEpoll(fd, EPOLLIN);
        } else {
            c.setState(CLOSED);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// ASYNC CGI READING
// ═══════════════════════════════════════════════════════════════════
void Server::handleCGIRead(int pipe_fd) {
    Client* c = cgi_clients[pipe_fd];
    char buf[4096];

    ssize_t n = read(pipe_fd, buf, sizeof(buf));

    if (n > 0) {
        // We read some data, append it to the client's buffer
        c->cgi_raw_output.append(buf, n);
        c->updateActivityTime(); // Keep the client from timing out
    }
    else if (n == 0) {
        // EOF: The script has finished writing and closed the pipe!
        std::cout << "[CGI] Process PID " << c->cgi_pid << " finished writing." << std::endl;

        // 1. Cleanup the pipe and process
        epoll_ctl(epfd, EPOLL_CTL_DEL, pipe_fd, NULL);
        close(pipe_fd);
        cgi_clients.erase(pipe_fd);
        waitpid(c->cgi_pid, NULL, 0); // Clean up the zombie process

        c->cgi_fd = -1;
        c->cgi_pid = -1;

        // 2. Parse the output we collected
        CGI cgi_parser;
        cgi_parser.parseOutput(c->cgi_raw_output);

        int status = cgi_parser.getStatus();
        std::string body = cgi_parser.getBody();
        std::string headers = cgi_parser.getHeaders();

        // 3. Build the final HTTP response
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status << " " << getStatusMsg(status) << "\r\n"
            << "Server: Webserv/1.0\r\n"
            << "Content-Length: " << body.size() << "\r\n";

        if (!headers.empty()) oss << headers << "\r\n";
        else oss << "Content-Type: text/html\r\n";

        oss << "Connection: " << (c->isKeepAlive() ? "keep-alive" : "close") << "\r\n\r\n";
        if (!body.empty()) oss << body;

        c->sendBuf() = oss.str();
        c->setFileSize(c->sendBuf().size());

        // 4. Switch client to WRITE_RESPONSE and wake up the client socket
        c->setState(WRITE_RESPONSE);
        modifyEpoll(c->getFd(), EPOLLOUT);
    }
    // If n == -1 and errno == EAGAIN, we just do nothing and wait for next epoll trigger
}
