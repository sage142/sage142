#include <asio.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using asio::ip::tcp;

struct ChatMessage {
  std::string author;
  std::string content;
};

class ChatServer {
 public:
  bool start(unsigned short port) {
    if (running_) return false;

    running_ = true;
    io_context_ = std::make_unique<asio::io_context>();
    acceptor_ = std::make_unique<tcp::acceptor>(*io_context_, tcp::endpoint(tcp::v4(), port));

    do_accept();
    network_thread_ = std::thread([this] { io_context_->run(); });
    return true;
  }

  void stop() {
    if (!running_) return;
    running_ = false;

    asio::post(*io_context_, [this] {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      for (auto& client : clients_) {
        std::error_code ec;
        client->socket.close(ec);
      }
      clients_.clear();
    });

    std::error_code ec;
    acceptor_->close(ec);
    io_context_->stop();
    if (network_thread_.joinable()) network_thread_.join();
  }

  ~ChatServer() { stop(); }

 private:
  struct ClientSession : std::enable_shared_from_this<ClientSession> {
    tcp::socket socket;
    asio::streambuf read_buffer;
    std::string username = "guest";

    explicit ClientSession(asio::io_context& io) : socket(io) {}
  };

  void do_accept() {
    auto client = std::make_shared<ClientSession>(*io_context_);

    acceptor_->async_accept(client->socket, [this, client](std::error_code ec) {
      if (!ec && running_) {
        {
          std::lock_guard<std::mutex> lock(clients_mutex_);
          clients_.push_back(client);
        }
        write_line(client, "SYS Welcome! Use HELLO <name> then MSG <text>\\n");
        do_read(client);
      }

      if (running_) do_accept();
    });
  }

  void do_read(const std::shared_ptr<ClientSession>& client) {
    asio::async_read_until(client->socket, client->read_buffer, '\n',
                           [this, client](std::error_code ec, std::size_t) {
                             if (ec || !running_) {
                               remove_client(client);
                               return;
                             }

                             std::istream stream(&client->read_buffer);
                             std::string line;
                             std::getline(stream, line);

                             if (line.rfind("HELLO ", 0) == 0) {
                               client->username = line.substr(6);
                               broadcast("SYS " + client->username + " joined the server\\n");
                             } else if (line.rfind("MSG ", 0) == 0) {
                               broadcast("CHAT " + client->username + ": " + line.substr(4) + "\\n");
                             }

                             do_read(client);
                           });
  }

  void broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& client : clients_) write_line(client, message);
  }

  void write_line(const std::shared_ptr<ClientSession>& client, const std::string& line) {
    asio::async_write(client->socket, asio::buffer(line),
                      [client](std::error_code, std::size_t) {});
  }

  void remove_client(const std::shared_ptr<ClientSession>& target) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), target), clients_.end());
  }

  std::atomic<bool> running_ = false;
  std::unique_ptr<asio::io_context> io_context_;
  std::unique_ptr<tcp::acceptor> acceptor_;
  std::thread network_thread_;
  std::mutex clients_mutex_;
  std::vector<std::shared_ptr<ClientSession>> clients_;
};

class ChatClient {
 public:
  bool connect(const std::string& host, const std::string& port, const std::string& username) {
    disconnect();

    try {
      io_context_ = std::make_unique<asio::io_context>();
      resolver_ = std::make_unique<tcp::resolver>(*io_context_);
      socket_ = std::make_unique<tcp::socket>(*io_context_);

      auto endpoints = resolver_->resolve(host, port);
      asio::connect(*socket_, endpoints);

      running_ = true;
      do_read();
      network_thread_ = std::thread([this] { io_context_->run(); });

      send_raw("HELLO " + username + "\n");
      return true;
    } catch (const std::exception& ex) {
      std::cerr << "Connect failed: " << ex.what() << '\n';
      disconnect();
      return false;
    }
  }

  void disconnect() {
    if (!io_context_) return;
    running_ = false;

    if (socket_ && socket_->is_open()) {
      std::error_code ec;
      socket_->shutdown(tcp::socket::shutdown_both, ec);
      socket_->close(ec);
    }

    io_context_->stop();
    if (network_thread_.joinable()) network_thread_.join();

    socket_.reset();
    resolver_.reset();
    io_context_.reset();
  }

  ~ChatClient() { disconnect(); }

  void send_chat(const std::string& text) { send_raw("MSG " + text + "\n"); }

