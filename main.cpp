#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <ctime>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#endif

using namespace std;

// HTTP 服务器类
class HTTPServer {
private:
    int server_fd;
    int port;

    // JSON 解析辅助函数：从 JSON body 中提取 key 对应的字符串值
    string parseJsonString(const string& body, const string& key) {
        string search_key = "\"" + key + "\":\"";
        size_t pos = body.find(search_key);
        if (pos == string::npos) {
            // 尝试不带引号的值（用于数字）
            string search_key_no_quote = "\"" + key + "\":";
            size_t pos2 = body.find(search_key_no_quote);
            if (pos2 != string::npos) {
                size_t val_start = pos2 + search_key_no_quote.length();
                size_t val_end = body.find_first_of(",}\r\n ", val_start);
                if (val_end == string::npos) val_end = body.length();
                return body.substr(val_start, val_end - val_start);
            }
            return "";
        }
        size_t val_start = pos + search_key.length();
        size_t val_end = body.find("\"", val_start);
        if (val_end == string::npos) return "";
        return body.substr(val_start, val_end - val_start);
    }

public:
    HTTPServer(int p) : port(p), server_fd(-1) {}

    bool start() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        // 创建 socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            cerr << "无法创建 socket" << endl;
            return false;
        }

        // 设置地址重用
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // 绑定地址
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (::bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cerr << "绑定端口失败" << endl;
            return false;
        }

        // 监听
        if (listen(server_fd, 10) < 0) {
            cerr << "监听失败" << endl;
            return false;
        }

        cout << "========================================" << endl;
        cout << "  C++ 视频转 GIF 服务器" << endl;
        cout << "========================================" << endl;
        cout << "服务器启动成功！" << endl;
        cout << "访问地址：http://localhost:" << port << endl;
        cout << "或访问：http://127.0.0.1:" << port << endl;
        cout << "按 Ctrl+C 停止服务器" << endl;
        cout << "========================================" << endl;

        return true;
    }

    void handleRequest(int client_fd) {
        // 动态读取所有请求数据，支持大文件上传
        string request;
        char temp_buffer[8192];
        int bytes_read;

        // 先读取第一部分
        bytes_read = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0);
        if (bytes_read <= 0) {
            close(client_fd);
            return;
        }
        request.append(temp_buffer, bytes_read);

        // 检查是否有 Content-Length，需要继续读取
        size_t content_length_pos = request.find("Content-Length: ");
        if (content_length_pos != string::npos) {
            size_t content_length_end = request.find("\r\n", content_length_pos);
            string len_str = request.substr(content_length_pos + 16, content_length_end - content_length_pos - 16);
            size_t content_length = atoi(len_str.c_str());

            // 计算已经接收了多少body数据
            size_t header_end = request.find("\r\n\r\n");
            if (header_end == string::npos) {
                header_end = request.find("\n\n");
            }

            if (header_end != string::npos) {
                size_t header_size = header_end + (request.find("\r\n\r\n") != string::npos ? 4 : 2);
                size_t body_received = request.length() - header_size;

                // 继续读取剩余数据
                while (body_received < content_length) {
                    bytes_read = recv(client_fd, temp_buffer, sizeof(temp_buffer), 0);
                    if (bytes_read <= 0) break;
                    request.append(temp_buffer, bytes_read);
                    body_received += bytes_read;
                }
            }
        }

        // 解析请求
        string method, path, version;
        stringstream ss(request);
        ss >> method >> path >> version;

        cout << "收到请求: " << method << " " << path << " (" << request.length() << " bytes)" << endl;

        // 处理不同的路径
        if (path == "/" || path == "/index.html") {
            serveFile(client_fd, "public/index.html", "text/html");
        }
        else if (path == "/upload" && method == "POST") {
            handleUpload(client_fd, request);
        }
        else if (path == "/api/convert" && method == "POST") {
            handleConvert(client_fd, request);
        }
        else if (path == "/api/history" && method == "GET") {
            handleHistory(client_fd);
        }
        else if (path == "/api/history" && method == "DELETE") {
            handleDeleteHistory(client_fd, request);
        }
        else if (path == "/api/estimate" && method == "POST") {
            handleEstimate(client_fd, request);
        }
        else if (path == "/status") {
            serveStatus(client_fd);
        }
        else if (path.find("/output/") == 0) {
            string filename = path.substr(8);
            serveFile(client_fd, "output/" + filename, "image/gif");
        }
        else if (path == "/style.css") {
            serveFile(client_fd, "public/style.css", "text/css");
        }
        else if (path == "/script.js") {
            serveFile(client_fd, "public/script.js", "application/javascript");
        }
        else {
            serve404(client_fd);
        }

        close(client_fd);
    }

    void handleUpload(int client_fd, string& request) {
        // 找到请求体的开始位置
        size_t header_end = request.find("\r\n\r\n");
        bool use_crlf = true;
        if (header_end == string::npos) {
            header_end = request.find("\n\n");
            use_crlf = false;
        }

        if (header_end == string::npos) {
            sendError(client_fd, 400, "Bad Request", "无法解析上传请求");
            return;
        }

        // 查找 Content-Disposition 中的 filename
        // 注意：filename 在 multipart body 的 Content-Disposition 头中，
        // 它在 HTTP header 结束之后，所以不能用 filename_pos > header_end 来判断
        size_t filename_pos = request.find("filename=\"");
        if (filename_pos == string::npos) {
            sendError(client_fd, 400, "Bad Request", "未找到文件名");
            return;
        }

        filename_pos += 10;
        size_t filename_end = request.find("\"", filename_pos);
        string original_filename = request.substr(filename_pos, filename_end - filename_pos);

        // 查找文件数据开始位置（跳过header后的第一个\r\n或\n）
        size_t data_start = header_end + (use_crlf ? 4 : 2);
        // multipart格式中，header后面还有 Content-Disposition 等行，需要跳过
        // 找到 "Content-Type:" 后的第一个空行，那之后才是真正的文件数据
        size_t content_type_pos = request.find("Content-Type:", header_end);
        if (content_type_pos != string::npos) {
            size_t file_header_end = request.find("\r\n\r\n", content_type_pos);
            if (file_header_end != string::npos) {
                data_start = file_header_end + 4;
            } else {
                file_header_end = request.find("\n\n", content_type_pos);
                if (file_header_end != string::npos) {
                    data_start = file_header_end + 2;
                }
            }
        }

        // 找到 multipart 边界结束的位置
        // boundary 行格式: --boundary 或 \r\n--boundary
        // 从 data_start 之后开始查找
        size_t boundary_pos = string::npos;

        // 先查找 \r\n--（前面有换行的boundary）
        size_t search_start = data_start;
        while (true) {
            boundary_pos = request.find("\r\n--", search_start);
            if (boundary_pos != string::npos) {
                // 检查这是否是结束boundary（后面跟着 \r\n 或 --）
                size_t after_boundary = boundary_pos + 4;
                // 跳过 boundary 字符串本身
                size_t next_crlf = request.find("\r\n", after_boundary);
                if (next_crlf != string::npos) {
                    // 检查是否是结束标记 (-- 结尾)
                    if (request.substr(next_crlf, 2) == "\r\n" || request.substr(next_crlf, 2) == "--") {
                        break;
                    }
                }
                search_start = boundary_pos + 1;
            } else {
                break;
            }
        }

        // 如果没找到，尝试找 \n--
        if (boundary_pos == string::npos) {
            search_start = data_start;
            while (true) {
                boundary_pos = request.find("\n--", search_start);
                if (boundary_pos != string::npos) {
                    size_t after_boundary = boundary_pos + 3;
                    size_t next_lf = request.find("\n", after_boundary);
                    if (next_lf != string::npos) {
                        if (request.substr(next_lf, 1) == "\n" || request.substr(next_lf, 2) == "--") {
                            break;
                        }
                    }
                    search_start = boundary_pos + 1;
                } else {
                    break;
                }
            }
        }

        size_t data_end;
        if (boundary_pos != string::npos) {
            // boundary_pos 指向 \r\n-- 或 \n-- 的开始
            // 文件数据在 boundary 之前结束，需要去掉前面的 \r\n 或 \n
            if (request.find("\r\n--", data_start) != string::npos &&
                boundary_pos == request.find("\r\n--", data_start)) {
                data_end = boundary_pos;  // 包含 \r\n 之前的所有内容
            } else {
                data_end = boundary_pos;
            }
        } else {
            data_end = request.length();
        }

        // 保存上传的文件
        string upload_dir = "uploads";
        mkdir(upload_dir.c_str(), 0755);

        string save_path = upload_dir + "/" + original_filename;
        ofstream outfile(save_path, ios::binary);
        if (!outfile) {
            sendError(client_fd, 500, "Server Error", "无法保存文件");
            return;
        }

        // 写入文件数据 - 使用原始字节，避免string截断\0
        size_t file_size = data_end - data_start;
        if (file_size > 0) {
            outfile.write(request.data() + data_start, file_size);
        }
        outfile.close();

        cout << "文件已上传: " << save_path << " (" << file_size << " bytes)" << endl;

        // 发送响应
        string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "\r\n"
                         "{\"success\":true,\"filename\":\"" + original_filename + "\",\"path\":\"" + save_path + "\"}";

        send(client_fd, response.c_str(), response.length(), 0);
    }

    void handleConvert(int client_fd, string& request) {
        // 解析 JSON 请求体
        size_t body_start = request.find("\r\n\r\n");
        bool body_use_crlf = true;
        if (body_start == string::npos) {
            body_start = request.find("\n\n");
            body_use_crlf = false;
        }

        if (body_start == string::npos) {
            sendError(client_fd, 400, "Bad Request", "无法解析请求");
            return;
        }

        string body = request.substr(body_start + (body_use_crlf ? 4 : 2));

        // 使用 parseJsonString 解析 JSON 参数
        string filename = parseJsonString(body, "filename");
        string start_time = parseJsonString(body, "start");
        string end_time = parseJsonString(body, "end");
        string quality = parseJsonString(body, "quality");
        string fps_str = parseJsonString(body, "fps");
        string width_str = parseJsonString(body, "width");
        string speed_str = parseJsonString(body, "speed");
        string crop = parseJsonString(body, "crop");
        string reverse_str = parseJsonString(body, "reverse");

        // 设置默认值
        if (start_time.empty()) start_time = "0";
        if (end_time.empty()) end_time = "5";
        if (quality.empty()) quality = "medium";
        if (fps_str.empty()) fps_str = "10";
        if (width_str.empty()) width_str = "480";
        if (speed_str.empty()) speed_str = "1";
        if (reverse_str.empty()) reverse_str = "false";

        if (filename.empty()) {
            sendError(client_fd, 400, "Bad Request", "文件名不能为空");
            return;
        }

        // 根据 quality 确定最大颜色数和抖动算法
        int max_colors = 128;
        string dither_option = "dither=sierra2_4a";
        if (quality == "high") {
            max_colors = 256;
            dither_option = "dither=bayer:bayer_scale=5";
        } else if (quality == "low") {
            max_colors = 64;
            dither_option = "dither=floyd_steinberg";
        }

        int fps = atoi(fps_str.c_str());
        int width = atoi(width_str.c_str());
        double speed = atof(speed_str.c_str());

        cout << "开始转换: " << filename << " (" << start_time << "s - " << end_time << "s)"
             << " quality=" << quality << " fps=" << fps << " width=" << width
             << " speed=" << speed << " crop=" << crop << " reverse=" << reverse_str << endl;

        // 创建输出目录
        mkdir("output", 0755);

        // 生成输出文件名
        string output_filename = "output_" + to_string(time(NULL)) + ".gif";
        string input_path = "uploads/" + filename;
        string output_path = "output/" + output_filename;

        // 构建 FFmpeg filter 链
        string filter_parts;

        // 速度调整（speed != 1 时添加）
        if (speed != 1.0) {
            double pts_factor = 1.0 / speed;
            ostringstream pts_ss;
            pts_ss << "setpts=" << pts_factor << "*PTS";
            filter_parts += pts_ss.str();
        }

        // 裁剪（crop 非空时添加）
        if (!crop.empty()) {
            if (!filter_parts.empty()) filter_parts += ",";
            filter_parts += "crop=" + crop;
        }

        // 帧率 + 缩放
        if (!filter_parts.empty()) filter_parts += ",";
        filter_parts += "fps=" + to_string(fps) + ",scale=" + to_string(width) + ":-1:flags=lanczos";

        // 倒放（reverse 需要放在 filter 链最后，在 split 之前）
        if (reverse_str == "true") {
            if (!filter_parts.empty()) filter_parts += ",";
            filter_parts += "reverse";
        }

        // 拼接 palettegen 和 paletteuse
        string filter_complex = filter_parts + ",split[s0][s1];[s0]palettegen=stats_mode=full:max_colors=" +
                                 to_string(max_colors) + "[p];[s1][p]paletteuse=" + dither_option;

        // 构建 FFmpeg 命令
        string ffmpeg_cmd = "ffmpeg -y -i \"" + input_path + "\" -ss " + start_time +
                            " -to " + end_time + " -vf \"" + filter_complex + "\" " +
                            "\"" + output_path + "\" 2>&1";

        cout << "执行命令: " << ffmpeg_cmd << endl;

        // 执行 FFmpeg
        FILE* pipe = popen(ffmpeg_cmd.c_str(), "r");
        if (!pipe) {
            sendError(client_fd, 500, "Server Error", "无法执行 FFmpeg");
            return;
        }

        // 读取 FFmpeg 输出
        char buffer[1024];
        string ffmpeg_output;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            ffmpeg_output += buffer;
        }

        int result = pclose(pipe);

        if (result == 0) {
            cout << "转换成功: " << output_path << endl;

            // 发送成功响应
            string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n"
                             "{\"success\":true,\"gif\":\"" + output_filename + "\"}";

            send(client_fd, response.c_str(), response.length(), 0);
        } else {
            cout << "转换失败" << endl;
            sendError(client_fd, 500, "Server Error", "FFmpeg 转换失败");
        }
    }

    // GET /api/history - 列出 output 目录下所有 .gif 文件
    void handleHistory(int client_fd) {
        string output_dir = "output";
        mkdir(output_dir.c_str(), 0755);

        DIR* dir = opendir(output_dir.c_str());
        if (!dir) {
            // 目录不存在或无法打开，返回空数组
            string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n"
                             "[]";
            send(client_fd, response.c_str(), response.length(), 0);
            return;
        }

        // 收集所有 .gif 文件信息
        vector<string> gif_entries;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            string name = entry->d_name;
            // 检查是否是 .gif 文件
            if (name.length() >= 4 && name.substr(name.length() - 4) == ".gif") {
                string filepath = output_dir + "/" + name;
                struct stat file_stat;
                if (stat(filepath.c_str(), &file_stat) == 0) {
                    // 构建单个文件条目的 JSON
                    string entry_json = "{\"filename\":\"" + name + "\","
                                       "\"size\":" + to_string(file_stat.st_size) + ","
                                       "\"created\":" + to_string(file_stat.st_mtime) + "}";
                    gif_entries.push_back(entry_json);
                }
            }
        }
        closedir(dir);

        // 按修改时间倒序排列（最新的在前面）
        // 简单起见，直接拼接数组
        string json_array = "[";
        for (size_t i = 0; i < gif_entries.size(); i++) {
            if (i > 0) json_array += ",";
            json_array += gif_entries[i];
        }
        json_array += "]";

        string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "\r\n" + json_array;

        send(client_fd, response.c_str(), response.length(), 0);
    }

    // DELETE /api/history - 删除 output 目录下指定的 .gif 文件
    void handleDeleteHistory(int client_fd, string& request) {
        // 解析 JSON 请求体
        size_t body_start = request.find("\r\n\r\n");
        bool body_use_crlf = true;
        if (body_start == string::npos) {
            body_start = request.find("\n\n");
            body_use_crlf = false;
        }

        if (body_start == string::npos) {
            sendError(client_fd, 400, "Bad Request", "无法解析请求");
            return;
        }

        string body = request.substr(body_start + (body_use_crlf ? 4 : 2));
        string filename = parseJsonString(body, "filename");

        if (filename.empty()) {
            sendError(client_fd, 400, "Bad Request", "文件名不能为空");
            return;
        }

        // 安全检查：防止路径遍历攻击
        if (filename.find("..") != string::npos || filename.find("/") != string::npos) {
            sendError(client_fd, 400, "Bad Request", "非法文件名");
            return;
        }

        string filepath = "output/" + filename;

        // 检查文件是否存在
        struct stat file_stat;
        if (stat(filepath.c_str(), &file_stat) != 0) {
            sendError(client_fd, 404, "Not Found", "文件不存在");
            return;
        }

        // 删除文件
        if (remove(filepath.c_str()) == 0) {
            cout << "文件已删除: " << filepath << endl;

            string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/json\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n"
                             "{\"success\":true}";

            send(client_fd, response.c_str(), response.length(), 0);
        } else {
            sendError(client_fd, 500, "Server Error", "删除文件失败");
        }
    }

    // POST /api/estimate - 估算 GIF 文件大小
    void handleEstimate(int client_fd, string& request) {
        // 解析 JSON 请求体
        size_t body_start = request.find("\r\n\r\n");
        bool body_use_crlf = true;
        if (body_start == string::npos) {
            body_start = request.find("\n\n");
            body_use_crlf = false;
        }

        if (body_start == string::npos) {
            sendError(client_fd, 400, "Bad Request", "无法解析请求");
            return;
        }

        string body = request.substr(body_start + (body_use_crlf ? 4 : 2));

        string duration_str = parseJsonString(body, "duration");
        string width_str = parseJsonString(body, "width");
        string fps_str = parseJsonString(body, "fps");
        string quality = parseJsonString(body, "quality");

        // 设置默认值
        if (duration_str.empty()) duration_str = "5";
        if (width_str.empty()) width_str = "480";
        if (fps_str.empty()) fps_str = "10";
        if (quality.empty()) quality = "medium";

        double duration = atof(duration_str.c_str());
        int width = atoi(width_str.c_str());
        int fps = atoi(fps_str.c_str());

        // 根据 quality 确定 bytes_per_pixel
        double bytes_per_pixel = 0.10; // medium 默认值
        if (quality == "high") {
            bytes_per_pixel = 0.15;
        } else if (quality == "low") {
            bytes_per_pixel = 0.07;
        }

        // 估算公式: estimated_size = duration * fps * width * (width * 0.75) * bytes_per_pixel
        int height = (int)(width * 0.75);
        double estimated_size = duration * fps * width * height * bytes_per_pixel;

        // 格式化文件大小文本
        string size_text;
        if (estimated_size >= 1048576) {
            ostringstream oss;
            oss << (estimated_size / 1048576.0);
            size_text = oss.str() + " MB";
        } else if (estimated_size >= 1024) {
            ostringstream oss;
            oss << (estimated_size / 1024.0);
            size_text = oss.str() + " KB";
        } else {
            size_text = to_string((int)estimated_size) + " B";
        }

        cout << "估算大小: duration=" << duration << " width=" << width
             << " fps=" << fps << " quality=" << quality
             << " => " << (long long)estimated_size << " bytes" << endl;

        // 构建响应 JSON
        ostringstream json_ss;
        json_ss << "{\"estimated_size\":" << (long long)estimated_size
                << ",\"estimated_size_text\":\"" << size_text << "\"}";

        string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "\r\n" + json_ss.str();

        send(client_fd, response.c_str(), response.length(), 0);
    }

    void serveFile(int client_fd, string filepath, string content_type) {
        ifstream file(filepath, ios::binary | ios::ate);
        if (!file) {
            serve404(client_fd);
            return;
        }

        streamsize size = file.tellg();
        file.seekg(0, ios::beg);

        char* buffer = new char[size];
        file.read(buffer, size);
        file.close();

        string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: " + content_type + "\r\n"
                         "Content-Length: " + to_string(size) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n";

        send(client_fd, response.c_str(), response.length(), 0);
        send(client_fd, buffer, size, 0);

        delete[] buffer;
    }

    void serveStatus(int client_fd) {
        string status = "{";
        status += "\"status\":\"running\",";
        status += "\"version\":\"1.0.0\",";
        status += "\"ffmpeg\":\"4.4.2\",";
        status += "\"uptime\":\"" + to_string(time(NULL)) + "\"";
        status += "}";

        string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "\r\n" + status;

        send(client_fd, response.c_str(), response.length(), 0);
    }

    void serve404(int client_fd) {
        string body = "<html><body><h1>404 Not Found</h1></body></html>";
        string response = "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Length: " + to_string(body.length()) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n" + body;

        send(client_fd, response.c_str(), response.length(), 0);
    }

    void sendError(int client_fd, int code, string title, string message) {
        string body = "<html><body><h1>" + to_string(code) + " " + title + "</h1><p>" + message + "</p></body></html>";
        string response = "HTTP/1.1 " + to_string(code) + " " + title + "\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Length: " + to_string(body.length()) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n" + body;

        send(client_fd, response.c_str(), response.length(), 0);
    }

    void run() {
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                cerr << "接受连接失败" << endl;
                continue;
            }

            // 为每个请求创建新线程处理
            pthread_t thread;
            int* fd_copy = new int(client_fd);
            pthread_create(&thread, NULL, handleRequestThread, fd_copy);
            pthread_detach(thread);
        }
    }

    static void* handleRequestThread(void* param) {
        int client_fd = *(int*)param;
        delete (int*)param;

        HTTPServer server(0);
        server.handleRequest(client_fd);
        return NULL;
    }

    ~HTTPServer() {
        if (server_fd >= 0) {
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // 忽略 SIGPIPE 信号
    signal(SIGPIPE, SIG_IGN);

    HTTPServer server(port);
    if (!server.start()) {
        cerr << "服务器启动失败" << endl;
        return 1;
    }

    server.run();

    return 0;
}