  bool is_connected() const { return running_; }

  std::vector<ChatMessage> drain_messages() {
    std::lock_guard<std::mutex> lock(messages_mutex_);
    std::vector<ChatMessage> out(messages_.begin(), messages_.end());
    messages_.clear();
    return out;
  }

 private:
  void send_raw(const std::string& line) {
    if (!socket_ || !socket_->is_open()) return;
    asio::write(*socket_, asio::buffer(line));
  }

  void do_read() {
    asio::async_read_until(*socket_, read_buffer_, '\n', [this](std::error_code ec, std::size_t) {
      if (ec || !running_) {
        running_ = false;
        return;
      }

      std::istream stream(&read_buffer_);
      std::string line;
      std::getline(stream, line);

      ChatMessage msg;
      if (line.rfind("CHAT ", 0) == 0) {
        auto payload = line.substr(5);
        auto sep = payload.find(':');
        if (sep != std::string::npos) {
          msg.author = payload.substr(0, sep);
          msg.content = payload.substr(sep + 2);
        } else {
          msg.author = "chat";
          msg.content = payload;
        }
      } else if (line.rfind("SYS ", 0) == 0) {
        msg.author = "system";
        msg.content = line.substr(4);
      } else {
        msg.author = "raw";
        msg.content = line;
      }

      {
        std::lock_guard<std::mutex> lock(messages_mutex_);
        messages_.push_back(msg);
      }

      do_read();
    });
  }

  std::atomic<bool> running_ = false;
  std::unique_ptr<asio::io_context> io_context_;
  std::unique_ptr<tcp::resolver> resolver_;
  std::unique_ptr<tcp::socket> socket_;
  asio::streambuf read_buffer_;
  std::thread network_thread_;
  std::mutex messages_mutex_;
  std::deque<ChatMessage> messages_;
};

int main() {
  if (!glfwInit()) return 1;

  const char* glsl_version = "#version 130";
  GLFWwindow* window = glfwCreateWindow(1280, 720, "Discord-ish ImGui Chat", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ChatServer local_server;
  ChatClient client;

  std::vector<ChatMessage> feed;

  std::array<char, 64> username_buf{"user"};
  std::array<char, 64> ip_buf{"127.0.0.1"};
  std::array<char, 16> port_buf{"7777"};
  std::array<char, 512> message_buf{};

  bool host_started = false;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for (const auto& m : client.drain_messages()) feed.push_back(m);

    ImGui::Begin("Discord-like Local Prototype");

    ImGui::TextWrapped("Simulates joining/hosting servers by IP + port. This is a demo protocol, not production-ready.");
    ImGui::Separator();

    ImGui::InputText("Username", username_buf.data(), username_buf.size());
    ImGui::InputText("Server IP", ip_buf.data(), ip_buf.size());
    ImGui::InputText("Port", port_buf.data(), port_buf.size());

    if (!host_started) {
      if (ImGui::Button("Host Server")) {
        host_started = local_server.start(static_cast<unsigned short>(std::stoi(port_buf.data())));
        if (host_started) feed.push_back({"system", "Local server started."});
      }
    } else {
      ImGui::SameLine();
      if (ImGui::Button("Stop Hosting")) {
        local_server.stop();
        host_started = false;
        feed.push_back({"system", "Local server stopped."});
      }
    }

    ImGui::SameLine();
    if (!client.is_connected()) {
      if (ImGui::Button("Join Server")) {
        if (client.connect(ip_buf.data(), port_buf.data(), username_buf.data())) {
          feed.push_back({"system", "Connected to server."});
        } else {
          feed.push_back({"system", "Connection failed."});
        }
      }
    } else {
      if (ImGui::Button("Disconnect")) {
        client.disconnect();
        feed.push_back({"system", "Disconnected."});
      }
    }

    ImGui::Separator();
    ImGui::Text("#general");
    ImGui::BeginChild("feed", ImVec2(0, -50), true);
    for (const auto& msg : feed) {
      ImGui::TextWrapped("[%s] %s", msg.author.c_str(), msg.content.c_str());
    }
    ImGui::EndChild();

    ImGui::InputText("Message", message_buf.data(), message_buf.size());
    ImGui::SameLine();
    if (ImGui::Button("Send") && client.is_connected() && std::strlen(message_buf.data()) > 0) {
      client.send_chat(message_buf.data());
      message_buf[0] = '\0';
    }

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  client.disconnect();
  local_server.stop();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
